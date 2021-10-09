////////////////////////////////////////////////////////////////////////////
// <Harmonic Oscillator>
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

int clampInt(const int _in, const int min = 0, const int max = 7)
{
	if (_in > max)
		return max;
	if (_in < min)
		return min;
	return _in;
}

float triShape(float _in)
{
	_in = _in - round(_in);
	return std::abs(_in + _in);
}

float LERP(const float _amountOfA, const float _inA, const float _inB)
{
	return ((_amountOfA * _inA) + ((1.0f - _amountOfA) * _inB));
}

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
struct sineOsc
{
	bool analog = false;
	bool soft = false;
	bool syncEnabled = false;
	// For optimizing in serial code
	int channels = 0;


	T phase = 0.f;
	T freq;
	T syncDirection = 1.f;

	dsp::TRCFilter<T> sqrFilter;

	dsp::MinBlepGenerator<QUALITY, OVERSAMPLE, T> sinMinBlep;

	T sinValue = 0.f;

	void setPitch(T pitch)
	{
		freq = dsp::FREQ_C4 * simd::pow(2.f, pitch);
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

	T light()
	{
		return simd::sin(2 * T(M_PI) * phase);
	}
};

/////////////////////////////////////////////////////////

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

//////////////////////////////////////////////////////////////////////////////

struct Verbo : Module {
	enum ParamIds {
		SLOPE_PARAM,
		FREQ_PARAM,
		FINE_PARAM,
		CV_PARAM,
		CENTER_PARAM,
		CENTER_CV_PARAM,
		WIDTH_PARAM,
		WIDTH_CV_PARAM,
		FM_PARAM,
		ENUMS(HARM_PARAM, 8),
		NUM_PARAMS
	};
	enum InputIds {
		SLOPE_INPUT,
		PITCH_INPUT,
		CV_INPUT,
		CENTER_INPUT,
		WIDTH_INPUT,
		FM_INPUT,
		ENUMS(HARM_INPUT, 8),
		NUM_INPUTS
	};
	enum OutputIds {
		SIN_OUTPUT,
		TRI_OUTPUT,
		SAW_OUTPUT,
		SQR_OUTPUT,
		ENUMS(HARM_OUTPUT, 8),
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(HARM_LIGHT, 10),
		NUM_LIGHTS
	};

	Oscillator<8, 8, float_4> oscillator[4];
	sineOsc<8, 8, float_4> bank[8] = {};



	float inMults[8] = {};
  float widthTable[9] = {0, 0, 0, 0.285, 0.285, 0.2608, 0.23523, 0.2125, 0.193};

	int panelTheme;

