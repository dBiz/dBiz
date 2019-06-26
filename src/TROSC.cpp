
#include "plugin.hpp"

using simd::float_4;

// Accurate only on [0, 1]
template <typename T>
T sin2pi_pade_05_7_6(T x)
{
	x -= 0.5f;
	return (T(-6.28319) * x + T(35.353) * simd::pow(x, 3) - T(44.9043) * simd::pow(x, 5) + T(16.0951) * simd::pow(x, 7)) / (1 + T(0.953136) * simd::pow(x, 2) + T(0.430238) * simd::pow(x, 4) + T(0.0981408) * simd::pow(x, 6));
}

template <typename T>
T sin2pi_pade_05_5_4(T x)
{
	x -= 0.5f;
	return (T(-6.283185307) * x + T(33.19863968) * simd::pow(x, 3) - T(32.44191367) * simd::pow(x, 5)) / (1 + T(1.296008659) * simd::pow(x, 2) + T(0.7028072946) * simd::pow(x, 4));
}

template <typename T>
T expCurve(T x)
{
	return (3 + x * (-13 + 5 * x)) / (3 + 2 * x);
}

template <int OVERSAMPLE, int QUALITY, typename T>
struct Oscillator
{
	bool analog = false;
	bool soft = false;
	bool syncEnabled = false;
	// For optimizing in serial code
	int channels = 0;

	T lastSyncValue = 0.f;
	T phase = 0.f;
	T freq;
	T pulseWidth = 0.5f;
	T syncDirection = 1.f;

	dsp::TRCFilter<T> sqrFilter;

	dsp::MinBlepGenerator<QUALITY, OVERSAMPLE, T> sqrMinBlep;
	dsp::MinBlepGenerator<QUALITY, OVERSAMPLE, T> sawMinBlep;
	dsp::MinBlepGenerator<QUALITY, OVERSAMPLE, T> triMinBlep;
	dsp::MinBlepGenerator<QUALITY, OVERSAMPLE, T> sinMinBlep;

	T sqrValue = 0.f;
	T sawValue = 0.f;
	T triValue = 0.f;
	T sinValue = 0.f;

	void setPitch(T pitch)
	{
		freq = dsp::FREQ_C4 * simd::pow(2.f, pitch);
	}
	void setPulseWidth(T pulseWidth)
	{
		const float pwMin = 0.01f;
		this->pulseWidth = simd::clamp(pulseWidth, pwMin, 1.f - pwMin);
	}

