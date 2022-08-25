////////////////////////////////////////////////////////////////////////////
// <Sub harmonic OSC>
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
struct subBank
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

	dsp::MinBlepGenerator<QUALITY, OVERSAMPLE, T> sawMinBlep;

	T sawValue = 0.f;

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

		// Saw
		sawValue = saw(phase);
		sawValue += sawMinBlep.process();

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

	T light()
	{
		return simd::sin(2 * T(M_PI) * phase);
	}
};

struct SuHa : Module {
	enum ParamIds
	{
		SUM_VOL_PARAM,
		ENUMS(VCO_PARAM, 2),
		ENUMS(VCO_OCT_PARAM, 2),
		ENUMS(SUB1_PARAM, 2),
		ENUMS(SUB2_PARAM, 2),
		ENUMS(VCO_VOL_PARAM, 2),
		ENUMS(SUB1_VOL_PARAM, 2),
		ENUMS(SUB2_VOL_PARAM, 2),
		NUM_PARAMS
	};
	enum InputIds
	{
		ENUMS(VCO_INPUT, 2),
		ENUMS(SUB1_INPUT, 2),
		ENUMS(SUB2_INPUT, 2),
		NUM_INPUTS
	};
	enum OutputIds
	{
		SUM_OUTPUT,
		ENUMS(VCO_OUTPUT, 2),
		ENUMS(SUB1_OUTPUT, 2),
		ENUMS(SUB2_OUTPUT, 2),
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	subBank <16,16,float_4> VCO[2]={};
	subBank <16,16,float_4> SUB1[2]={};
	subBank <16,16,float_4> SUB2[2]={};

	int panelTheme;


	SuHa()
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		configParam(SUM_VOL_PARAM,  0.0, 1.0, 0.0,"VOLUME");

		for(int i=0;i<2;i++)
		{
			configParam(VCO_PARAM+i,  -54.0, 54.0, 0.0,"Freq");
			configParam(VCO_OCT_PARAM + i, -3.0, 2.0, 0.0, "Octave Select");
			configParam(SUB1_PARAM+i,  1.0, 15.0, 1.0,"Sub1");
			configParam(SUB2_PARAM+i,  1.0, 15.0, 1.0,"Sub2");
			configParam(VCO_VOL_PARAM+i,  0.0, 1.0, 0.0,"Main Vol");
			configParam(SUB1_VOL_PARAM+i,  0.0, 1.0, 0.0,"Sub 1 Vol");
			configParam(SUB2_VOL_PARAM+i,  0.0, 1.0, 0.0,"Sub 2 Vol");
			onReset();

			panelTheme = (loadDarkAsDefault() ? 1 : 0);
		}
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

		int s1[2]={};
		int s2[2] = {};
		float_4 sum=0.0f;

		float_4 pitch[2];
		float freq[2];
		float octave[2];

		for (int i=0;i<2;i++)
		{

		octave[i]=round(params[VCO_OCT_PARAM+i].getValue());
		freq[i]=params[VCO_PARAM+i].getValue()/12.f;
		pitch[i]=freq[i]+octave[i];
		pitch[i]+=inputs[VCO_INPUT + i].getVoltageSimd<float_4>(0);


		s1[i] = round(params[SUB1_PARAM+i].getValue() + clamp(inputs[SUB1_INPUT+i].getVoltage(), -15.0f, 15.0f));
		if (s1[i]>15) s1[i]=15;
		if (s1[i]<=1) s1[i]=1;

		s2[i] = round(params[SUB2_PARAM + i].getValue() + clamp(inputs[SUB2_INPUT + i].getVoltage(), -15.0f, 15.0f));
		if (s2[i]>15) s2[i]=15;
		if (s2[i]<=1) s2[i]=1;

		VCO[i].setPitch(pitch[i]);
		SUB1[i].freq=VCO[i].freq/s1[i];
		SUB2[i].freq=VCO[i].freq/s2[i];

		VCO[i].process(args.sampleTime, 0.f);
		SUB1[i].process(args.sampleTime, 0.f);
		SUB2[i].process(args.sampleTime, 0.f);

		outputs[VCO_OUTPUT + i].setVoltageSimd(2.0f * VCO[i].saw()* params[VCO_VOL_PARAM + i].getValue(),0);
		outputs[SUB1_OUTPUT + i].setVoltageSimd(2.0f * SUB1[i].saw() * params[SUB1_VOL_PARAM + i].getValue(),0);
		outputs[SUB2_OUTPUT + i].setVoltageSimd(2.0f * SUB2[i].saw() * params[SUB2_VOL_PARAM + i].getValue(),0);
		}

		for (int i = 0; i < 2; i++)
		{
			sum += clamp(outputs[VCO_OUTPUT + i].getVoltage() + outputs[SUB1_OUTPUT + i].getVoltage() + outputs[SUB2_OUTPUT + i].getVoltage(), -5.0f, 5.0f);
		}


		outputs[SUM_OUTPUT].setVoltageSimd(sum*params[SUM_VOL_PARAM].getValue(),0);

	}
};


