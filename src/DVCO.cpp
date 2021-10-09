////////////////////////////////////////////////////////////////////////////
// <Dual Digital Oscillator>
// Copyright (C) <2019>  <Giovanni Ghisleni>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
/////////////////////////////////////////////////////////////////////////////

#include "plugin.hpp"

using simd::float_4;

// Accurate only on [0, 1]
template <typename T>
T sin2pi_pade_05_7_6(T x) {
	x -= 0.5f;
	return (T(-6.28319) * x + T(35.353) * simd::pow(x, 3) - T(44.9043) * simd::pow(x, 5) + T(16.0951) * simd::pow(x, 7))
		/ (1 + T(0.953136) * simd::pow(x, 2) + T(0.430238) * simd::pow(x, 4) + T(0.0981408) * simd::pow(x, 6));
}

template <typename T>
T sin2pi_pade_05_5_4(T x) {
	x -= 0.5f;
	return (T(-6.283185307) * x + T(33.19863968) * simd::pow(x, 3) - T(32.44191367) * simd::pow(x, 5))
		/ (1 + T(1.296008659) * simd::pow(x, 2) + T(0.7028072946) * simd::pow(x, 4));
}

template <typename T>
T expCurve(T x) {
	return (3 + x * (-13 + 5 * x)) / (3 + 2 * x);
}


template <int OVERSAMPLE, int QUALITY, typename T>
struct Ocillator {
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

	void setPitch(T pitch) {
		freq = dsp::FREQ_C4 * simd::pow(2.f, pitch);
	}
	void setPulseWidth(T pulseWidth) {
		const float pwMin = 0.01f;
		this->pulseWidth = simd::clamp(pulseWidth, pwMin, 1.f - pwMin);
	}

