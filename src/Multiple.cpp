////////////////////////////////////////////////////////////////////////////
// <Utility Module with attenuators, multiples and mixer>
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


struct Multiple : Module {
	enum ParamIds {
		A1_PARAM,
		A2_PARAM,
		A3_PARAM,
		O1_PARAM,
		O2_PARAM,
		O3_PARAM,
		L1_PARAM,
		L2_PARAM,
		L3_PARAM,
		L4_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		I1_INPUT,
		I2_INPUT,
		I3_INPUT,
		OC1_INPUT,
		OC2_INPUT,
		OC3_INPUT,
		SC1_INPUT,
		SC2_INPUT,
		SC3_INPUT,
		M1_INPUT,
		M2_INPUT,
		M3_INPUT,
		M4_INPUT,

		A_INPUT,
		A1_INPUT,
		B_INPUT,
		B1_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
		I1_OUTPUT,
		I2_OUTPUT,
		I3_OUTPUT,
		M12_OUTPUT,
		M34_OUTPUT,

		A11_OUTPUT,
		A12_OUTPUT,
		B11_OUTPUT,
		B12_OUTPUT,

		A1_OUTPUT,
		A2_OUTPUT,
		A3_OUTPUT,
		B1_OUTPUT,
		B2_OUTPUT,
		B3_OUTPUT,
		NUM_OUTPUTS
	};

	enum LightIds
	{
		A1_POS_LIGHT,
		A1_NEG_LIGHT,
		A2_POS_LIGHT,
		A2_NEG_LIGHT,
		A3_POS_LIGHT,
		A3_NEG_LIGHT,
		NUM_LIGHTS
	};

int panelTheme;

	Multiple() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		configParam(A1_PARAM,  -1.f,1.f,0.1f,"Scale Op");
		configParam(A2_PARAM,  -1.f,1.f,0.1f,"Scale Op");
		configParam(A3_PARAM,  -1.f,1.f,0.1f,"Scale Op");

		configParam(O1_PARAM,  -5.f,5.f,0.f,"Attenuverter 1");
		configParam(O2_PARAM,  -5.f,5.f,0.f,"Attenuverter 2");
		configParam(O3_PARAM,  -5.f,5.f,0.f,"Attenuverter 3");


		configParam(L1_PARAM,  0.f, 1.f, 0.f, "Level 1");
		configParam(L2_PARAM,  0.f, 1.f, 0.f, "Level 2");
		configParam(L3_PARAM,  0.f, 1.f, 0.f, "Level 3");
		configParam(L4_PARAM,  0.f, 1.f, 0.f, "Level 4");
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
	float deltaTime = args.sampleTime;
	//////////////ATTENUVERTERS//////////////////

	float at1_in = inputs[I1_INPUT].getVoltage()*params[A1_PARAM].getValue();
	float at2_in = inputs[I2_INPUT].getVoltage()*params[A2_PARAM].getValue();
	float at3_in = inputs[I3_INPUT].getVoltage()*params[A3_PARAM].getValue();

	outputs[I1_OUTPUT].setVoltage(at1_in);
	outputs[I2_OUTPUT].setVoltage(at2_in);
	outputs[I3_OUTPUT].setVoltage(at3_in);


	if (at1_in > 0)
		lights[A1_POS_LIGHT].setSmoothBrightness(at1_in, deltaTime);
	else
		lights[A1_NEG_LIGHT].setSmoothBrightness(-at1_in, deltaTime);

	if (at2_in > 0)
		lights[A2_POS_LIGHT].setSmoothBrightness(at2_in, deltaTime);
	else
		lights[A2_NEG_LIGHT].setSmoothBrightness(-at2_in, deltaTime);

	if (at3_in > 0)
		lights[A3_POS_LIGHT].setSmoothBrightness(at3_in, deltaTime);
	else
		lights[A3_NEG_LIGHT].setSmoothBrightness(-at3_in, deltaTime);

	//////////////MIXER//////////////////

	float mix1_in = inputs[M1_INPUT].getVoltage()*std::pow(params[L1_PARAM].getValue(),2.f);
	float mix2_in = inputs[M2_INPUT].getVoltage() * std::pow(params[L2_PARAM].getValue(), 2.f);
	float mix3_in = inputs[M3_INPUT].getVoltage() * std::pow(params[L3_PARAM].getValue(), 2.f);
	float mix4_in = inputs[M4_INPUT].getVoltage() * std::pow(params[L4_PARAM].getValue(), 2.f);

