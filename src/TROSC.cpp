
#include "plugin.hpp"

extern float sawTable[2048];
extern float triTable[2048];

template <int OVERSAMPLE, int QUALITY>
struct VoltageControlledOscillator
{
	bool analog = false;
	bool soft = false;
	float lastSyncValue = 0.0f;
	float phase = 0.0f;
	float freq;
	float pw = 0.5f;
	float pitch;
	bool syncEnabled = false;
	bool syncDirection = false;

	dsp::Decimator<OVERSAMPLE, QUALITY> triDecimator;
	dsp::Decimator<OVERSAMPLE, QUALITY> sinDecimator;
	dsp::Decimator<OVERSAMPLE, QUALITY> sawDecimator;
	dsp::Decimator<OVERSAMPLE, QUALITY> sqrDecimator;
	dsp::RCFilter sqrFilter;

	// For analog detuning effect
	float pitchSlew = 0.0f;
	int pitchSlewIndex = 0;

	float sinBuffer[OVERSAMPLE] = {};
	float triBuffer[OVERSAMPLE] = {};
	float sawBuffer[OVERSAMPLE] = {};
	float sqrBuffer[OVERSAMPLE] = {};

	void setPitch(float pitchKnob, float pitchCv)
	{
		// Compute frequency
		pitch = pitchKnob;
		if (analog)
		{
			// Apply pitch slew
			const float pitchSlewAmount = 3.0f;
			pitch += pitchSlew * pitchSlewAmount;
		}
		else
		{
			// Quantize coarse knob if digital mode
			pitch = std::round(pitch);
		}
		pitch += pitchCv;
		// Note C4
		freq = dsp::FREQ_C4 * std::pow(2.f, pitch / 12.f);
	}
	void setPulseWidth(float pulseWidth)
	{
		const float pwMin = 0.01f;
		pw = clamp(pulseWidth, pwMin, 1.0f - pwMin);
	}

	void process(float deltaTime, float syncValue)
	{
		if (analog)
		{
			// Adjust pitch slew
			if (++pitchSlewIndex > 32)
			{
				const float pitchSlewTau = 100.0f; // Time constant for leaky integrator in seconds
				pitchSlew += (random::normal() - pitchSlew / pitchSlewTau) * APP->engine->getSampleTime();
				pitchSlewIndex = 0;
			}
		}

		// Advance phase
		float deltaPhase = clamp(freq * deltaTime, 1e-6, 0.5f);

		// Detect sync
		int syncIndex = -1;		   // Index in the oversample loop where sync occurs [0, OVERSAMPLE)
		float syncCrossing = 0.0f; // Offset that sync occurs [0.0f, 1.0f)
		if (syncEnabled)
		{
			syncValue -= 0.01f;
			if (syncValue > 0.0f && lastSyncValue <= 0.0f)
			{
				float deltaSync = syncValue - lastSyncValue;
				syncCrossing = 1.0f - syncValue / deltaSync;
				syncCrossing *= OVERSAMPLE;
				syncIndex = (int)syncCrossing;
				syncCrossing -= syncIndex;
			}
			lastSyncValue = syncValue;
		}

		if (syncDirection)
			deltaPhase *= -1.0f;

		sqrFilter.setCutoff(40.0f * deltaTime);

		for (int i = 0; i < OVERSAMPLE; i++)
		{
			if (syncIndex == i)
			{
				if (soft)
				{
					syncDirection = !syncDirection;
					deltaPhase *= -1.0f;
				}
				else
				{
					// phase = syncCrossing * deltaPhase / OVERSAMPLE;
					phase = 0.0f;
				}
			}

			if (analog)
			{
				// Quadratic approximation of sine, slightly richer harmonics
				if (phase < 0.5f)
					sinBuffer[i] = 1.f - 16.f * std::pow(phase - 0.25f, 2);
				else
					sinBuffer[i] = -1.f + 16.f * std::pow(phase - 0.75f, 2);
				sinBuffer[i] *= 1.08f;
			}
			else
			{
				sinBuffer[i] = std::sin(2.f * M_PI * phase);
			}
			if (analog)
			{
				triBuffer[i] = 1.25f * interpolateLinear(triTable, phase * 2047.f);
			}
			else
			{
				if (phase < 0.25f)
					triBuffer[i] = 4.f * phase;
				else if (phase < 0.75f)
					triBuffer[i] = 2.f - 4.f * phase;
				else
					triBuffer[i] = -4.f + 4.f * phase;
			}
			if (analog)
			{
				sawBuffer[i] = 1.66f * interpolateLinear(sawTable, phase * 2047.f);
			}
			else
			{
				if (phase < 0.5f)
					sawBuffer[i] = 2.f * phase;
				else
					sawBuffer[i] = -2.f + 2.f * phase;
			}
			sqrBuffer[i] = (phase < pw) ? 1.f : -1.f;
			if (analog)
			{
				// Simply filter here
				sqrFilter.process(sqrBuffer[i]);
				sqrBuffer[i] = 0.71f * sqrFilter.highpass();
			}

			// Advance phase
			phase += deltaPhase / OVERSAMPLE;
			phase = eucMod(phase, 1.0f);
		}
	}