	void process(float deltaTime, T syncValue) {
		// Advance phase
		T deltaPhase = simd::clamp(freq * deltaTime, 1e-6f, 0.35f);
		if (soft) {
			// Reverse direction
			deltaPhase *= syncDirection;
		}
		else {
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
		if (wrapMask) {
			for (int i = 0; i < channels; i++) {
				if (wrapMask & (1 << i)) {
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
		if (pulseMask) {
			for (int i = 0; i < channels; i++) {
				if (pulseMask & (1 << i)) {
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
		if (halfMask) {
			for (int i = 0; i < channels; i++) {
				if (halfMask & (1 << i)) {
					T mask = simd::movemaskInverse<T>(1 << i);
					float p = halfCrossing[i] - 1.f;
					T x = mask & (-2.f * syncDirection);
					sawMinBlep.insertDiscontinuity(p, x);
				}
			}
		}

		// Detect sync
		// Might be NAN or outside of [0, 1) range
		if (syncEnabled) {
			T syncCrossing = -lastSyncValue / (syncValue - lastSyncValue);
			lastSyncValue = syncValue;
			T sync = (0.f < syncCrossing) & (syncCrossing <= 1.f);
			int syncMask = simd::movemask(sync);
			if (syncMask) {
				if (soft) {
					syncDirection = simd::ifelse(sync, -syncDirection, syncDirection);
				}
				else {
					T newPhase = simd::ifelse(sync, syncCrossing * deltaPhase, phase);
					// Insert minBLEP for sync
					for (int i = 0; i < channels; i++) {
						if (syncMask & (1 << i)) {
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

		if (analog) {
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

	T sin(T phase) {
		T v;
		if (analog) {
			// Quadratic approximation of sine, slightly richer harmonics
			T halfPhase = (phase < 0.5f);
			T x = phase - simd::ifelse(halfPhase, 0.25f, 0.75f);
			v = 1.f - 16.f * simd::pow(x, 2);
			v *= simd::ifelse(halfPhase, 1.f, -1.f);
		}
		else {
			v = sin2pi_pade_05_5_4(phase);
			// v = sin2pi_pade_05_7_6(phase);
			// v = simd::sin(2 * T(M_PI) * phase);
		}
		return v;
	}
	T sin() {
		return sinValue;
	}

	T tri(T phase) {
		T v;
		if (analog) {
			T x = phase + 0.25f;
			x -= simd::trunc(x);
			T halfX = (x < 0.5f);
			x = 2 * x - simd::ifelse(halfX, 0.f, 1.f);
			v = expCurve(x) * simd::ifelse(halfX, 1.f, -1.f);
		}
		else {
			v = 1 - 4 * simd::fmin(simd::fabs(phase - 0.25f), simd::fabs(phase - 1.25f));
		}
		return v;
	}
	T tri() {
		return triValue;
	}

	T saw(T phase) {
		T v;
		T x = phase + 0.5f;
		x -= simd::trunc(x);
		if (analog) {
			v = -expCurve(x);
		}
		else {
			v = 2 * x - 1;
		}
		return v;
	}
	T saw() {
		return sawValue;
	}

	T sqr(T phase) {
		T v = simd::ifelse(phase < pulseWidth, 1.f, -1.f);
		return v;
	}
	T sqr() {
		return sqrValue;
	}

	T light() {
		return simd::sin(2 * T(M_PI) * phase);
	}
};

struct DVCO : Module {
	enum ParamIds {
		MODE_A_PARAM,
		MODE_B_PARAM,
		SYNC_A_PARAM,
		SYNC_B_PARAM,
		FREQ_A_PARAM,
		FREQ_B_PARAM,
		FINE_A_PARAM,
		FINE_B_PARAM,
		FM_A_PARAM,
		FM2_A_PARAM,
		FM_B_PARAM,
		FM2_B_PARAM,
		PW_A_PARAM,
		PW_B_PARAM,
		PWM_A_PARAM,
		PWM_B_PARAM,
		WAVE_A_PARAM,
		WAVE_B_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		PITCH_A_INPUT,
		PITCH_B_INPUT,
		FM_A_INPUT,
		FM2_A_INPUT,
		FM_B_INPUT,
		FM2_B_INPUT,
		SYNC_A_INPUT,
		SYNC_B_INPUT,
		PW_A_INPUT,
		PW_B_INPUT,
		WAVE_A_INPUT,
		WAVE_B_INPUT,
		CARRIER_INPUT,
		MODULATOR_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OSC_A_OUTPUT,
		OSC_AN_OUTPUT,
		OSC_B_OUTPUT,
		OSC_BN_OUTPUT,
		RING_OUTPUT,
		SUM_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	Ocillator<8, 8, float_4> oscillator_a[4];
	Ocillator<8, 8, float_4> oscillator_b[4];

	int panelTheme;

	DVCO() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		configParam(MODE_A_PARAM,  0.0, 1.0, 1.0,"Analog mode A" );
		configParam(MODE_B_PARAM,  0.0, 1.0, 1.0,"Analog mode B" );
		configParam(SYNC_A_PARAM,  0.0, 1.0, 1.0,"Hard sync A" );
		configParam(SYNC_B_PARAM,  0.0, 1.0, 1.0,"Hard sync B" );
		configParam(FREQ_A_PARAM,  -54.f, 54.f, 0.f, "Osc1 Frequency", " Hz", dsp::FREQ_SEMITONE, dsp::FREQ_C4);
		configParam(FREQ_B_PARAM,  -54.f, 54.f, 0.f, "Osc2 Frequency", " Hz", dsp::FREQ_SEMITONE, dsp::FREQ_C4);
		configParam(FINE_A_PARAM,  -1.f, 1.f, 0.f, "Osc1 Fine frequency");
		configParam(FINE_B_PARAM,  -1.f, 1.f, 0.f, "Osc2 Fine frequency");
		configParam(FM_A_PARAM,  0.f, 1.f, 0.f, "Osc1 Frequency modulation", "%", 0.f, 100.f);
		configParam(FM2_A_PARAM,  0.f, 1.f, 0.f, "Osc1 Frequency modulation 2", "%", 0.f, 100.f);
		configParam(FM_B_PARAM,  0.f, 1.f, 0.f, "Osc2 Frequency modulation", "%", 0.f, 100.f);
		configParam(FM2_B_PARAM,  0.f, 1.f, 0.f, "Osc2 Frequency modulation 2", "%", 0.f, 100.f);
		configParam(PW_A_PARAM,  0.0, 1.0, 0.5,"Osc1 Pulse width", "%", 0.f, 100.f);
		configParam(PW_B_PARAM,  0.0, 1.0, 0.5,"Osc2 Pulse width", "%", 0.f, 100.f);
		configParam(PWM_A_PARAM,  0.0, 1.0, 0.0,"Osc1 Pulse width modulation", "%", 0.f, 100.f);
		configParam(PWM_B_PARAM,  0.0, 1.0, 0.0,"Osc2 Pulse width modulation", "%", 0.f, 100.f);
		configParam(WAVE_A_PARAM,  0.0, 3.0, 1.5,"Wave1 Sel");
		configParam(WAVE_B_PARAM,  0.0, 3.0, 1.5,"Wave2 Sel");
		onReset();

		panelTheme = (loadDarkAsDefault() ? 1 : 0);
	}

	  json_t *dataToJson() override {
	    json_t *rootJ = json_object();

	    // panelTheme
	    json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));
	    return rootJ;
	    }
	    void dataFromJson(json_t *rootJ) override {
	      // panelTheme
	      json_t *panelThemeJ = json_object_get(rootJ, "panelTheme");
	      if (panelThemeJ)
	        panelTheme = json_integer_value(panelThemeJ);
	    }

	void process(const ProcessArgs &args) override
 	{

		//////////////////////////////////////////////////////////////////////////

		float freqParamA = params[FREQ_A_PARAM].getValue() / 12.f;
		freqParamA += dsp::quadraticBipolar(params[FINE_A_PARAM].getValue()) * 3.f / 12.f;
		float fmParamA = dsp::quadraticBipolar(params[FM_A_PARAM].getValue());
		float fmParamA2 = dsp::quadraticBipolar(params[FM2_A_PARAM].getValue());
		float waveParamA = params[WAVE_A_PARAM].getValue();

		float freqParamB = params[FREQ_B_PARAM].getValue() / 12.f;
		freqParamB += dsp::quadraticBipolar(params[FINE_B_PARAM].getValue()) * 3.f / 12.f;
		float fmParamB = dsp::quadraticBipolar(params[FM_B_PARAM].getValue());
		float fmParamB2 = dsp::quadraticBipolar(params[FM2_B_PARAM].getValue());
		float waveParamB = params[WAVE_B_PARAM].getValue();

		int channelsA = std::max(inputs[PITCH_A_INPUT].getChannels(), 1);
		int channelsB = std::max(inputs[PITCH_B_INPUT].getChannels(), 1);

		// int Cchannels = std::max(inputs[CARRIER_INPUT].getChannels(), 1);
		// int Mchannels = std::max(inputs[MODULATOR_INPUT].getChannels(), 1);

		float_4 va=0.f;
		float_4 vb=0.f;
		float_4 sum=0.f;


		for (int c = 0; c < channelsA; c += 4)
		{
			float_4 pitchA;
		//	auto *oscillator = &oscillator_a[c / 4];
			oscillator_a->channels = std::min(channelsA - c, 4);
			oscillator_a->analog = params[MODE_A_PARAM].getValue() > 0.f;
			oscillator_a->soft = params[SYNC_A_PARAM].getValue() <= 0.f;


			pitchA = freqParamA;
			pitchA += inputs[PITCH_A_INPUT].getVoltageSimd<float_4>(c);

			if (inputs[FM_A_INPUT].isConnected()) {
				pitchA += fmParamA * inputs[FM_A_INPUT].getPolyVoltageSimd<float_4>(c);
			}
			if (inputs[FM2_A_INPUT].isConnected()) {
				pitchA += fmParamA2 * inputs[FM2_A_INPUT].getPolyVoltageSimd<float_4>(c);
			}

			oscillator_a->setPitch(pitchA);
			oscillator_a->setPulseWidth(params[PW_A_PARAM].getValue() + params[PWM_A_PARAM].getValue() * inputs[PW_A_INPUT].getPolyVoltageSimd<float_4>(c) / 10.f);

			oscillator_a->syncEnabled = inputs[SYNC_A_INPUT].isConnected();
			oscillator_a->process(args.sampleTime, inputs[SYNC_A_INPUT].getPolyVoltageSimd<float_4>(c));

			// if (outputs[OSC_A_OUTPUT].isConnected()||outputs[OSC_AN_OUTPUT].isConnected()||outputs[SUM_OUTPUT].isConnected())
			// {
				float_4 waveA = simd::clamp(waveParamA + inputs[WAVE_A_INPUT].getPolyVoltageSimd<float_4>(c) / 10.f * 3.f, 0.f, 3.f);
				va += oscillator_a->sin() * simd::fmax(0.f, 1.f - simd::fabs(waveA - 0.f));
				va += oscillator_a->tri() * simd::fmax(0.f, 1.f - simd::fabs(waveA - 1.f));
				va += oscillator_a->saw() * simd::fmax(0.f, 1.f - simd::fabs(waveA - 2.f));
				va += oscillator_a->sqr() * simd::fmax(0.f, 1.f - simd::fabs(waveA - 3.f));

				outputs[OSC_A_OUTPUT].setVoltageSimd(5.f * va, c);
				outputs[OSC_AN_OUTPUT].setVoltageSimd(5.f * va *-1.f, c);
			// }
				sum+=va;
		}


		for (int c = 0; c < channelsB; c += 4)
		{
		//	auto *oscillator = &oscillator_b[c / 4];
			oscillator_b->channels = std::min(channelsB - c, 4);
			oscillator_b->analog = params[MODE_B_PARAM].getValue() > 0.f;
			oscillator_b->soft = params[SYNC_B_PARAM].getValue() <= 0.f;

			float_4 pitchB = freqParamB;
			pitchB += inputs[PITCH_B_INPUT].getVoltageSimd<float_4>(c);

			if (inputs[FM_B_INPUT].isConnected()) {
				pitchB += fmParamB * inputs[FM_B_INPUT].getPolyVoltageSimd<float_4>(c);
			}
			if (inputs[FM2_B_INPUT].isConnected()) {
				pitchB += fmParamB2 * inputs[FM2_B_INPUT].getPolyVoltageSimd<float_4>(c);
			}

			oscillator_b->setPitch(pitchB);
			oscillator_b->setPulseWidth(params[PW_B_PARAM].getValue() + params[PWM_B_PARAM].getValue() * inputs[PW_B_INPUT].getPolyVoltageSimd<float_4>(c) / 10.f);

			oscillator_b->syncEnabled = inputs[SYNC_B_INPUT].isConnected();
			oscillator_b->process(args.sampleTime, outputs[OSC_A_OUTPUT].getPolyVoltageSimd<float_4>(c));

		 	// if (outputs[OSC_B_OUTPUT].isConnected()||outputs[OSC_BN_OUTPUT].isConnected()||outputs[SUM_OUTPUT].isConnected())
			//  {
				float_4 waveB = simd::clamp(waveParamB + inputs[WAVE_B_INPUT].getPolyVoltageSimd<float_4>(c) / 10.f * 3.f, 0.f, 3.f);
				vb += oscillator_b->sin() * simd::fmax(0.f, 1.f - simd::fabs(waveB - 0.f));
				vb += oscillator_b->tri() * simd::fmax(0.f, 1.f - simd::fabs(waveB - 1.f));
				vb += oscillator_b->saw() * simd::fmax(0.f, 1.f - simd::fabs(waveB - 2.f));
				vb += oscillator_b->sqr() * simd::fmax(0.f, 1.f - simd::fabs(waveB - 3.f));

				outputs[OSC_B_OUTPUT].setVoltageSimd(5.f * vb, c);
				outputs[OSC_BN_OUTPUT].setVoltageSimd(5.f * vb*-1.f, c);
		 	//}
				sum+=vb;

		}

		outputs[OSC_A_OUTPUT].setChannels(channelsA);
		outputs[OSC_B_OUTPUT].setChannels(channelsB);

		outputs[OSC_AN_OUTPUT].setChannels(channelsA);
		outputs[OSC_BN_OUTPUT].setChannels(channelsB);

		for (int c = 0; c < channelsB+channelsA; c += 8)
		{
			if(outputs[SUM_OUTPUT].isConnected())
			outputs[SUM_OUTPUT].setVoltageSimd(2.5f * sum, c);

			if(outputs[RING_OUTPUT].isConnected())
			{
				if(inputs[CARRIER_INPUT].isConnected()) va=inputs[CARRIER_INPUT].getPolyVoltageSimd<float_4>(c)/10.f;
				if(inputs[MODULATOR_INPUT].isConnected()) vb=inputs[MODULATOR_INPUT].getPolyVoltageSimd<float_4>(c)/10.f;

				outputs[RING_OUTPUT].setVoltageSimd(5*va*vb,c);
			}
		}
		outputs[SUM_OUTPUT].setChannels(channelsA+channelsB);
		outputs[RING_OUTPUT].setChannels(channelsA+channelsB);

}

};


struct DVCOWidget : ModuleWidget
{


	SvgPanel* darkPanel;
	struct PanelThemeItem : MenuItem {
	  DVCO *module;
	  int theme;
	  void onAction(const event::Action &e) override {
	    module->panelTheme = theme;
	  }
	  void step() override {
	    rightText = (module->panelTheme == theme) ? "✔" : "";
	  }
	};
	void appendContextMenu(Menu *menu) override {
	  MenuLabel *spacerLabel = new MenuLabel();
	  menu->addChild(spacerLabel);

	  DVCO *module = dynamic_cast<DVCO*>(this->module);
	  assert(module);

	  MenuLabel *themeLabel = new MenuLabel();
	  themeLabel->text = "Panel Theme";
	  menu->addChild(themeLabel);

	  PanelThemeItem *lightItem = new PanelThemeItem();
	  lightItem->text = lightPanelID;
	  lightItem->module = module;
	  lightItem->theme = 0;
	  menu->addChild(lightItem);

	  PanelThemeItem *darkItem = new PanelThemeItem();
	  darkItem->text = darkPanelID;
	  darkItem->module = module;
	  darkItem->theme = 1;
	  menu->addChild(darkItem);

	  menu->addChild(createMenuItem<DarkDefaultItem>("Dark as default", CHECKMARK(loadDarkAsDefault())));
	}

DVCOWidget(DVCO *module)
{
	setModule(module);
	setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Light/DVCO.svg")));
	if (module) {
    darkPanel = new SvgPanel();
    darkPanel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Dark/DVCO.svg")));
    darkPanel->visible = false;
    addChild(darkPanel);
  }

	float mid = (15*13)/2;
	int jacks = 27;
	int knobs = 38;
	int border=10;



	addChild(createWidget<ScrewBlack>(Vec(15, 0)));
	addChild(createWidget<ScrewBlack>(Vec(box.size.x-30, 0)));
	addChild(createWidget<ScrewBlack>(Vec(15, 365)));
	addChild(createWidget<ScrewBlack>(Vec(box.size.x-30, 365)));


///////////////////////////////////////port//////////////////////////////////////////////////

	addParam(createParam<MCKSSS2>(Vec(border, 260), module, DVCO::MODE_A_PARAM));
	addParam(createParam<MCKSSS2>(Vec(border+jacks, 260), module, DVCO::SYNC_A_PARAM));
	addParam(createParam<MCKSSS2>(Vec(box.size.x - 26, 260), module, DVCO::MODE_B_PARAM));
	addParam(createParam<MCKSSS2>(Vec(box.size.x - jacks - 26, 260), module, DVCO::SYNC_B_PARAM));

	///////////////////////////////////////params////////////////////////////////////////


	addParam(createParam<LRoundWhy>(Vec(10, 40), module, DVCO::FREQ_A_PARAM));
	addParam(createParam<RoundWhy>(Vec(15+knobs, 15), module, DVCO::FINE_A_PARAM));
	addParam(createParam<LRoundWhy>(Vec(box.size.x -45-10, 40), module, DVCO::FREQ_B_PARAM));
	addParam(createParam<RoundWhy>(Vec(box.size.x-90, 15), module, DVCO::FINE_B_PARAM));
	addParam(createParam<RoundAzz>(Vec(15, 110), module, DVCO::PW_A_PARAM));
	addParam(createParam<RoundWhy>(Vec(15+knobs+5, 60), module, DVCO::FM_A_PARAM));
	addParam(createParam<RoundWhy>(Vec(15+knobs+5, 100), module, DVCO::FM2_A_PARAM));


	addParam(createParam<RoundAzz>(Vec(5, 160), module, DVCO::PWM_A_PARAM));
	addParam(createParam<RoundAzz>(Vec(box.size.x - knobs-15, 110), module, DVCO::PW_B_PARAM));
	addParam(createParam<RoundWhy>(Vec(box.size.x - (knobs*2)-15-5, 60), module, DVCO::FM_B_PARAM));
	addParam(createParam<RoundWhy>(Vec(box.size.x - (knobs*2)-15-5, 100), module, DVCO::FM2_B_PARAM));


	addParam(createParam<RoundAzz>(Vec(box.size.x - knobs-5, 160), module, DVCO::PWM_B_PARAM));
	addParam(createParam<RoundRed>(Vec(15+knobs, 150), module, DVCO::WAVE_A_PARAM));
	addParam(createParam<RoundRed>(Vec(box.size.x - (knobs * 2) - 15 , 150), module, DVCO::WAVE_B_PARAM));

	////////////////////////////////jacks//////////////////////////////////////////////////////////


	addInput(createInput<PJ301MCPort>(Vec(border-5, 290),module, DVCO::PITCH_A_INPUT));
	addInput(createInput<PJ301MCPort>(Vec(border-5+jacks, 290),module, DVCO::FM_A_INPUT));
	addInput(createInput<PJ301MCPort>(Vec(border-5 + jacks*2, 290),module, DVCO::FM2_A_INPUT));
	addInput(createInput<PJ301MCPort>(Vec(border-5 + jacks*2, 325),module, DVCO::WAVE_A_INPUT));
	addInput(createInput<PJ301MCPort>(Vec(border-5+jacks, 325),module, DVCO::SYNC_A_INPUT));
	addInput(createInput<PJ301MCPort>(Vec(border-5, 325),module, DVCO::PW_A_INPUT));

	addInput(createInput<PJ301MCPort>(Vec(box.size.x-8-jacks, 290),module, DVCO::PITCH_B_INPUT));
	addInput(createInput<PJ301MCPort>(Vec(box.size.x-8-jacks*2, 290),module, DVCO::FM_B_INPUT));
	addInput(createInput<PJ301MCPort>(Vec(box.size.x-8-jacks*3, 290),module, DVCO::FM2_B_INPUT));
	addInput(createInput<PJ301MCPort>(Vec(box.size.x-8-jacks*3, 325),module, DVCO::WAVE_B_INPUT));
	addInput(createInput<PJ301MCPort>(Vec(box.size.x-(jacks*2)-8, 325),module, DVCO::SYNC_B_INPUT));
	addInput(createInput<PJ301MCPort>(Vec(box.size.x-jacks-8, 325),module, DVCO::PW_B_INPUT));

	addInput(createInput<PJ301MOrPort>(Vec(mid-jacks-11,245),module, DVCO::CARRIER_INPUT));
	addInput(createInput<PJ301MOrPort>(Vec(mid+10, 245),module, DVCO::MODULATOR_INPUT));

//////////////////////////////OUTPUTS////////////////////////////////////////////////////////////////

	addOutput(createOutput<PJ301MOPort>(Vec(border-4, 225), module, DVCO::OSC_A_OUTPUT));
	addOutput(createOutput<PJ301MOPort>(Vec(border-4+jacks, 225), module, DVCO::OSC_AN_OUTPUT));
	addOutput(createOutput<PJ301MOPort>(Vec(box.size.x-jacks-8 , 225), module, DVCO::OSC_B_OUTPUT));
	addOutput(createOutput<PJ301MOPort>(Vec(box.size.x-jacks*2-8 , 225), module, DVCO::OSC_BN_OUTPUT));
	addOutput(createOutput<PJ301MOPort>(Vec(mid-14, 255), module, DVCO::SUM_OUTPUT));
	addOutput(createOutput<PJ301MOPort>(Vec(mid-14, 225), module, DVCO::RING_OUTPUT));


}
void step() override {
  if (module) {
	Widget* panel = getPanel();
    panel->visible = ((((DVCO*)module)->panelTheme) == 0);
    darkPanel->visible  = ((((DVCO*)module)->panelTheme) == 1);
  }
  Widget::step();
}
};
Model *modelDVCO = createModel<DVCO, DVCOWidget>("DVCO");