	void process(float deltaTime, T syncValue)
	{
		// Advance phase
		T deltaPhase = simd::clamp(freq * deltaTime, 1e-6f, 0.35f);
		if (soft)
		{
			// Reverse direction
			deltaPhase *= syncDirection;
		}
		else
		{
			// Reset back to forward
			syncDirection = 1.f;
		}
		phase += deltaPhase;
		// Wrap phase
		phase -= simd::floor(phase);

		// Jump sqr when crossing 0, or 1 if backwards
		T wrapPhase = (syncDirection == -1.f) & 1.f;
		T wrapCrossing = (wrapPhase - (phase - deltaPhase)) / deltaPhase;
		int wrapMask = simd::movemask((0 < wrapCrossing) & (wrapCrossing <= 1.f));
		if (wrapMask)
		{
			for (int i = 0; i < channels; i++)
			{
				if (wrapMask & (1 << i))
				{
					T mask = simd::movemaskInverse<T>(1 << i);
					float p = wrapCrossing[i] - 1.f;
					T x = mask & (2.f * syncDirection);
					sqrMinBlep.insertDiscontinuity(p, x);
				}
			}
		}

		// Jump sqr when crossing `pulseWidth`
		T pulseCrossing = (pulseWidth - (phase - deltaPhase)) / deltaPhase;
		int pulseMask = simd::movemask((0 < pulseCrossing) & (pulseCrossing <= 1.f));
		if (pulseMask)
		{
			for (int i = 0; i < channels; i++)
			{
				if (pulseMask & (1 << i))
				{
					T mask = simd::movemaskInverse<T>(1 << i);
					float p = pulseCrossing[i] - 1.f;
					T x = mask & (-2.f * syncDirection);
					sqrMinBlep.insertDiscontinuity(p, x);
				}
			}
		}

		// Jump saw when crossing 0.5
		T halfCrossing = (0.5f - (phase - deltaPhase)) / deltaPhase;
		int halfMask = simd::movemask((0 < halfCrossing) & (halfCrossing <= 1.f));
		if (halfMask)
		{
			for (int i = 0; i < channels; i++)
			{
				if (halfMask & (1 << i))
				{
					T mask = simd::movemaskInverse<T>(1 << i);
					float p = halfCrossing[i] - 1.f;
					T x = mask & (-2.f * syncDirection);
					sawMinBlep.insertDiscontinuity(p, x);
				}
			}
		}

		// Detect sync
		// Might be NAN or outside of [0, 1) range
		if (syncEnabled)
		{
			T syncCrossing = -lastSyncValue / (syncValue - lastSyncValue);
			lastSyncValue = syncValue;
			T sync = (0.f < syncCrossing) & (syncCrossing <= 1.f);
			int syncMask = simd::movemask(sync);
			if (syncMask)
			{
				if (soft)
				{
					syncDirection = simd::ifelse(sync, -syncDirection, syncDirection);
				}
				else
				{
					T newPhase = simd::ifelse(sync, syncCrossing * deltaPhase, phase);
					// Insert minBLEP for sync
					for (int i = 0; i < channels; i++)
					{
						if (syncMask & (1 << i))
						{
							T mask = simd::movemaskInverse<T>(1 << i);
							float p = syncCrossing[i] - 1.f;
							T x;
							x = mask & (sqr(newPhase) - sqr(phase));
							sqrMinBlep.insertDiscontinuity(p, x);
							x = mask & (saw(newPhase) - saw(phase));
							sawMinBlep.insertDiscontinuity(p, x);
							x = mask & (tri(newPhase) - tri(phase));
							triMinBlep.insertDiscontinuity(p, x);
							x = mask & (sin(newPhase) - sin(phase));
							sinMinBlep.insertDiscontinuity(p, x);
						}
					}
					phase = newPhase;
				}
			}
		}

		// Square
		sqrValue = sqr(phase);
		sqrValue += sqrMinBlep.process();

		if (analog)
		{
			sqrFilter.setCutoffFreq(20.f * deltaTime);
			sqrFilter.process(sqrValue);
			sqrValue = sqrFilter.highpass() * 0.95f;
		}

		// Saw
		sawValue = saw(phase);
		sawValue += sawMinBlep.process();

		// Tri
		triValue = tri(phase);
		triValue += triMinBlep.process();

		// Sin
		sinValue = sin(phase);
		sinValue += sinMinBlep.process();
	}

	T sin(T phase)
	{
		T v;
		if (analog)
		{
			// Quadratic approximation of sine, slightly richer harmonics
			T halfPhase = (phase < 0.5f);
			T x = phase - simd::ifelse(halfPhase, 0.25f, 0.75f);
			v = 1.f - 16.f * simd::pow(x, 2);
			v *= simd::ifelse(halfPhase, 1.f, -1.f);
		}
		else
		{
			v = sin2pi_pade_05_5_4(phase);
			// v = sin2pi_pade_05_7_6(phase);
			// v = simd::sin(2 * T(M_PI) * phase);
		}
		return v;
	}
	T sin()
	{
		return sinValue;
	}

	T tri(T phase)
	{
		T v;
		if (analog)
		{
			T x = phase + 0.25f;
			x -= simd::trunc(x);
			T halfX = (x < 0.5f);
			x = 2 * x - simd::ifelse(halfX, 0.f, 1.f);
			v = expCurve(x) * simd::ifelse(halfX, 1.f, -1.f);
		}
		else
		{
			v = 1 - 4 * simd::fmin(simd::fabs(phase - 0.25f), simd::fabs(phase - 1.25f));
		}
		return v;
	}
	T tri()
	{
		return triValue;
	}

	T saw(T phase)
	{
		T v;
		T x = phase + 0.5f;
		x -= simd::trunc(x);
		if (analog)
		{
			v = -expCurve(x);
		}
		else
		{
			v = 2 * x - 1;
		}
		return v;
	}
	T saw()
	{
		return sawValue;
	}