	if(outputs[M12_OUTPUT].isConnected()){
	outputs[M12_OUTPUT].setVoltage(mix1_in+mix2_in);
	outputs[M34_OUTPUT].setVoltage(mix3_in+mix4_in);
	}
	else
	{
		outputs[M34_OUTPUT].setVoltage(mix1_in + mix2_in + mix3_in + mix4_in);
	}


	//////////////MULTIPLIER//////////////////



	float m1_in = inputs[A_INPUT].getVoltage();
	float m2_in = inputs[B_INPUT].getVoltage();

	float m11_in = inputs[A1_INPUT].getVoltage();
	float m22_in = inputs[B1_INPUT].getVoltage();

	outputs[A1_OUTPUT].setVoltage(m11_in);
	outputs[A2_OUTPUT].setVoltage(m11_in);
	outputs[A3_OUTPUT].setVoltage(m11_in);

	outputs[B1_OUTPUT].setVoltage(m22_in);
	outputs[B2_OUTPUT].setVoltage(m22_in);
	outputs[B3_OUTPUT].setVoltage(m22_in);

	outputs[A11_OUTPUT].setVoltage(m1_in);
	outputs[A12_OUTPUT].setVoltage(m1_in);

	outputs[B11_OUTPUT].setVoltage(m2_in);
	outputs[B12_OUTPUT].setVoltage(m2_in);
}
};

struct MultipleWidget : ModuleWidget {


	SvgPanel* darkPanel;
	struct PanelThemeItem : MenuItem {
	  Multiple *module;
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

	  Multiple *module = dynamic_cast<Multiple*>(this->module);
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

	MultipleWidget(Multiple *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance,  "res/Light/Multiple.svg")));
		if (module) {
	    darkPanel = new SvgPanel();
	    darkPanel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Dark/Multiple.svg")));
	    darkPanel->visible = false;
	    addChild(darkPanel);
	  }

		addChild(createWidget<ScrewBlack>(Vec(15, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 30, 0)));
		addChild(createWidget<ScrewBlack>(Vec(15, 365)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 30, 365)));
 
