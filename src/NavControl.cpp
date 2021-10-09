////////////////////////////////////////////////////////////////////////////
// <NavControlr>
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


/////added fine out /////////////////////////////////////////////////
struct NavControl : Module {
	enum ParamIds {
    ATTENUVERTER,
    FADER,
    NUM_PARAMS
	};
	enum InputIds {
	ATTENUVERTER_INPUT,
    FADER_INPUT,
    NUM_INPUTS
	};
	enum OutputIds {
	ATTENUVERTER_OUTPUT,
    FADER_OUTPUT,
    NUM_OUTPUTS
	};

	float att_in = 0.0;
    float fader_in = 0.0;
    float att_out = 0.0;
    float fader_out = 0.0;

	NavControl()
  {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS);

    configParam(ATTENUVERTER,  -5.0, 5.0 , 0.0,"Attenuverter Value");
    configParam(FADER,  0.0, 1.0, 0.0,"Fader Value");
    
	onReset();

	panelTheme = (loadDarkAsDefault() ? 1 : 0);

  }


	int panelTheme;

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
     
    if(inputs[ATTENUVERTER_INPUT].isConnected() /*|| inputs[FADER_INPUT].isConnected()*/)
    {
		att_in = inputs[ATTENUVERTER_INPUT].getVoltage();
	}
	else
	{
		att_in = 5.0f;
	}
	if(inputs[FADER_INPUT].isConnected())
    {
		fader_in = inputs[FADER_INPUT].getVoltage();
	}
	else
	{
		fader_in = 5.0f;
	}
	
	fader_out=fader_in*params[FADER].getValue();
    att_out=att_in+params[ATTENUVERTER].getValue();

 	
	outputs[FADER_OUTPUT].setVoltage(fader_out);
	if(outputs[FADER_OUTPUT].isConnected())
	{
		outputs[ATTENUVERTER_OUTPUT].setVoltage(att_out);
	}
	else
	outputs[ATTENUVERTER_OUTPUT].setVoltage(att_out+fader_out);
	

  }
};

//////////////////////////////////////////////////////////////////
struct NavControlWidget : ModuleWidget {


	SvgPanel* darkPanel;
	struct PanelThemeItem : MenuItem {
	  NavControl *module;
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

	  NavControl *module = dynamic_cast<NavControl*>(this->module);
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
NavControlWidget(NavControl *module){
   setModule(module);
   setPanel(APP->window->loadSvg(asset::plugin(pluginInstance,  "res/Light/NavControl.svg")));
	 if (module) {
     darkPanel = new SvgPanel();
     darkPanel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Dark/NavControl.svg")));
     darkPanel->visible = false;
     addChild(darkPanel);
   }

   //Screw

   addChild(createWidget<ScrewBlack>(Vec(15, 0)));
   addChild(createWidget<ScrewBlack>(Vec(15, 365)));

   //

   addParam(createParam<MicroBlu>(Vec(17.5, 115), module, NavControl::ATTENUVERTER));
   addParam(createParam<SlidePotL>(Vec(22.5, 170), module, NavControl::FADER));

   addInput(createInput<PJ301MIPort>(Vec(2, 12 + 10),  module, NavControl::ATTENUVERTER_INPUT));
   addInput(createInput<PJ301MIPort>(Vec(2, 12 + 55), module, NavControl::FADER_INPUT));
   

   addOutput(createOutput<PJ301MOPort>(Vec(31, 12 + 10), module, NavControl::ATTENUVERTER_OUTPUT));
   addOutput(createOutput<PJ301MOPort>(Vec(31, 12 + 55),module, NavControl::FADER_OUTPUT));


}
void step() override {
  if (module) {
	Widget* panel = getPanel();
    panel->visible = ((((NavControl*)module)->panelTheme) == 0);
    darkPanel->visible  = ((((NavControl*)module)->panelTheme) == 1);
  }
  Widget::step();
}
};
Model *modelNavControl = createModel<NavControl, NavControlWidget>("NavControl");
