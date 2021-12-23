////////////////////////////////////////////////////////////////////////////
// <Transposer>
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
struct Transpose : Module {
	enum ParamIds {
    OCTAVE_SHIFT_1,
    OCTAVE_SHIFT_2,
    SEMITONE_SHIFT_1,
    SEMITONE_SHIFT_2,
    FINE_SHIFT_1,
   // FINE_SHIFT_2,
    NUM_PARAMS
	};
	enum InputIds {
		OCTAVE_SHIFT_1_INPUT,
    OCTAVE_SHIFT_2_INPUT,
    SEMITONE_SHIFT_1_INPUT,
    SEMITONE_SHIFT_2_INPUT,
    OCTAVE_SHIFT_1_CVINPUT,
    OCTAVE_SHIFT_2_CVINPUT,
    SEMITONE_SHIFT_1_CVINPUT,
    SEMITONE_SHIFT_2_CVINPUT,
    FINE_SHIFT_1_INPUT,
  //  FINE_SHIFT_2_INPUT,
    FINE_SHIFT_1_CVINPUT,
  //  FINE_SHIFT_2_CVINPUT,
    NUM_INPUTS
	};
	enum OutputIds {
		OCTAVE_SHIFT_1_OUTPUT,
    OCTAVE_SHIFT_2_OUTPUT,
    SEMITONE_SHIFT_1_OUTPUT,
    SEMITONE_SHIFT_2_OUTPUT,
    FINE_SHIFT_1_OUTPUT,
  //  FINE_SHIFT_2_OUTPUT,
    NUM_OUTPUTS
	};

	Transpose()
  {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS);

    configParam(OCTAVE_SHIFT_1,  -4.5, 4.5, 0.0,"Octave shift");
    configParam(OCTAVE_SHIFT_2,  -4.5, 4.5, 0.0,"Octave shift");
    configParam(SEMITONE_SHIFT_1,  -6.5, 6.5, 0.0,"Semitone shift");
    configParam(SEMITONE_SHIFT_2,  -6.5, 6.5, 0.0,"Semitone shift");
    configParam(FINE_SHIFT_1,  -1, 1, 0.0,"Fine Shift");
   // configParam(FINE_SHIFT_2,  -1, 1, 0.0,"Fine Shift");
	 onReset();

	 panelTheme = (loadDarkAsDefault() ? 1 : 0);

  }

  float octave_1_out = 0.0;
  float octave_2_out = 0.0;
  float semitone_1_out = 0.0;
  float semitone_2_out = 0.0;
  float fine_1_out = 0.0;
  float fine_2_out = 0.0;

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
  octave_1_out = inputs[OCTAVE_SHIFT_1_INPUT].getVoltage() + round(params[OCTAVE_SHIFT_1].getValue()) + round(inputs[OCTAVE_SHIFT_1_CVINPUT].getVoltage()/2);
  octave_2_out = inputs[OCTAVE_SHIFT_2_INPUT].getVoltage() + round(params[OCTAVE_SHIFT_2].getValue()) + round(inputs[OCTAVE_SHIFT_1_CVINPUT].getVoltage()/2);
  semitone_1_out = inputs[SEMITONE_SHIFT_1_INPUT].getVoltage() + round(params[SEMITONE_SHIFT_1].getValue())*(1.0/12.0) + round(inputs[SEMITONE_SHIFT_1_CVINPUT].getVoltage()/2)*(1.0/12.0);
  semitone_2_out = inputs[SEMITONE_SHIFT_2_INPUT].getVoltage() + round(params[SEMITONE_SHIFT_2].getValue())*(1.0/12.0) + round(inputs[SEMITONE_SHIFT_2_CVINPUT].getVoltage()/2)*(1.0/12.0);
  fine_1_out = inputs[FINE_SHIFT_1_INPUT].getVoltage() + (params[FINE_SHIFT_1].getValue())*(1.0/12.0) + (inputs[FINE_SHIFT_1_CVINPUT].getVoltage()/2)*(1.0/2.0);
 // fine_2_out = inputs[FINE_SHIFT_2_INPUT].getVoltage() + (params[FINE_SHIFT_2].getValue())*(1.0/12.0) + (inputs[FINE_SHIFT_2_CVINPUT].getVoltage()/2)*(1.0/2.0);

  outputs[OCTAVE_SHIFT_1_OUTPUT].setVoltage(octave_1_out);
  outputs[OCTAVE_SHIFT_2_OUTPUT].setVoltage(octave_2_out);
  outputs[SEMITONE_SHIFT_1_OUTPUT].setVoltage(semitone_1_out);
  outputs[SEMITONE_SHIFT_2_OUTPUT].setVoltage(semitone_2_out);
  outputs[FINE_SHIFT_1_OUTPUT].setVoltage(fine_1_out);
 // outputs[FINE_SHIFT_2_OUTPUT].setVoltage(fine_2_out);

}
};