	float sin()
	{
		return sinDecimator.process(sinBuffer);
	}
	float tri()
	{
		return triDecimator.process(triBuffer);
	}
	float saw()
	{
		return sawDecimator.process(sawBuffer);
	}
	float sqr()
	{
		return sqrDecimator.process(sqrBuffer);
	}
	float light()
	{
		return std::sin(2 * M_PI * phase);
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

	VoltageControlledOscillator<8, 8> a_osc;
	VoltageControlledOscillator<8, 8> b_osc;
	VoltageControlledOscillator<8, 8> c_osc;

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

	float a_pitchCv = 0.0;
	float b_pitchCv = 0.0;
	float c_pitchCv = 0.0;

	a_osc.analog = params[MODE_A_PARAM].value > 0.0f;
	a_osc.soft = params[SYNC_A_PARAM].value <= 0.0f;

	b_osc.analog = params[MODE_B_PARAM].value > 0.0f;
	b_osc.soft = params[SYNC_B_PARAM].value <= 0.0f;

	c_osc.analog = params[MODE_C_PARAM].value > 0.0f;
	c_osc.soft = params[SYNC_C_PARAM].value <= 0.0f;

	float a_pitchFine = 3.0f * dsp::quadraticBipolar(params[FINE_A_PARAM].value);
	a_pitchCv = 12.0f * inputs[PITCH_A_INPUT].value;

	float b_pitchFine = 3.0f * dsp::quadraticBipolar(params[FINE_B_PARAM].value);
	if(params[LINK_A_PARAM].value==1)
	b_pitchCv = 12.0f * inputs[PITCH_B_INPUT].value;
	else
	b_pitchCv = a_pitchCv ;

	float c_pitchFine = 3.0f * dsp::quadraticBipolar(params[FINE_C_PARAM].value);
	if (params[LINK_B_PARAM].value == 1)
	c_pitchCv = 12.0f * inputs[PITCH_C_INPUT].value;
	else
	c_pitchCv = b_pitchCv;



	if (inputs[FM_A_INPUT].isConnected())
	{
		a_pitchCv += dsp::quadraticBipolar(params[FM_A_PARAM].value) * 12.0f * inputs[FM_A_INPUT].value;
	}
	a_osc.setPitch(params[FREQ_A_PARAM].value, a_pitchFine + a_pitchCv);
	a_osc.syncEnabled = inputs[SYNC_A_INPUT].isConnected();

	if (inputs[FM_B_INPUT].isConnected())
	{
		b_pitchCv += dsp::quadraticBipolar(params[FM_B_PARAM].value) * 12.0f * inputs[FM_B_INPUT].value;
	}
	b_osc.setPitch(params[FREQ_B_PARAM].value, b_pitchFine + b_pitchCv);
	b_osc.syncEnabled = inputs[SYNC_B_INPUT].isConnected();

	if (inputs[FM_C_INPUT].isConnected())
	{
		c_pitchCv += dsp::quadraticBipolar(params[FM_C_PARAM].value) * 12.0f * inputs[FM_C_INPUT].value;
	}
	c_osc.setPitch(params[FREQ_C_PARAM].value, c_pitchFine + c_pitchCv);
	c_osc.setPulseWidth(params[C_WIDTH_PARAM].value + params[C_WIDTH_PARAM].value * inputs[C_WIDTH_INPUT].value / 10.0f);
	c_osc.syncEnabled = inputs[SYNC_C_INPUT].isConnected();



	a_osc.process(APP->engine->getSampleTime(), inputs[SYNC_A_INPUT].value);
	b_osc.process(APP->engine->getSampleTime(), inputs[SYNC_B_INPUT].value);
	c_osc.process(APP->engine->getSampleTime(), inputs[SYNC_C_INPUT].value);

	// Set output
	float wave_a = clamp(params[WAVE_A_MIX].value, 0.0f, 1.0f);
	float wave2_a = clamp(params[WAVE2_A_MIX].value, 0.0f, 1.0f);
	float mix_a = clamp ( params[WAVE_A_SEL_PARAM].value,0.f,1.f);
		if(inputs[A_WAVE_MIX_INPUT].isConnected())
			mix_a *= clamp(inputs[A_WAVE_MIX_INPUT].value / 10.f, 0.f, 1.f);

	float wave_b = clamp(params[WAVE_B_MIX].value, 0.0f, 1.0f);
	float wave2_b = clamp(params[WAVE2_B_MIX].value, 0.0f, 1.0f);
	float mix_b = clamp(params[WAVE_B_SEL_PARAM].value,0.f,1.f);
		if(inputs[B_WAVE_MIX_INPUT].isConnected())
			mix_b *= clamp(inputs[B_WAVE_MIX_INPUT].value / 10.f, 0.f, 1.f);

	float wave_c = clamp(params[WAVE_C_MIX].value, 0.0f, 1.0f);
	float mix_c = clamp(params[WAVE_C_SEL_PARAM].value,0.f,1.f);
		if(inputs[C_WAVE_MIX_INPUT].isConnected())
			mix_c *= clamp(inputs[C_WAVE_MIX_INPUT].value / 10.f, 0.f, 1.f);

	float out_a;
	float out2_a;
	float a_out;

	float out_b;
	float out2_b;
	float b_out;

	float out_c;
	float out2_c;
	float c_out;

	float mixa,mixb,mixc;

	out_a =  crossfade(a_osc.sin(), a_osc.tri(), wave_a);
	out2_a = crossfade(a_osc.saw(), a_osc.sqr(), wave2_a);
	a_out =  crossfade(out_a, out2_a, mix_a);

	out_b =  crossfade(b_osc.sin(), b_osc.tri(), wave_b);
	out2_b = crossfade(b_osc.saw(), b_osc.sqr(), wave2_b);
	b_out =  crossfade(out_b, out2_b, mix_b);

	out_c = crossfade(c_osc.sin(), c_osc.tri(), wave_c);
	out2_c =c_osc.sqr();
	c_out = crossfade(out_c, out2_c, mix_c);

	mixa = 5*(a_out*params[LEVEL_A_PARAM].value);
	if(inputs[A_VOL_IN].isConnected()) 
	 mixa*= clamp(inputs[A_VOL_IN].value /10.f , -1.0f, 1.0f);
	outputs[A_OUTPUT].value = mixa;

	mixb = 5*(b_out*params[LEVEL_B_PARAM].value);
	if(inputs[B_VOL_IN].isConnected()) 
	 mixb*= clamp(inputs[B_VOL_IN].value /10.f , -1.0f, 1.0f);
	outputs[B_OUTPUT].value = mixb;

	mixc = 5*(c_out*params[LEVEL_C_PARAM].value);
	if(inputs[C_VOL_IN].isConnected()) 
	 mixc*= clamp(inputs[C_VOL_IN].value /10.f , -1.0f, 1.0f);
	outputs[C_OUTPUT].value = mixc;

	outputs[MIX_OUTPUT].value = clamp((mixa + mixb + mixc)/5,-5.f,5.f);
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
