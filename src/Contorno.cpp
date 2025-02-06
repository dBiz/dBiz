////////////////////////////////////////////////////////////////////////////
// <Contorno is a simple 4 X envelope generator>
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

inline float ifelse(bool cond, float a, float b) {
	return cond ? a : b;
}

static float shapeDelta(float delta, float tau, float shape) {
	float lin = sgn(delta) * 10.f / tau;
	if (shape < 0.f) {
		float log = sgn(delta) * 40.f / tau / (std::fabs(delta) + 1.f);
		return crossfade(lin, log, -shape * 0.95f);
	}
	else {
		float exp = M_E * delta / tau;
		return crossfade(lin, exp, shape * 0.90f);
	}
}

struct Contorno : Module {
	enum ParamIds {
		ENUMS(RANGE_PARAM, 4),
		ENUMS(TRIGG_PARAM, 4),
		ENUMS(CYCLE_PARAM, 4),
		ENUMS(SHAPE_PARAM, 4),
		ENUMS(RISE_PARAM, 4),
		ENUMS(FALL_PARAM, 4),
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(TRIGG_INPUT, 4),
		ENUMS(CYCLE_INPUT, 4),
		ENUMS(RISE_INPUT, 4),
		ENUMS(FALL_INPUT, 4),
		ENUMS(IN_INPUT, 4),
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(OUT_OUTPUT, 4),
		NUM_OUTPUTS
	};

	enum LightIds
	{
		ENUMS(TRIGG_LIGHT, 4),
		ENUMS(CYCLE_LIGHT, 4),
		ENUMS(RISE_LIGHT, 4),
		ENUMS(FALL_LIGHT, 4),
		NUM_LIGHTS
	};

	float out[4] {};
	float gate[4] = {};

	int panelTheme;

	dsp::SchmittTrigger trigger[4];
	dsp::SchmittTrigger cycle[4];
	bool cycleState[4];
	dsp::PulseGenerator endOfCyclePulse[4];

	Contorno()
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		for (int i = 0; i < 4; i++)
		{
			configSwitch(RANGE_PARAM +i , 0.0, 2.0, 0.0, "Ch range", {"Medium", "Fast", "Slow"});
			configButton(TRIGG_PARAM + i ,string::f("Channel %d trig", i + 1));
			configSwitch(CYCLE_PARAM +i , 0.0, 1.0, 0.0, string::f("Channel %d cycle", i + 1));
			configParam(SHAPE_PARAM + i,  -1.0, 1.0, 0.0, string::f("Channel %d shape", i + 1));
			configParam(RISE_PARAM + i,  0.0, 1.0, 0.0, string::f("Channel %d rise", i + 1));
			configParam(FALL_PARAM + i,  0.0, 1.0, 0.0, string::f("Channel %d fall", i + 1));
			
			configInput(TRIGG_INPUT + i, string::f("Channel %d trig cv", i + 1));
			configInput(CYCLE_INPUT + i, string::f("Channel %d cycle cv", i + 1));
			configInput(RISE_INPUT + i, string::f("Channel %d rise mod", i + 1));
			configInput(FALL_INPUT + i, string::f("Channel %d fall mod", i + 1));
			configInput(IN_INPUT + i, string::f("Channel %d", i + 1));
			
			configOutput(OUT_OUTPUT + i, string::f("Channel %d", i + 1));
		}

		onReset();
		