struct SuHaWidget : ModuleWidget {
	
	int lastPanelTheme = -1;
	std::shared_ptr<window::Svg> light_svg;
	std::shared_ptr<window::Svg> dark_svg;
	
	struct PanelThemeItem : MenuItem {
	  SuHa *module;
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

	  SuHa *module = dynamic_cast<SuHa*>(this->module);
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
	SuHaWidget(SuHa *module){
		setModule(module);
		// Main panels from Inkscape
 		light_svg = APP->window->loadSvg(asset::plugin(pluginInstance, "res/Light/SuHa.svg"));
		dark_svg = APP->window->loadSvg(asset::plugin(pluginInstance, "res/Dark/SuHa.svg"));
		int panelTheme = isDark(module ? (&(((SuHa*)module)->panelTheme)) : NULL) ? 1 : 0;// need this here since step() not called for module browser
		setPanel(panelTheme == 0 ? light_svg : dark_svg);	

		int KS=44;
		int JS = 32;
		float Side=7.5;

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));


		///////////////////////////////////////////////////////////////////////////////////

		for (int i = 0; i < 2; i++)
		{

			addParam(createParam<DKnob>(Vec(Side + 25, 47 + i * 100), module, SuHa::VCO_PARAM + i));
			addParam(createParam<SDKnobSnap>(Vec(Side + 5, 82 + i * 100), module, SuHa::VCO_OCT_PARAM + i));
			addParam(createParam<DKnob>(Vec(Side + 25 + 40, 47 +i*100), module, SuHa::SUB1_PARAM +i));
			addParam(createParam<DKnob>(Vec(Side + 25 + 2 * 40, 47 +i*100), module, SuHa::SUB2_PARAM +i));


			addParam(createParam<Trim>(Vec(Side + 38, 20 + i*34*3), module, SuHa::VCO_VOL_PARAM +i));
			addParam(createParam<Trim>(Vec(Side + 38 + 40, 20 + i*34*3), module, SuHa::SUB1_VOL_PARAM +i));
			addParam(createParam<Trim>(Vec(Side + 38 + 2 * 40, 20 + i*34*3), module, SuHa::SUB2_VOL_PARAM +i));

		}
			addInput(createInput<PJ301MVAPort>(Vec(Side + 17, 240+0*JS),  module, SuHa::VCO_INPUT +0));
			addInput(createInput<PJ301MVAPort>(Vec(Side + 17, 240+1*JS),  module, SuHa::VCO_INPUT +1));

			addInput(createInput<PJ301MVAPort>(Vec(Side + 17 + KS, 240+0*JS),  module, SuHa::SUB1_INPUT +0));
			addInput(createInput<PJ301MVAPort>(Vec(Side + 17 + KS, 240+1*JS),  module, SuHa::SUB1_INPUT +1));

			addInput(createInput<PJ301MVAPort>(Vec(Side + 17 + 2 * KS, 240+0*JS),  module, SuHa::SUB2_INPUT +0));
			addInput(createInput<PJ301MVAPort>(Vec(Side + 17 + 2 * KS, 240+1*JS),  module, SuHa::SUB2_INPUT +1));


			addOutput(createOutput<PJ301MVAPort>(Vec(Side + 17, 240 + 2 * JS+0*JS),  module, SuHa::VCO_OUTPUT +0));
			addOutput(createOutput<PJ301MVAPort>(Vec(Side + 17, 240 + 2 * JS+1*JS),  module, SuHa::VCO_OUTPUT +1));

			addOutput(createOutput<PJ301MVAPort>(Vec(Side + 17 + KS, 240 + 2 * JS+0*JS),  module, SuHa::SUB1_OUTPUT +0));
			addOutput(createOutput<PJ301MVAPort>(Vec(Side + 17 + KS, 240 + 2 * JS+1*JS),  module, SuHa::SUB1_OUTPUT +1));

			addOutput(createOutput<PJ301MVAPort>(Vec(Side + 17 + 2 * KS, 240 + 2 * JS+0*JS),  module, SuHa::SUB2_OUTPUT +0));
			addOutput(createOutput<PJ301MVAPort>(Vec(Side + 17 + 2 * KS, 240 + 2 * JS+1*JS),  module, SuHa::SUB2_OUTPUT +1));


			addParam(createParam<SDKnob>(Vec(Side + 90, 202), module, SuHa::SUM_VOL_PARAM));
			addOutput(createOutput<PJ301MVAPort>(Vec(Side + 60, 205),  module, SuHa::SUM_OUTPUT));


			//////////////////////////////////////////////////////////////////////////////////////////////////////
	}
	void step() override {
		int panelTheme = isDark(module ? (&(((SuHa*)module)->panelTheme)) : NULL) ? 1 : 0;
		if (lastPanelTheme != panelTheme) {
			lastPanelTheme = panelTheme;
			SvgPanel* panel = (SvgPanel*)getPanel();
			panel->setBackground(panelTheme == 0 ? light_svg : dark_svg);
		}
		Widget::step();
	}
};


// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelSuHa = createModel<SuHa, SuHaWidget>("SuHa");