  Verbo() {
	config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

	for (int i = 0; i < 8; i++)
	{
		configParam(HARM_PARAM + i,  0.0, 1.0, 0.0, "Harm Param");
	}
		configParam(FM_PARAM,  0.0, 1.0, 0.0, "Fm Param");
		configParam(CV_PARAM,  -1.0, 1.0, 0.0, "Cv Param");

		configParam(WIDTH_CV_PARAM,  -1.0, 1.0, 0.0, "Width Cv Param");
		configParam(WIDTH_PARAM,  0.0, 7.0, 0.0, "Width Param");

		configParam(SLOPE_PARAM,  0.0, 5.0, 0.0, "Slope Param");

		configParam(CENTER_CV_PARAM,  -1.0, 1.0, 0.0, "Center Cv Param");
		configParam(CENTER_PARAM,  0.0, 7.0, 0.0, "Center Param");

		configParam(FREQ_PARAM,  -54.f, 54.f, 0.f, "Frequency", "Hz", std::pow(2, 1 / 12.f), dsp::FREQ_C4);
		configParam(FINE_PARAM,  -1.f, 1.f, 0.f, "Fine frequency");
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


		float freq = params[FREQ_PARAM].getValue() / 12.f;
		freq += dsp::quadraticBipolar(params[FINE_PARAM].getValue()) * 3.f / 12.f;
		float fmParam = dsp::quadraticBipolar(params[FM_PARAM].getValue());

		int channels = std::max(inputs[PITCH_INPUT].getChannels(), 1);

		for (int c = 0; c < channels; c += 4)
		{
			float_4 pitch;
			oscillator->channels = std::min(channels - c, 4);
			oscillator->analog = 0;
			oscillator->soft = 0;

			pitch = freq;
			pitch += inputs[PITCH_INPUT].getVoltageSimd<float_4>(c);

			if (inputs[FM_INPUT].isConnected())
			{
				pitch += fmParam * inputs[FM_INPUT].getPolyVoltageSimd<float_4>(c);
			}

			oscillator->setPitch(pitch);
			oscillator->process(args.sampleTime,0.f);

		}

	///////////////////////////////////////////
	///////////////////////////////////////////

    		int stages = 8;
    		const float invStages = 1.0f/stages;
    		const float halfStages = stages * 0.5f;
    		const float remainInvStages = 1.0f - invStages;

    		float widthControl = params[WIDTH_PARAM].getValue() + inputs[WIDTH_INPUT].getVoltage()*params[WIDTH_CV_PARAM].getValue();
    		widthControl = clamp(widthControl, 0.0f, 5.0f) * 0.2f;
    		widthControl = widthControl * widthControl * widthTable[stages];

    		float CenterControl = params[CENTER_PARAM].getValue() + inputs[CENTER_INPUT].getVoltage()*params[CENTER_CV_PARAM].getValue();
    		CenterControl = clamp(CenterControl, 0.0f, 5.0f) * 0.2f;

    		float slopeControl = params[SLOPE_PARAM].getValue() + inputs[SLOPE_INPUT].getVoltage();
    		slopeControl = clamp(slopeControl, 0.0f, 5.0f) * 0.2f;

    		float scanFactor1 = LERP(widthControl, halfStages, invStages);
    		float scanFactor2 = LERP(widthControl, halfStages + remainInvStages, 1.0f);
    		float scanFinal = LERP(CenterControl, scanFactor2, scanFactor1);

    		float invWidth = 1.0f/(LERP(widthControl, float(stages), invStages+invStages));

    		float subStage = 0.0f;
    		for(int i = 0; i < 8; i++)
    		{
    		    inMults[i] = (scanFinal + subStage) * invWidth;
    		    subStage = subStage - invStages;
    		}

    		for(int i = 0; i < 8; i++)
    		{
    		    inMults[i] = clamp(inMults[i], 0.0f, 1.0f);
    		    inMults[i] = triShape(inMults[i]);
    		    inMults[i] = clamp(inMults[i], 0.0f, 1.0f);

    		    const float shaped = (2.0f - inMults[i]) * inMults[i];
    		    inMults[i] = LERP(slopeControl, shaped, inMults[i]);
    		}

			float_4 harm_sum=0.0f;
			float_4 harm_out=0.0f;
			float_4 harms[8];

    for(int i = 0; i < 8; i++)
    {
		bank[i].freq=(i+2)*oscillator->freq;
		bank[i].process(args.sampleTime, 0.f);

		harms[i]=(5 * bank[i].sin() * clamp((params[HARM_PARAM + i].getValue() + inputs[HARM_INPUT + i].getVoltage()), 0.0, 1.0));
		harm_out += 0.5 * bank[i].sin() * clamp((params[HARM_PARAM + i].getValue()+ inputs[HARM_INPUT + i].getVoltage()), 0.0f, 1.0f);
		lights[HARM_LIGHT + i].setSmoothBrightness(params[HARM_PARAM + i].getValue() * 1.2,APP->engine->getSampleTime());

		outputs[HARM_OUTPUT + i].setVoltageSimd(harms[i], 0);
	}

	
	for (int i = 0; i < 8; i++)
	{
		harm_sum += bank[i].sin() * inMults[i] * 1.2;
		lights[HARM_LIGHT + i].setSmoothBrightness(std::fmax(0.0, inMults[i]),APP->engine->getSampleTime());
	}

	
	for (int c = 0; c < channels; c += 4)
	{
		outputs[SAW_OUTPUT].setVoltageSimd(5*oscillator->saw(),c);
		outputs[SQR_OUTPUT].setVoltageSimd(5*oscillator->sqr(), c);
		outputs[TRI_OUTPUT].setVoltageSimd(5*oscillator->tri(), c);
		outputs[SIN_OUTPUT].setVoltageSimd(5 * oscillator->sin() + harm_sum + harm_out, c);
	}

		outputs[SAW_OUTPUT].setChannels(channels);
		outputs[SQR_OUTPUT].setChannels(channels);
		outputs[TRI_OUTPUT].setChannels(channels);
		outputs[SIN_OUTPUT].setChannels(channels);
  }

};


struct VerboWidget : ModuleWidget {

	SvgPanel* darkPanel;
	struct PanelThemeItem : MenuItem {
	  Verbo *module;
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

	  Verbo *module = dynamic_cast<Verbo*>(this->module);
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
 VerboWidget(Verbo *module){
	setModule(module);
	setPanel(APP->window->loadSvg(asset::plugin(pluginInstance,"res/Light/Verbo.svg")));
	if (module) {
    darkPanel = new SvgPanel();
    darkPanel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance,"res/Dark/Verbo.svg")));
    darkPanel->visible = false;
    addChild(darkPanel);
  }

	addChild(createWidget<ScrewBlack>(Vec(15, 0)));
	addChild(createWidget<ScrewBlack>(Vec(box.size.x-30, 0)));
	addChild(createWidget<ScrewBlack>(Vec(15, 365)));
	addChild(createWidget<ScrewBlack>(Vec(box.size.x-30, 365)));


	addParam(createParam<VerboL>(Vec(15, 160), module, Verbo::FREQ_PARAM));
	addParam(createParam<Trimpot>(Vec(85, 140), module, Verbo::FINE_PARAM));