		panelTheme = (loadDarkAsDefault() ? 1 : 0);

	}

	void process(const ProcessArgs &args) override
	{

	for (int c=0;c<4;c++)
	{
		if(cycle[c].process(params[CYCLE_PARAM+c].getValue()+inputs[CYCLE_INPUT+c].getVoltage()))
		{
			cycleState[c]=!cycleState[c];
		}
			lights[CYCLE_LIGHT+c].setBrightness(cycleState[c] ? 1.0 : 0.0);
		
		

		float in = inputs[IN_INPUT + c].getVoltage();

		if (trigger[c].process(params[TRIGG_PARAM + c].getValue() * 10.0 + inputs[TRIGG_INPUT + c].getVoltage()/2.0)) {
			gate[c] = true;
		}
				if (gate[c]) {
			in = 10.0;
		}

		float shape = params[SHAPE_PARAM + c].getValue();
		float delta = in - out[c];

		// Integrator
		float minTime;
		switch ((int) params[RANGE_PARAM + c].getValue()) {
			case 0: minTime = 1e-2; break;
			case 1: minTime = 1e-3; break;
			default: minTime = 1e-1; break;
		}

		bool rising = false;
		bool falling = false;

		if (delta > 0) {
			// Rise
			float riseCv = params[RISE_PARAM + c].getValue() + inputs[RISE_INPUT + c].getVoltage() / 10.0;
			riseCv = clamp(riseCv, 0.0, 1.0);
			float rise = minTime * powf(2.0, riseCv * 10.0);
			out[c] += shapeDelta(delta, rise, shape) * args.sampleTime;
			rising = (in - out[c] > 1e-3);
			if (!rising) {
				gate[c] = false;
			}
		}
		else if (delta < 0) {
			// Fall
			float fallCv = params[FALL_PARAM + c].getValue() + inputs[FALL_INPUT + c].getVoltage() / 10.0;
			fallCv = clamp(fallCv, 0.0, 1.0);
			float fall = minTime * powf(2.0, fallCv * 10.0);
			out[c] += shapeDelta(delta, fall, shape) * args.sampleTime;
			falling = (in - out[c] < -1e-3);
			if (!falling) {
				// End of cycle, check if we should turn the gate back on (cycle mode)
				if (cycleState[c]) {
					gate[c] = true;
				}
			}
		}
		else {
			gate[c] = false;
			//lights[CYCLE_LIGHT+c].setBrightness(0.0);
		}

		if (!rising && !falling) {
			out[c] = in;
		}

		//if (params[CYCLE_PARAM + c].getValue() == 1.0 || inputs[CYCLE_INPUT+c].getVoltage()>0.0) lights[CYCLE_LIGHT + c].setBrightness(1.0);

		lights[RISE_LIGHT + c].setSmoothBrightness(rising ? 1.0 : 0.0, args.sampleTime);
		lights[FALL_LIGHT + c].setSmoothBrightness(falling ? 1.0 : 0.0, args.sampleTime);
		outputs[OUT_OUTPUT + c].setVoltage(out[c]);
		}

	}

	void onReset() override
	{
		for (int i = 0; i < 4; i++)
		{
			cycleState[i] = false;
		}
	}

	json_t *dataToJson() override
	{
		json_t *rootJ = json_object();
		// cycle states
		json_t *cycleStatesJ = json_array();
		for (int i = 0; i < 4; i++)
		{
			json_t *cycleStateJ = json_boolean(cycleState[i]);
			json_array_append_new(cycleStatesJ, cycleStateJ);
		}

		json_object_set_new(rootJ, "cycles", cycleStatesJ);

		// panelTheme
		json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));
		return rootJ;
	}

	void dataFromJson(json_t *rootJ) override
	{
		// cycle states
		json_t *cycleStatesJ = json_object_get(rootJ, "cycles");
		if (cycleStatesJ)
		{
			for (int i = 0; i < 4; i++)
			{
				json_t *cycleStateJ = json_array_get(cycleStatesJ, i);
				if (cycleStateJ)
					cycleState[i] = json_boolean_value(cycleStateJ);
			}
		}
		// panelTheme
		json_t *panelThemeJ = json_object_get(rootJ, "panelTheme");
		if (panelThemeJ)
			panelTheme = json_integer_value(panelThemeJ);
	}
};



struct ContornoWidget : ModuleWidget {


    int lastPanelTheme = -1;
	std::shared_ptr<window::Svg> light_svg;
	std::shared_ptr<window::Svg> dark_svg;
	
	struct PanelThemeItem : MenuItem {
	  Contorno *module;
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

	  Contorno *module = dynamic_cast<Contorno*>(this->module);
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