	T sqr(T phase)
	{
		T v = simd::ifelse(phase < pulseWidth, 1.f, -1.f);
		return v;
	}
	T sqr()
	{
		return sqrValue;
	}

	T light()
	{
		return simd::sin(2 * T(M_PI) * phase);
	}
};

struct TROSC : Module
{
	enum ParamIds
	{
		LINK_A_PARAM,
		LINK_B_PARAM,

		MODE_A_PARAM,
		SYNC_A_PARAM,
		MODE_B_PARAM,
		SYNC_B_PARAM,
		MODE_C_PARAM,
		SYNC_C_PARAM,

		WAVE_A_SEL_PARAM,
		WAVE_B_SEL_PARAM,
		WAVE_C_SEL_PARAM,

		FREQ_A_PARAM,
		FINE_A_PARAM,
		FREQ_B_PARAM,
		FINE_B_PARAM,
		FREQ_C_PARAM,
		FINE_C_PARAM,

		FM_A_PARAM,
		FM_B_PARAM,
		FM_C_PARAM,

		LEVEL_A_PARAM,
		LEVEL_B_PARAM,
		LEVEL_C_PARAM,

		WAVE_A_MIX,
		WAVE2_A_MIX,
		WAVE_B_MIX,
		WAVE2_B_MIX,
		WAVE_C_MIX,
		C_WIDTH_PARAM,
		NUM_PARAMS

	};
	enum InputIds
	{
		PITCH_A_INPUT,
		PITCH_B_INPUT,
		PITCH_C_INPUT,

		SYNC_A_INPUT,
		SYNC_B_INPUT,
		SYNC_C_INPUT,

		FM_A_INPUT,
		FM_B_INPUT,
		FM_C_INPUT,

		A_WAVE_MIX_INPUT,
		B_WAVE_MIX_INPUT,
		C_WAVE_MIX_INPUT,

		A_VOL_IN,
		B_VOL_IN,
		C_VOL_IN,

		C_WIDTH_INPUT,

		NUM_INPUTS

	};
	enum OutputIds
	{
		A_OUTPUT,
		B_OUTPUT,
		C_OUTPUT,
		MIX_OUTPUT,
		NUM_OUTPUTS

	};
	enum LightIds
	{
		NUM_LIGHTS

	};

	Oscillator<8, 8,float_4> a_osc[4];
	Oscillator<8, 8,float_4> b_osc[4];
	Oscillator<8, 8,float_4> c_osc[4];

	TROSC() {
	
	config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

	configParam(FREQ_A_PARAM,  -54.f, 54.f, 0.f, "Osc1 Frequency", "Hz", std::pow(2, 1 / 12.f), dsp::FREQ_C4);
	configParam(FREQ_B_PARAM,  -54.f, 54.f, 0.f, "Osc2 Frequency", "Hz", std::pow(2, 1 / 12.f), dsp::FREQ_C4);
	configParam(FREQ_C_PARAM,  -54.f, 54.f, 0.f, "Osc3 Frequency", "Hz", std::pow(2, 1 / 12.f), dsp::FREQ_C4);

	configParam(MODE_A_PARAM,  0.f, 1.f, 1.f, "Osc1 Analog mode");
	configParam(MODE_B_PARAM,  0.f, 1.f, 1.f, "Osc2 Analog mode");
	configParam(MODE_C_PARAM,  0.f, 1.f, 1.f, "Osc3 Analog mode");

	configParam(SYNC_A_PARAM,  0.f, 1.f, 1.f, "Osc1 Hard sync");
	configParam(SYNC_B_PARAM,  0.f, 1.f, 1.f, "Osc2 Hard sync");
	configParam(SYNC_C_PARAM,  0.f, 1.f, 1.f, "Osc3 Hard sync");

	configParam(FINE_A_PARAM,  -1.f, 1.f, 0.f, "Osc1 Fine frequency");
	configParam(FINE_B_PARAM,  -1.f, 1.f, 0.f, "Osc2 Fine frequency");
	configParam(FINE_C_PARAM,  -1.f, 1.f, 0.f, "Osc3 Fine frequency");

	configParam(FM_A_PARAM,  0.f, 1.f, 0.f, "Osc1 Frequency modulation");
	configParam(FM_B_PARAM,  0.f, 1.f, 0.f, "Osc2 Frequency modulation");
	configParam(FM_C_PARAM,  0.f, 1.f, 0.f, "Osc3 Frequency modulation");

	configParam(LEVEL_A_PARAM,  0.f, 1.f, 0.f, "Osc1 Amp Level");
	configParam(LEVEL_B_PARAM,  0.f, 1.f, 0.f, "Osc2 Amp Level");
	configParam(LEVEL_C_PARAM,  0.f, 1.f, 0.f, "Osc3 Amp Level");

	configParam(WAVE_A_MIX,  0.f, 1.f, 0.f, "Wave A Level");
	configParam(WAVE2_A_MIX,  0.f, 1.f, 0.f, "Wave 2A Level");
	configParam(WAVE_B_MIX,  0.f, 1.f, 0.f, "Wave B Level");
	configParam(WAVE2_B_MIX,  0.f, 1.f, 0.f, "Wave 2B Level");
	configParam(WAVE_C_MIX,  0.f, 1.f, 0.f, "Wave C Level");
	configParam(C_WIDTH_PARAM,  0.f, 1.f, 0.f, "Wave C Width Level");

	configParam(WAVE_A_SEL_PARAM,  0.f, 1.f, 0.f, "Wave AMix Level");
	configParam(WAVE_B_SEL_PARAM,  0.f, 1.f, 0.f, "Wave BMix Level");
	configParam(WAVE_C_SEL_PARAM,  0.f, 1.f, 0.f, "Wave CMix Level");

	configParam(LINK_A_PARAM,  0.f, 1.f, 0.f, "Link A Param");
	configParam(LINK_B_PARAM,  0.f, 1.f, 0.f, "Link B Param");
	}
	