	int space=30;
	int left=40;
	for(int i=0; i<8;i++)
	{
		addParam(createParam<SlidePot>(Vec(left+95+space*i, 110), module, Verbo::HARM_PARAM+i));
		addChild(createLight<SmallLight<OrangeLight>>(Vec(left + 95 + space * i, 250), module, Verbo::HARM_LIGHT + i));
	}
		addOutput(createOutput<PJ301MOPort>(Vec(left+90+space*0, 80), module, Verbo::HARM_OUTPUT+0));
		addOutput(createOutput<PJ301MOPort>(Vec(left+90+space*1, 80), module, Verbo::HARM_OUTPUT+1));
		addOutput(createOutput<PJ301MOPort>(Vec(left+90+space*2, 80), module, Verbo::HARM_OUTPUT+2));
		addOutput(createOutput<PJ301MOPort>(Vec(left+90+space*3, 80), module, Verbo::HARM_OUTPUT+3));
		addOutput(createOutput<PJ301MOPort>(Vec(left+90+space*4, 80), module, Verbo::HARM_OUTPUT+4));
		addOutput(createOutput<PJ301MOPort>(Vec(left+90+space*5, 80), module, Verbo::HARM_OUTPUT+5));
		addOutput(createOutput<PJ301MOPort>(Vec(left+90+space*6, 80), module, Verbo::HARM_OUTPUT+6));
		addOutput(createOutput<PJ301MOPort>(Vec(left+90+space*7, 80), module, Verbo::HARM_OUTPUT+7));

		addInput(createInput<PJ301MIPort>(Vec(left+90+space*0, 222),module, Verbo::HARM_INPUT+0));
		addInput(createInput<PJ301MIPort>(Vec(left+90+space*1, 222),module, Verbo::HARM_INPUT+1));
		addInput(createInput<PJ301MIPort>(Vec(left+90+space*2, 222),module, Verbo::HARM_INPUT+2));
		addInput(createInput<PJ301MIPort>(Vec(left+90+space*3, 222),module, Verbo::HARM_INPUT+3));
		addInput(createInput<PJ301MIPort>(Vec(left+90+space*4, 222),module, Verbo::HARM_INPUT+4));
		addInput(createInput<PJ301MIPort>(Vec(left+90+space*5, 222),module, Verbo::HARM_INPUT+5));
		addInput(createInput<PJ301MIPort>(Vec(left+90+space*6, 222),module, Verbo::HARM_INPUT+6));
		addInput(createInput<PJ301MIPort>(Vec(left+90+space*7, 222),module, Verbo::HARM_INPUT+7));


	int ks = 60;
	int vp=20;

		addParam(createParam<VerboS>(Vec(10, vp+272), module, Verbo::FM_PARAM));
		addInput(createInput<PJ301MCPort>(Vec(15, vp+320), module, Verbo::FM_INPUT));
		addParam(createParam<VerboS>(Vec(55, vp+272), module, Verbo::CV_PARAM));
		addInput(createInput<PJ301MCPort>(Vec(60, vp+320),module, Verbo::CV_INPUT));
		addInput(createInput<PJ301MCPort>(Vec(90, vp+320),module, Verbo::PITCH_INPUT));

		addParam(createParam<VerboS>(Vec(30+left+ks, vp+272), module, Verbo::WIDTH_CV_PARAM));
		addParam(createParam<VerboS>(Vec(30+left+ks+space*2, vp+272), module, Verbo::WIDTH_PARAM));

		addParam(createParam<Trimpot>(Vec(30+left+ks*2-15, vp+322.5), module, Verbo::SLOPE_PARAM));
		addInput(createInput<PJ301MCPort>(Vec(30+left+ks*2+25, vp+320),module, Verbo::SLOPE_INPUT));

		addParam(createParam<VerboS>(Vec(30+left+ks*3, vp+272), module, Verbo::CENTER_CV_PARAM));
		addParam(createParam<VerboS>(Vec(30+left+ks*3+space*2, vp+272), module, Verbo::CENTER_PARAM));

		addInput(createInput<PJ301MCPort>(Vec(30+left+ks+5, vp+320),  module, Verbo::WIDTH_INPUT));
		addInput(createInput<PJ301MCPort>(Vec(30+left+ks*3+5, vp+320), module, Verbo::CENTER_INPUT));



		addOutput(createOutput<PJ301MOPort>(Vec(5, 80), module, Verbo::TRI_OUTPUT));
		addOutput(createOutput<PJ301MOPort>(Vec(33, 80),module, Verbo::SQR_OUTPUT));
		addOutput(createOutput<PJ301MOPort>(Vec(61, 80),module, Verbo::SAW_OUTPUT));
		addOutput(createOutput<PJ301MOPort>(Vec(89, 80),module, Verbo::SIN_OUTPUT));


 }
 void step() override {
   if (module) {
	 Widget* panel = getPanel();
     panel->visible = ((((Verbo*)module)->panelTheme) == 0);
     darkPanel->visible  = ((((Verbo*)module)->panelTheme) == 1);
   }
   Widget::step();
 }
};

Model *modelVerbo = createModel<Verbo, VerboWidget>("Verbo");