//////////////////////////////////////////////////////////////////
struct TransposeWidget : ModuleWidget {


	SvgPanel* darkPanel;
	struct PanelThemeItem : MenuItem {
	  Transpose *module;
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

	  Transpose *module = dynamic_cast<Transpose*>(this->module);
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
TransposeWidget(Transpose *module){
   setModule(module);
   setPanel(APP->window->loadSvg(asset::plugin(pluginInstance,  "res/Light/Transpose.svg")));
	 if (module) {
     darkPanel = new SvgPanel();
     darkPanel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Dark/Transpose.svg")));
     darkPanel->visible = false;
     addChild(darkPanel);
   }

   //Screw

   addChild(createWidget<ScrewBlack>(Vec(15, 0)));
   addChild(createWidget<ScrewBlack>(Vec(15, 365)));

   //

   addParam(createParam<FlatASnap>(Vec(2, 15), module, Transpose::OCTAVE_SHIFT_1));
   addParam(createParam<FlatASnap>(Vec(2, 75), module, Transpose::OCTAVE_SHIFT_2));
   addParam(createParam<FlatASnap>(Vec(2, 135), module, Transpose::SEMITONE_SHIFT_1));
   addParam(createParam<FlatASnap>(Vec(2, 195), module, Transpose::SEMITONE_SHIFT_2));
   addParam(createParam<FlatA>(Vec(2, 255), module, Transpose::FINE_SHIFT_1));
   //addParam(createParam<FlatR>(Vec(2, 315), module, Transpose::FINE_SHIFT_2));

   addInput(createInput<PJ301MIPort>(Vec(3, 2 + 45),  module, Transpose::OCTAVE_SHIFT_1_INPUT));
   addInput(createInput<PJ301MIPort>(Vec(3, 2 + 105), module, Transpose::OCTAVE_SHIFT_2_INPUT));
   addInput(createInput<PJ301MIPort>(Vec(3, 2 + 165), module, Transpose::SEMITONE_SHIFT_1_INPUT));
   addInput(createInput<PJ301MIPort>(Vec(3, 2 + 225), module, Transpose::SEMITONE_SHIFT_2_INPUT));
   addInput(createInput<PJ301MIPort>(Vec(3, 2 + 285), module, Transpose::FINE_SHIFT_1_INPUT));
  // addInput(createInput<PJ301MIPort>(Vec(3, 2 + 345), module, Transpose::FINE_SHIFT_2_INPUT));

   addInput(createInput<PJ301MCPort>(Vec(33, 15),  module, Transpose::OCTAVE_SHIFT_1_CVINPUT));
   addInput(createInput<PJ301MCPort>(Vec(33, 75),  module, Transpose::OCTAVE_SHIFT_2_CVINPUT));
   addInput(createInput<PJ301MCPort>(Vec(33, 135), module, Transpose::SEMITONE_SHIFT_1_CVINPUT));
   addInput(createInput<PJ301MCPort>(Vec(33, 195), module, Transpose::SEMITONE_SHIFT_2_CVINPUT));
   addInput(createInput<PJ301MCPort>(Vec(33, 255), module, Transpose::FINE_SHIFT_1_CVINPUT));
  // addInput(createInput<PJ301MCPort>(Vec(33, 315), module, Transpose::FINE_SHIFT_2_CVINPUT));

   addOutput(createOutput<PJ301MOPort>(Vec(33, 2 + 45), module, Transpose::OCTAVE_SHIFT_1_OUTPUT));
   addOutput(createOutput<PJ301MOPort>(Vec(33, 2 + 105),module, Transpose::OCTAVE_SHIFT_2_OUTPUT));
   addOutput(createOutput<PJ301MOPort>(Vec(33, 2 + 165),module, Transpose::SEMITONE_SHIFT_1_OUTPUT));
   addOutput(createOutput<PJ301MOPort>(Vec(33, 2 + 225),module, Transpose::SEMITONE_SHIFT_2_OUTPUT));
   addOutput(createOutput<PJ301MOPort>(Vec(33, 2 + 285),module, Transpose::FINE_SHIFT_1_OUTPUT));
  // addOutput(createOutput<PJ301MOPort>(Vec(33, 2 + 345),module, Transpose::FINE_SHIFT_2_OUTPUT));

}
void step() override {
  if (module) {
	Widget* panel = getPanel();
    panel->visible = ((((Transpose*)module)->panelTheme) == 0);
    darkPanel->visible  = ((((Transpose*)module)->panelTheme) == 1);
  }
  Widget::step();
}
};
Model *modelTranspose = createModel<Transpose, TransposeWidget>("Transpose");