	void process(const ProcessArgs &args) override 
	{

		float freqParamA = params[FREQ_A_PARAM].getValue() / 12.f;
		freqParamA += dsp::quadraticBipolar(params[FINE_A_PARAM].getValue()) * 3.f / 12.f;
		float fmParamA = dsp::quadraticBipolar(params[FM_A_PARAM].getValue());
		
		float freqParamB = params[FREQ_B_PARAM].getValue() / 12.f;
		freqParamB += dsp::quadraticBipolar(params[FINE_B_PARAM].getValue()) * 3.f / 12.f;
		float fmParamB = dsp::quadraticBipolar(params[FM_B_PARAM].getValue());
		
		float freqParamC = params[FREQ_C_PARAM].getValue() / 12.f;
		freqParamC += dsp::quadraticBipolar(params[FINE_C_PARAM].getValue()) * 3.f / 12.f;
		float fmParamC = dsp::quadraticBipolar(params[FM_C_PARAM].getValue());
		

		int channelsA = std::max(inputs[PITCH_A_INPUT].getChannels(), 1);
		int channelsB = std::max(inputs[PITCH_B_INPUT].getChannels(), 1);
		int channelsC = std::max(inputs[PITCH_C_INPUT].getChannels(), 1);

		float_4 out_a;
		float_4 out2_a;
		float_4 a_out;
		float_4 mixa;

		float_4 out_b;
		float_4 out2_b;
		float_4 b_out;
		float_4 mixb;

		float_4 out_c;
		float_4 out2_c;
		float_4 c_out;
		float_4 mixc;

		float_4 linka, linkb,linkc;

		for (int c = 0; c < channelsA; c += 4)
		{
			
			linka = inputs[PITCH_A_INPUT].getVoltageSimd<float_4>(c);
			linkb = inputs[PITCH_B_INPUT].getVoltageSimd<float_4>(c);
			linkc = inputs[PITCH_C_INPUT].getVoltageSimd<float_4>(c);

			if (params[LINK_A_PARAM].getValue() == 0) linkb = linka;
			if (params[LINK_B_PARAM].getValue() == 0) linkc = linkb;
		}

			for (int c = 0; c < channelsA; c += 4)
			{
				float_4 pitchA;
				//	auto *oscillator = &a_osc[c / 4];
				a_osc->channels = std::min(channelsA - c, 4);
				a_osc->analog = params[MODE_A_PARAM].getValue() > 0.f;
				a_osc->soft = params[SYNC_A_PARAM].getValue() <= 0.f;

				pitchA = freqParamA;
				pitchA += linka;

				if (inputs[FM_A_INPUT].isConnected())
				{
					pitchA += fmParamA * inputs[FM_A_INPUT].getPolyVoltageSimd<float_4>(c);
				}

				a_osc->setPitch(pitchA);

				a_osc->syncEnabled = inputs[SYNC_A_INPUT].isConnected();
				a_osc->process(args.sampleTime, inputs[SYNC_A_INPUT].getPolyVoltageSimd<float_4>(c));

				float wave_a = clamp(params[WAVE_A_MIX].getValue(), 0.0f, 1.0f);
				float wave2_a = clamp(params[WAVE2_A_MIX].getValue(), 0.0f, 1.0f);
				float mix_a = clamp(params[WAVE_A_SEL_PARAM].getValue(), 0.f, 1.f);
				if (inputs[A_WAVE_MIX_INPUT].isConnected())
					mix_a *= clamp(inputs[A_WAVE_MIX_INPUT].getVoltage() / 10.f, 0.f, 1.f);

				out_a = crossfade(a_osc->sin(), a_osc->tri(), wave_a);
				out2_a = crossfade(a_osc->saw(), a_osc->sqr(), wave2_a);
				a_out = crossfade(out_a, out2_a, mix_a);

				mixa = 5 * (a_out * params[LEVEL_A_PARAM].getValue());
				if (inputs[A_VOL_IN].isConnected())
					mixa *= clamp(inputs[A_VOL_IN].getVoltage() / 10.f, -1.0f, 1.0f);

				outputs[A_OUTPUT].setVoltageSimd(mixa, c);
			}
			for (int c = 0; c < channelsB; c += 4)
			{
				float_4 pitchB;
				//	auto *oscillator = &a_osc[c / 4];
				b_osc->channels = std::min(channelsB - c, 4);
				b_osc->analog = params[MODE_B_PARAM].getValue() > 0.f;
				b_osc->soft = params[SYNC_B_PARAM].getValue() <= 0.f;

				pitchB = freqParamB;
				pitchB += linkb;

				if (inputs[FM_B_INPUT].isConnected())
				{
					pitchB += fmParamB * inputs[FM_B_INPUT].getPolyVoltageSimd<float_4>(c);
				}

				b_osc->setPitch(pitchB);

				b_osc->syncEnabled = inputs[SYNC_B_INPUT].isConnected();
				b_osc->process(args.sampleTime, inputs[SYNC_B_INPUT].getPolyVoltageSimd<float_4>(c));

				float wave_b = clamp(params[WAVE_B_MIX].getValue(), 0.0f, 1.0f);
				float wave2_b = clamp(params[WAVE2_B_MIX].getValue(), 0.0f, 1.0f);
				float mix_b = clamp(params[WAVE_B_SEL_PARAM].getValue(), 0.f, 1.f);
				if (inputs[B_WAVE_MIX_INPUT].isConnected())
					mix_b *= clamp(inputs[A_WAVE_MIX_INPUT].getVoltage() / 10.f, 0.f, 1.f);

				out_b = crossfade(b_osc->sin(), b_osc->tri(), wave_b);
				out2_b = crossfade(b_osc->saw(), b_osc->sqr(), wave2_b);
				b_out = crossfade(out_b, out2_b, mix_b);

				mixb = 5 * (b_out * params[LEVEL_B_PARAM].getValue());
				if (inputs[B_VOL_IN].isConnected())
					mixb *= clamp(inputs[B_VOL_IN].getVoltage() / 10.f, -1.0f, 1.0f);

				outputs[B_OUTPUT].setVoltageSimd(mixb, c);
			}
			for (int c = 0; c < channelsC; c += 4)
			{
				float_4 pitchC;
				//	auto *oscillator = &a_osc[c / 4];
				c_osc->channels = std::min(channelsC - c, 4);
				c_osc->analog = params[MODE_C_PARAM].getValue() > 0.f;
				c_osc->soft = params[SYNC_C_PARAM].getValue() <= 0.f;

				pitchC = freqParamC;
				pitchC += linkc;

				if (inputs[FM_C_INPUT].isConnected())
				{
					pitchC += fmParamC * inputs[FM_C_INPUT].getPolyVoltageSimd<float_4>(c);
				}

				c_osc->setPitch(pitchC);
				c_osc->setPulseWidth(params[C_WIDTH_PARAM].getValue() + params[C_WIDTH_PARAM].getValue() * inputs[C_WIDTH_INPUT].getPolyVoltageSimd<float_4>(c) / 10.f);

				c_osc->syncEnabled = inputs[SYNC_A_INPUT].isConnected();
				c_osc->process(args.sampleTime, inputs[SYNC_A_INPUT].getPolyVoltageSimd<float_4>(c));

				float wave_c = clamp(params[WAVE_C_MIX].getValue(), 0.0f, 1.0f);
				float mix_c = clamp(params[WAVE_C_SEL_PARAM].getValue(), 0.f, 1.f);
				if (inputs[C_WAVE_MIX_INPUT].isConnected())
					mix_c *= clamp(inputs[C_WAVE_MIX_INPUT].getVoltage() / 10.f, 0.f, 1.f);

				out_c = crossfade(c_osc->sin(), c_osc->tri(), wave_c);
				out2_c = c_osc->sqr();
				c_out = crossfade(out_c, out2_c, mix_c);

				mixc = 5 * (c_out * params[LEVEL_C_PARAM].getValue());
				if (inputs[C_VOL_IN].isConnected())
					mixc *= clamp(inputs[C_VOL_IN].getVoltage() / 10.f, -1.0f, 1.0f);

				outputs[C_OUTPUT].setVoltageSimd(mixc, c);
			}

			outputs[MIX_OUTPUT].setVoltageSimd(clamp((mixa + mixb + mixc) / 5, -5.f, 5.f), 0);
  }


};