	ContornoWidget(Contorno *module){
	 setModule(module);
	 // Main panels from Inkscape
 		light_svg = APP->window->loadSvg(asset::plugin(pluginInstance, "res/Light/Contorno.svg"));
		dark_svg = APP->window->loadSvg(asset::plugin(pluginInstance, "res/Dark/Contorno.svg"));
		int panelTheme = isDark(module ? (&(((Contorno*)module)->panelTheme)) : NULL) ? 1 : 0;// need this here since step() not called for module browser
		setPanel(panelTheme == 0 ? light_svg : dark_svg);	

	 addChild(createWidget<ScrewBlack>(Vec(15, 0)));
	 addChild(createWidget<ScrewBlack>(Vec(box.size.x - 30, 0)));
	 addChild(createWidget<ScrewBlack>(Vec(15, 365)));
	 addChild(createWidget<ScrewBlack>(Vec(box.size.x - 30, 365)));

	 int space = 64;

	 for (int i = 0; i < 4; i++)
	 {

		 addParam(createParam<MCKSSS>(Vec(space * i + 52, 25), module, Contorno::RANGE_PARAM + i));


		 addParam(createLightParam<LEDLightBezel<BlueLight>>(Vec(space * i + 7, 297), module, Contorno::CYCLE_PARAM + i,Contorno::CYCLE_LIGHT + i));
	

		 addParam(createParam<RoundWhy>(Vec(space * i + 12.5, 39), module, ::Contorno::SHAPE_PARAM + i));

		 addParam(createParam<SlidePot>(Vec(space * i + 10, 100), module, ::Contorno::RISE_PARAM + i));
		 addParam(createParam<SlidePot>(Vec(space * i + 40, 100), module, ::Contorno::FALL_PARAM + i));


		 addParam(createParam<BPush>(Vec(space * i + 5, 255), module, ::Contorno::TRIGG_PARAM + i));

		 addChild(createLight<SmallLight<OrangeLight>>(Vec(space * i + 15, 212), module, Contorno::RISE_LIGHT + i));
		 addChild(createLight<SmallLight<OrangeLight>>(Vec(space * i + 45, 212), module, Contorno::FALL_LIGHT + i));
	  }

		 addOutput(createOutput<PJ301MSPort>(Vec(space * 0 + 35, 335), module, ::Contorno::OUT_OUTPUT + 0));
		 addOutput(createOutput<PJ301MSPort>(Vec(space * 1 + 35, 335), module, ::Contorno::OUT_OUTPUT + 1));
		 addOutput(createOutput<PJ301MSPort>(Vec(space * 2 + 35, 335), module, ::Contorno::OUT_OUTPUT + 2));
		 addOutput(createOutput<PJ301MSPort>(Vec(space * 3 + 35, 335), module, ::Contorno::OUT_OUTPUT + 3));

		 addInput(createInput<PJ301MSPort>(Vec(35 + space * 0, 294), module, ::Contorno::CYCLE_INPUT + 0));
		 addInput(createInput<PJ301MSPort>(Vec(35 + space * 1, 294), module, ::Contorno::CYCLE_INPUT + 1));
		 addInput(createInput<PJ301MSPort>(Vec(35 + space * 2, 294), module, ::Contorno::CYCLE_INPUT + 2));
		 addInput(createInput<PJ301MSPort>(Vec(35 + space * 3, 294), module, ::Contorno::CYCLE_INPUT + 3));


		 addInput(createInput<PJ301MSPort>(Vec(space * 0 + 35, 220), module, ::Contorno::FALL_INPUT + 0));
		 addInput(createInput<PJ301MSPort>(Vec(space * 1 + 35, 220), module, ::Contorno::FALL_INPUT + 1));
		 addInput(createInput<PJ301MSPort>(Vec(space * 2 + 35, 220), module, ::Contorno::FALL_INPUT + 2));
		 addInput(createInput<PJ301MSPort>(Vec(space * 3 + 35, 220), module, ::Contorno::FALL_INPUT + 3));

		 addInput(createInput<PJ301MSPort>(Vec(space * 0 + 5, 220), module, ::Contorno::RISE_INPUT + 0));
		 addInput(createInput<PJ301MSPort>(Vec(space * 1 + 5, 220), module, ::Contorno::RISE_INPUT + 1));
		 addInput(createInput<PJ301MSPort>(Vec(space * 2 + 5, 220), module, ::Contorno::RISE_INPUT + 2));
		 addInput(createInput<PJ301MSPort>(Vec(space * 3 + 5, 220), module, ::Contorno::RISE_INPUT + 3));

		 addInput(createInput<PJ301MSPort>(Vec(space * 0 + 35, 255), module, ::Contorno::TRIGG_INPUT + 0));
		 addInput(createInput<PJ301MSPort>(Vec(space * 1 + 35, 255), module, ::Contorno::TRIGG_INPUT + 1));
		 addInput(createInput<PJ301MSPort>(Vec(space * 2 + 35, 255), module, ::Contorno::TRIGG_INPUT + 2));
		 addInput(createInput<PJ301MSPort>(Vec(space * 3 + 35, 255), module, ::Contorno::TRIGG_INPUT + 3));

		 addInput(createInput<PJ301MSPort>(Vec(space * 0 + 5, 335), module, ::Contorno::IN_INPUT + 0));
		 addInput(createInput<PJ301MSPort>(Vec(space * 1 + 5, 335), module, ::Contorno::IN_INPUT + 1));
		 addInput(createInput<PJ301MSPort>(Vec(space * 2 + 5, 335), module, ::Contorno::IN_INPUT + 2));
		 addInput(createInput<PJ301MSPort>(Vec(space * 3 + 5, 335), module, ::Contorno::IN_INPUT + 3));


}
void step() override {
		int panelTheme = isDark(module ? (&(((Contorno*)module)->panelTheme)) : NULL) ? 1 : 0;
		if (lastPanelTheme != panelTheme) {
			lastPanelTheme = panelTheme;
			SvgPanel* panel = (SvgPanel*)getPanel();
			panel->setBackground(panelTheme == 0 ? light_svg : dark_svg);
		}
		Widget::step();
	}
};
Model *modelContorno = createModel<Contorno, ContornoWidget>("Contorno");