////////////////////////////////////////////////////////////////////////////////////////

		addParam(createParam<FlatA>(Vec(10, 20), module, Multiple::A1_PARAM));
		addParam(createParam<FlatA>(Vec(10 + 35, 20), module, Multiple::A2_PARAM));
		addParam(createParam<FlatA>(Vec(10 + 70, 20), module, Multiple::A3_PARAM));

		addParam(createParam<FlatG>(Vec(10, 90), module, Multiple::O1_PARAM));
		addParam(createParam<FlatG>(Vec(10 + 35, 90), module, Multiple::O2_PARAM));
		addParam(createParam<FlatG>(Vec(10 + 70, 90), module, Multiple::O3_PARAM));


		addInput(createInput<PJ301MCPort>(Vec(11, 55), module, Multiple::OC1_INPUT));
		addInput(createInput<PJ301MCPort>(Vec(11+36, 55), module, Multiple::OC2_INPUT));
		addInput(createInput<PJ301MCPort>(Vec(11+72, 55), module, Multiple::OC3_INPUT));

		addInput(createInput<PJ301MCPort>(Vec(11, 125), module, Multiple::SC1_INPUT));
		addInput(createInput<PJ301MCPort>(Vec(11+36, 125), module, Multiple::SC2_INPUT));
		addInput(createInput<PJ301MCPort>(Vec(11+72, 125), module, Multiple::SC3_INPUT));

		// addChild(createLight<SmallLight<GreenRedLight>>(Vec(40, 30), module, Multiple::A1_POS_LIGHT));
		// addChild(createLight<SmallLight<GreenRedLight>>(Vec(40+80, 30), module, Multiple::A2_POS_LIGHT));
		// addChild(createLight<SmallLight<GreenRedLight>>(Vec(40+160, 30), module, Multiple::A3_POS_LIGHT));

		addInput(createInput<PJ301MCPort>(Vec(11, 160), module, Multiple::I1_INPUT));
		addOutput(createOutput<PJ301MOPort>(Vec(11, 190), module, Multiple::I1_OUTPUT));

		addInput(createInput<PJ301MCPort>(Vec(11+36, 160), module, Multiple::I2_INPUT));
		addOutput(createOutput<PJ301MOPort>(Vec(11+36, 190), module, Multiple::I2_OUTPUT));

		addInput(createInput<PJ301MCPort>(Vec(11+72, 160), module, Multiple::I3_INPUT));
		addOutput(createOutput<PJ301MOPort>(Vec(11+72, 190), module, Multiple::I3_OUTPUT));

		/////////////////////////////////////////////////////////////////////////////////////////////////////////

		 //addParam(createParam<RoundWhy>(Vec(13, 175), module, Multiple::L1_PARAM));
		 //addParam(createParam<RoundWhy>(Vec(13 + 60, 175), module, Multiple::L2_PARAM));
		 //addParam(createParam<RoundWhy>(Vec(13 + 120, 175), module, Multiple::L3_PARAM));
		 //addParam(createParam<RoundWhy>(Vec(13 + 180, 175), module, Multiple::L4_PARAM));
 
		 //addInput(createInput<PJ301MIPort>(Vec(11, 230), module, Multiple::M1_INPUT));
		 //addInput(createInput<PJ301MIPort>(Vec(11 + 70, 230), module, Multiple::M2_INPUT));
		 //addInput(createInput<PJ301MIPort>(Vec(130, 230), module, Multiple::M3_INPUT));
		 //addInput(createInput<PJ301MIPort>(Vec(130 + 70, 230), module, Multiple::M4_INPUT));
 
		 //addOutput(createOutput<PJ301MOPort>(Vec(11 + 35, 230), module, Multiple::M12_OUTPUT));
		 //addOutput(createOutput<PJ301MOPort>(Vec(130 + 35, 230), module, Multiple::M34_OUTPUT));

		////////////////////////////////////////////////////////////////////////////////////////////

		//addInput(createInput<PJ301MIPort>(Vec(8, 280), module, Multiple::A_INPUT));
		//addOutput(createOutput<PJ301MOPort>(Vec(8 + 30 * 1, 280), module, Multiple::A11_OUTPUT));
		//addOutput(createOutput<PJ301MOPort>(Vec(8 + 30 * 2, 280), module, Multiple::A12_OUTPUT));

		//addInput(createInput<PJ301MIPort>(Vec(8, 320), module, Multiple::B_INPUT));
		//addOutput(createOutput<PJ301MOPort>(Vec(8 + 30 * 1, 320), module, Multiple::B11_OUTPUT));
		//addOutput(createOutput<PJ301MOPort>(Vec(8 + 30 * 2, 320), module, Multiple::B12_OUTPUT));

		//addInput(createInput<PJ301MIPort>(Vec(115, 280), module, Multiple::A1_INPUT));
		//addOutput(createOutput<PJ301MOPort>(Vec(115 + 30 * 1, 280), module, Multiple::A1_OUTPUT));
		//addOutput(createOutput<PJ301MOPort>(Vec(115 + 30 * 2, 280), module, Multiple::A2_OUTPUT));
		//addOutput(createOutput<PJ301MOPort>(Vec(115 + 30 * 3, 280), module, Multiple::A3_OUTPUT));

		//addInput(createInput<PJ301MIPort>(Vec(115, 320), module, Multiple::B1_INPUT));
		//addOutput(createOutput<PJ301MOPort>(Vec(115 + 30 * 1, 320), module, Multiple::B1_OUTPUT));
		//addOutput(createOutput<PJ301MOPort>(Vec(115 + 30 * 2, 320), module, Multiple::B2_OUTPUT));
		//addOutput(createOutput<PJ301MOPort>(Vec(115 + 30 * 3, 320), module, Multiple::B3_OUTPUT));
}
void step() override {
  if (module) {
	Widget* panel = getPanel();
    panel->visible = ((((Multiple*)module)->panelTheme) == 0);
    darkPanel->visible  = ((((Multiple*)module)->panelTheme) == 1);
  }
  Widget::step();
}
};
Model *modelMultiple = createModel<Multiple, MultipleWidget>("Multiple");