struct TROSCWidget : ModuleWidget {
	TROSCWidget(TROSC *module){
	 setModule(module);
	 setPanel(APP->window->loadSvg(asset::plugin(pluginInstance,  "res/TROSC.svg")));

	 addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
	 addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
	 addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
	 addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

	 int space = 170;
	 int vspace = 50;

	 addParam(createParam<VerboDL>(Vec(30, 20), module, TROSC::FREQ_A_PARAM));
	 addParam(createParam<VerboDL>(Vec(30, 150), module, TROSC::FREQ_B_PARAM));
	 addParam(createParam<VerboDL>(Vec(30, 280), module, TROSC::FREQ_C_PARAM));

	 addParam(createParam<MCKSSS2>(Vec(5, 5 + 20), module, TROSC::MODE_A_PARAM));
	 addParam(createParam<MCKSSS2>(Vec(5, 5 + 150), module, TROSC::MODE_B_PARAM));
	 addParam(createParam<MCKSSS2>(Vec(5, 5 + 280), module, TROSC::MODE_C_PARAM));
	 addParam(createParam<MCKSSS2>(Vec(143, 75 + 20), module, TROSC::SYNC_A_PARAM));
	 addParam(createParam<MCKSSS2>(Vec(143, 75 + 150), module, TROSC::SYNC_B_PARAM));
	 addParam(createParam<MCKSSS2>(Vec(143, 75 + 280), module, TROSC::SYNC_C_PARAM));

	 addParam(createParam<VerboDS>(Vec(110, 20), module, TROSC::FINE_A_PARAM));
	 addParam(createParam<VerboDS>(Vec(110, 150), module, TROSC::FINE_B_PARAM));
	 addParam(createParam<VerboDS>(Vec(110, 280), module, TROSC::FINE_C_PARAM));

	 addParam(createParam<VerboDS>(Vec(150, 20 - 10), module, TROSC::FM_A_PARAM));
	 addParam(createParam<VerboDS>(Vec(150, 150 - 10), module, TROSC::FM_B_PARAM));
	 addParam(createParam<VerboDS>(Vec(150, 280 - 10), module, TROSC::FM_C_PARAM));

	 addParam(createParam<VerboDS>(Vec(250, vspace + 20), module, TROSC::LEVEL_A_PARAM));
	 addParam(createParam<VerboDS>(Vec(250, vspace + 150), module, TROSC::LEVEL_B_PARAM));
	 addParam(createParam<VerboDS>(Vec(250, vspace + 280), module, TROSC::LEVEL_C_PARAM));

	 addParam(createParam<LEDSliderGreen>(Vec(20 + space, 20), module, TROSC::WAVE_A_MIX));
	 addParam(createParam<LEDSliderGreen>(Vec(50 + space, 20), module, TROSC::WAVE2_A_MIX));
	 addParam(createParam<LEDSliderGreen>(Vec(20 + space, 150), module, TROSC::WAVE_B_MIX));
	 addParam(createParam<LEDSliderGreen>(Vec(50 + space, 150), module, TROSC::WAVE2_B_MIX));
	 addParam(createParam<LEDSliderGreen>(Vec(20 + space, 280), module, TROSC::WAVE_C_MIX));
	 addParam(createParam<VerboDS>(Vec(40 + space, 290), module, TROSC::C_WIDTH_PARAM));

	 addParam(createParam<Trimpot>(Vec(73 + space, 20 - 10), module, TROSC::WAVE_A_SEL_PARAM));
	 addParam(createParam<Trimpot>(Vec(73 + space, 150 - 10), module, TROSC::WAVE_B_SEL_PARAM));
	 addParam(createParam<Trimpot>(Vec(73 + space, 280 - 10), module, TROSC::WAVE_C_SEL_PARAM));

	 addInput(createInput<PJ301MCPort>(Vec(100 + space, 20 - 13), module, TROSC::A_WAVE_MIX_INPUT));
	 addInput(createInput<PJ301MCPort>(Vec(100 + space, 150 - 13), module, TROSC::B_WAVE_MIX_INPUT));
	 addInput(createInput<PJ301MCPort>(Vec(100 + space, 280 - 13), module, TROSC::C_WAVE_MIX_INPUT));

	 addInput(createInput<PJ301MCPort>(Vec(2, 30 + 20), module, TROSC::PITCH_A_INPUT));
	 addInput(createInput<PJ301MCPort>(Vec(2, 30 + 150), module, TROSC::PITCH_B_INPUT));
	 addInput(createInput<PJ301MCPort>(Vec(2, 30 + 280), module, TROSC::PITCH_C_INPUT));

	 addParam(createParam<SilverSwitch>(Vec(60, 90 + 20), module, TROSC::LINK_A_PARAM));
	 addParam(createParam<SilverSwitch>(Vec(60, 90 + 150), module, TROSC::LINK_B_PARAM));

	 addInput(createInput<PJ301MOrPort>(Vec(115, 55 + 20), module, TROSC::SYNC_A_INPUT));
	 addInput(createInput<PJ301MOrPort>(Vec(115, 55 + 150), module, TROSC::SYNC_B_INPUT));
	 addInput(createInput<PJ301MOrPort>(Vec(115, 55 + 280), module, TROSC::SYNC_C_INPUT));

	 addInput(createInput<PJ301MCPort>(Vec(155, 45 + 20), module, TROSC::FM_A_INPUT));
	 addInput(createInput<PJ301MCPort>(Vec(155, 45 + 150), module, TROSC::FM_B_INPUT));
	 addInput(createInput<PJ301MCPort>(Vec(155, 45 + 280), module, TROSC::FM_C_INPUT));

	 addInput(createInput<PJ301MCPort>(Vec(290, vspace + 10 + 20), module, TROSC::A_VOL_IN));
	 addInput(createInput<PJ301MCPort>(Vec(290, vspace + 10 + 150), module, TROSC::B_VOL_IN));
	 addInput(createInput<PJ301MCPort>(Vec(290, vspace + 10 + 280), module, TROSC::C_VOL_IN));

	 addInput(createInput<PJ301MCPort>(Vec(215, 50 + 280), module, TROSC::C_WIDTH_INPUT));

	 addOutput(createOutput<PJ301MOPort>(Vec(290, 30), module, TROSC::MIX_OUTPUT));

	 addOutput(createOutput<PJ301MOPort>(Vec(255, 20 + 20), module, TROSC::A_OUTPUT));
	 addOutput(createOutput<PJ301MOPort>(Vec(255, 20 + 150), module, TROSC::B_OUTPUT));
	 addOutput(createOutput<PJ301MOPort>(Vec(255, 20 + 280), module, TROSC::C_OUTPUT));
		}
};


// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.

Model *modelTROSC = createModel<TROSC, TROSCWidget>("TROSC");
