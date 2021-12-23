////////////////////////////////////////////////////////////////////////////
// <Bene Pad - pad for Bene sequencer>
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

struct BenePads : Module {
    enum ParamIds
    {
        ENUMS(BUTTON_PARAM, 16),
        NUM_PARAMS
    };
    enum InputIds
    {
     NUM_INPUTS
    };
	enum OutputIds {
	X_OUT,
  Y_OUT,
  G_OUT,
	NUM_OUTPUTS
  };
  enum LightIds
  {
    ENUMS(PAD_LIGHT, 16),
    NUM_LIGHTS
  };

  dsp::SchmittTrigger button_triggers[4][4];

  int x_position = 0;
  int y_position = 0;

  // Expander
  float consumerMessage[3] = {};// this module must read from here
  float producerMessage[3] = {};// mother will write into here

  int panelTheme;

	BenePads() {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
    for (int i=0; i<16;i++)
    {
      configButton(BUTTON_PARAM + i,"Triggers");
    }

    leftExpander.producerMessage = producerMessage;
    leftExpander.consumerMessage = consumerMessage;
    

    onReset();

		panelTheme = (loadDarkAsDefault() ? 1 : 0);
  }

////////////////////////////


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


////////////////////////////
  void process(const ProcessArgs &args) override
  {
  bool motherPresent = (leftExpander.module && leftExpander.module->model == modelBene);
   if (motherPresent) 
   {
    float *messagesToMother = (float*)leftExpander.module->rightExpander.producerMessage;
    bool shot = false;
    for (int i = 0; i < 4; i++)
    {
      for (int j = 0; j < 4; j++)
      {
        if ((params[BUTTON_PARAM + i + j*4].value))
        {
          lights[PAD_LIGHT+i+j*4].value = 1.0;
          shot = true;
          x_position = i; 
          y_position = j;
          messagesToMother[0] = i + 1;
          messagesToMother[1] = j + 1;
        }
        else
        {
        lights[PAD_LIGHT+i+j*4].value=0.0  ;
        }
        if (shot)
        {
          messagesToMother[2] = 10.0;
        }
        else
        {
          messagesToMother[2] = 0.0;
        }
      }
    }
    leftExpander.module->rightExpander.messageFlipRequested = true;
   }
  }
};

////////////////////////////////

struct BenePadsWidget : ModuleWidget {

/////////////////////////////////


SvgPanel* darkPanel;
struct PanelThemeItem : MenuItem {
  BenePads *module;
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

  BenePads *module = dynamic_cast<BenePads*>(this->module);
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

/////////////////////////////////////
BenePadsWidget(BenePads *module){
  setModule(module);
  setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Light/BenePad.svg")));
  if (module) {
    darkPanel = new SvgPanel();
    darkPanel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Dark/BenePad.svg")));
    darkPanel->visible = false;
    addChild(darkPanel);
  }

   int top = 20;
  int left = 3;
  int column_spacing = 35;
  int row_spacing = 35;
  int button_offset = 20;

  // addOutput(createOutput<PJ301MOrPort>(Vec(130, 20), module, BenePads::X_OUT));
  // addOutput(createOutput<PJ301MOrPort>(Vec(130, 50), module, BenePads::Y_OUT));
  // addOutput(createOutput<PJ301MOrPort>(Vec(130, 80), module, BenePads::G_OUT));

    for (int i = 0; i < 4; i++)
    {
      for ( int j = 0 ; j < 4 ; j++)
      {
        addParam(createLightParam<LEDLightBezel<OrangeLight>>(Vec(2+button_offset + left + column_spacing * i - 10,2+ top + row_spacing * j + 170),module,BenePads::BUTTON_PARAM + i + j * 4, BenePads::PAD_LIGHT + i + j * 4));
      }

    }

  addChild(createWidget<ScrewBlack>(Vec(15, 0)));
  addChild(createWidget<ScrewBlack>(Vec(box.size.x-30, 0)));
  addChild(createWidget<ScrewBlack>(Vec(15, 365)));
  addChild(createWidget<ScrewBlack>(Vec(box.size.x-30, 365)));

}
void step() override {
  if (module) {
    Widget* panel = getPanel();
    panel->visible = ((((BenePads*)module)->panelTheme) == 0);
    darkPanel->visible  = ((((BenePads*)module)->panelTheme) == 1);
  }
  Widget::step();
}
};
Model *modelBenePads = createModel<BenePads, BenePadsWidget>("BenePads");
