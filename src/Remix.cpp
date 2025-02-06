////////////////////////////////////////////////////////////////////////////
// <Channel scanner>
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


struct Remix : Module {
    enum ParamIds
    {
        SCAN_PARAM,
        CV_SCAN_PARAM,
        WIDTH_PARAM,
        CV_WIDTH_PARAM,
        LEVEL_PARAM,
        SLOPE_PARAM,
        CV_LEVEL_PARAM,
        CH1_LEVEL_PARAM,
        CH2_LEVEL_PARAM,
        CH3_LEVEL_PARAM,
        CH4_LEVEL_PARAM,
        CH5_LEVEL_PARAM,
        CH6_LEVEL_PARAM,
        NUM_PARAMS
    };
    enum InputIds
    {
        CH1_INPUT,
        CH2_INPUT,
        CH3_INPUT,
        CH4_INPUT,
        CH5_INPUT,
        CH6_INPUT,
        CH1_CV,
        CH2_CV,
        CH3_CV,
        CH4_CV,
        CH5_CV,
        CH6_CV,
        SLOPE_INPUT,
        SCAN_INPUT,
        WIDTH_INPUT,
        LEVEL_INPUT,
        NUM_INPUTS
    };
    enum OutputIds
    {
        A_OUTPUT,
        B_OUTPUT,
        C_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds
    {
        ENUMS(CH_LIGHT, 8),

        NUM_LIGHTS
    };

    float ins[6] = {};
    float outs[6] = {};
    float inMults[6] = {};
    float widthTable[7] = {0.0, 0.285, 0.285, 0.2608, 0.23523, 0.2125, 0.193};

    int panelTheme;

    Remix(){
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        configParam(SCAN_PARAM,  0.0, 5.0,0.0,"Scan Param");
        configParam(CV_SCAN_PARAM,  0.0, 1.0, 0.0,"Scan Cv");
        configParam(WIDTH_PARAM,  0.0, 5.0,0.0,"Width");
        configParam(CV_WIDTH_PARAM,  0.0, 1.0, 0.0,"Width Cv");
        configParam(LEVEL_PARAM,  0.0, 1.0, 0.0,"Level");
        configParam(SLOPE_PARAM,  0.0, 5.0,0.0,"Slope");
        configParam(CV_LEVEL_PARAM,  0.0, 1.0, 0.0,"Cv");
        configParam(CH1_LEVEL_PARAM,  0.0, 1.0, 0.0,"Ch 1 Level");
        configParam(CH2_LEVEL_PARAM,  0.0, 1.0, 0.0,"Ch 2 Level");
        configParam(CH3_LEVEL_PARAM,  0.0, 1.0, 0.0,"Ch 3 Level");
        configParam(CH4_LEVEL_PARAM,  0.0, 1.0, 0.0,"Ch 4 Level");
        configParam(CH5_LEVEL_PARAM,  0.0, 1.0, 0.0,"Ch 5 Level");
        configParam(CH6_LEVEL_PARAM,  0.0, 1.0, 0.0,"Ch 6 Level");
		
		
		configInput(CH1_INPUT,"Ch 1");
        configInput(CH2_INPUT,"Ch 2");
        configInput(CH3_INPUT,"Ch 3");
        configInput(CH4_INPUT,"Ch 4");
        configInput(CH5_INPUT,"Ch 5");
        configInput(CH6_INPUT,"Ch 6");
        configInput(CH1_CV,"Ch 1 Cv");
        configInput(CH2_CV,"Ch 2 Cv");
        configInput(CH3_CV,"Ch 3 Cv");
        configInput(CH4_CV,"Ch 4 Cv");
        configInput(CH5_CV,"Ch 5 Cv");
        configInput(CH6_CV,"Ch 6 Cv");
        configInput(SLOPE_INPUT,"Slope");
        configInput(SCAN_INPUT,"Ch Scan");
        configInput(WIDTH_INPUT,"Width");
        configInput(LEVEL_INPUT,"Level1");
		
		
        configOutput(A_OUTPUT,"A_");
        configOutput(B_OUTPUT,"B_");
        configOutput(C_OUTPUT,"C_");
	
       // onReset();

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


    int clampInt(const int _in, const int min = 0, const int max = 5)
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

    void process(const ProcessArgs &args) override
   {

    float allInValue = 0.0f;

    for (int i = 0; i < 6; i++)
    {
        if (!inputs[CH1_INPUT + i].isConnected())
            ins[i] = allInValue;
        else {
            if(inputs[CH1_CV+i].isConnected())
                ins[i] = inputs[CH1_INPUT + i].getVoltage() * params[CH1_LEVEL_PARAM + i].getValue() * inputs[CH1_CV + i].getVoltage();
            else
                ins[i] = inputs[CH1_INPUT + i].getVoltage()*params[CH1_LEVEL_PARAM+i].getValue();
        }
    }

    int stages = 6;
    const float invStages = 1.0f / stages;
    const float halfStages = stages * 0.5f;
    const float remainInvStages = 1.0f - invStages;

    float widthControl = params[WIDTH_PARAM].getValue() + inputs[WIDTH_INPUT].getVoltage() * params[CV_WIDTH_PARAM].getValue();
    widthControl = clamp(widthControl, 0.0f, 5.0f) * 0.2f;
    widthControl = widthControl * widthControl * widthTable[stages];

    float scanControl = params[SCAN_PARAM].getValue() + inputs[SCAN_INPUT].getVoltage() * params[CV_SCAN_PARAM].getValue();
    scanControl = clamp(scanControl, 0.0f, 5.0f) * 0.2f;

    float slopeControl = params[SLOPE_PARAM].getValue() + inputs[SLOPE_INPUT].getVoltage();
    slopeControl = clamp(slopeControl, 0.0f, 5.0f) * 0.2f;


    float scanFactor1 = LERP(widthControl, halfStages, invStages);
    float scanFactor2 = LERP(widthControl, halfStages + remainInvStages, 1.0f);
    float scanFinal = LERP(scanControl, scanFactor2, scanFactor1);

    float invWidth = 1.0f / (LERP(widthControl, float(stages), invStages + invStages));

    float subStage = 0.0f;
    for (int i = 0; i < 6; i++)
    {
        inMults[i] = (scanFinal + subStage) * invWidth;
        subStage = subStage - invStages;
    }

    for (int i = 0; i < 6; i++)
    {
        inMults[i] = clamp(inMults[i], 0.0f, 1.0f);
        inMults[i] = triShape(inMults[i]);
        inMults[i] = clamp(inMults[i], 0.0f, 1.0f);

        const float shaped = (2.0f - inMults[i]) * inMults[i];
        inMults[i] = LERP(slopeControl, shaped, inMults[i]);
    }

    outputs[A_OUTPUT].setVoltage(0.0f);
    outputs[B_OUTPUT].setVoltage(0.0f);
    outputs[C_OUTPUT].setVoltage(0.0f);

    for (int i = 0; i < 6; i++)
    {
        outputs[i].setVoltage(ins[i] * inMults[i]);

        lights[CH_LIGHT + i].setSmoothBrightness(fmaxf(0.0, inMults[i]),APP->engine->getSampleTime());

        outputs[B_OUTPUT].value = outputs[B_OUTPUT].value + outputs[i].value;

        if (i <= 1)
        {
            outputs[A_OUTPUT].value = outputs[A_OUTPUT].value + outputs[i].value;
        }
        else if (i >= 4)
        {
            outputs[C_OUTPUT].value = outputs[C_OUTPUT].value + outputs[i].value;
        }

    outputs[A_OUTPUT].value = crossfade(outputs[A_OUTPUT].value * params[LEVEL_PARAM].value, outputs[A_OUTPUT].value * params[LEVEL_PARAM].value*clamp(inputs[LEVEL_INPUT].value/10,0.0f,1.0f),params[CV_LEVEL_PARAM].value);
    outputs[B_OUTPUT].value = crossfade(outputs[B_OUTPUT].value * params[LEVEL_PARAM].value, outputs[B_OUTPUT].value * params[LEVEL_PARAM].value*clamp(inputs[LEVEL_INPUT].value/10,0.0f,1.0f),params[CV_LEVEL_PARAM].value);
    outputs[C_OUTPUT].value = crossfade(outputs[C_OUTPUT].value * params[LEVEL_PARAM].value, outputs[C_OUTPUT].value * params[LEVEL_PARAM].value*clamp(inputs[LEVEL_INPUT].value/10,0.0f,1.0f),params[CV_LEVEL_PARAM].value);
    }
}
};


struct RemixWidget : ModuleWidget {


    int lastPanelTheme = -1;
	std::shared_ptr<window::Svg> light_svg;
	std::shared_ptr<window::Svg> dark_svg;
	
  struct PanelThemeItem : MenuItem {
    Remix *module;
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

    Remix *module = dynamic_cast<Remix*>(this->module);
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
RemixWidget(Remix *module) {
    setModule(module);
	// Main panels from Inkscape
 		light_svg = APP->window->loadSvg(asset::plugin(pluginInstance, "res/Light/Remix.svg"));
		dark_svg = APP->window->loadSvg(asset::plugin(pluginInstance, "res/Dark/Remix.svg"));
		int panelTheme = isDark(module ? (&(((Remix*)module)->panelTheme)) : NULL) ? 1 : 0;// need this here since step() not called for module browser
		setPanel(panelTheme == 0 ? light_svg : dark_svg);	
		
    int knob = 30;
    int jack = 27;

    
    float mid = (15*12)/2;
    float midy= 190;
    int space = 9;

    ////////////////////////////////////////////
    SegmentDisplay* segmentDisplay = createWidget<SegmentDisplay>(Vec(mid-50, 180));
    segmentDisplay->box.size = Vec(100, 20);
    segmentDisplay->setLights<BlueLight>(module, Remix::CH_LIGHT,6);
    addChild(segmentDisplay);


    ////////////////////////////////////////////

	addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
	addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
	addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
	addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));


	  addParam(createParam<FlatR>(Vec(10, midy+20), module, Remix::SCAN_PARAM));
    addParam(createParam<FlatA>(Vec(10, midy+10+knob+20), module, Remix::CV_SCAN_PARAM));

    addParam(createParam<FlatR>(Vec(mid-15, midy+20), module, Remix::WIDTH_PARAM));
    addParam(createParam<FlatA>(Vec(mid-15, midy+10+knob+20), module, Remix::CV_WIDTH_PARAM));

    addParam(createParam<Trim>(Vec(mid - 25, 322.5), module, Remix::SLOPE_PARAM));
    addInput(createInput<PJ301MSPort>(Vec(mid + 5 , 320),  module, Remix::SLOPE_INPUT));

    addParam(createParam<FlatR>(Vec(box.size.x - 40 , midy+20), module, Remix::LEVEL_PARAM));
    addParam(createParam<FlatA>(Vec(box.size.x - 40 , midy+10+knob+20), module, Remix::CV_LEVEL_PARAM));

      addOutput(createOutput<PJ301MSPort>(Vec(15 , 20), module, Remix::A_OUTPUT));
      addInput(createInput<PJ301MSPort>(Vec(15, 320),  module, Remix::SCAN_INPUT));

      addOutput(createOutput<PJ301MSPort>(Vec(mid-12 , 20), module, Remix::B_OUTPUT));
      addInput(createInput<PJ301MSPort>(Vec(mid-12, 290),  module, Remix::WIDTH_INPUT));

      addOutput(createOutput<PJ301MSPort>(Vec(box.size.x-40, 20), module, Remix::C_OUTPUT));
      addInput(createInput<PJ301MSPort>(Vec(box.size.x-40, 320),  module, Remix::LEVEL_INPUT));

            addInput(createInput<PJ301MSPort>(Vec(space + 0 * jack,60),  module, Remix::CH1_INPUT));
            addParam(createParam<Trim>(Vec(15 + 0 * jack,115),module,Remix::CH1_LEVEL_PARAM));
            addInput(createInput<PJ301MSPort>(Vec(space  + jack * 0, 140), module, Remix::CH1_CV));

            addInput(createInput<PJ301MSPort>(Vec(space + 1 * jack,60),  module, Remix::CH2_INPUT));
            addParam(createParam<Trim>(Vec(15 + 1 * jack, 115), module, Remix::CH2_LEVEL_PARAM));
            addInput(createInput<PJ301MSPort>(Vec(space  + jack * 1, 140), module, Remix::CH2_CV));

            addInput(createInput<PJ301MSPort>(Vec(space + 2 * jack,60),  module, Remix::CH3_INPUT));
            addParam(createParam<Trim>(Vec(15 + 2 * jack, 115), module, Remix::CH3_LEVEL_PARAM));
            addInput(createInput<PJ301MSPort>(Vec(space  + jack * 2, 140), module, Remix::CH3_CV));


            addInput(createInput<PJ301MSPort>(Vec(space + 3 * jack,60),  module, Remix::CH4_INPUT));
            addParam(createParam<Trim>(Vec(15 + 3 * jack,115),module,Remix::CH4_LEVEL_PARAM));
            addInput(createInput<PJ301MSPort>(Vec(space + jack * 3, 140), module, Remix::CH4_CV));

            addInput(createInput<PJ301MSPort>(Vec(space + 4 * jack,60),  module, Remix::CH5_INPUT));
            addParam(createParam<Trim>(Vec(15 + 4 * jack, 115), module, Remix::CH5_LEVEL_PARAM));
            addInput(createInput<PJ301MSPort>(Vec(space + jack * 4, 140), module, Remix::CH5_CV));

            addInput(createInput<PJ301MSPort>(Vec(space + 5 * jack,60),  module, Remix::CH6_INPUT));
            addParam(createParam<Trim>(Vec(15 + 5 * jack, 115), module, Remix::CH6_LEVEL_PARAM));
            addInput(createInput<PJ301MSPort>(Vec(space + jack * 5, 140), module, Remix::CH6_CV));


}

void step() override {
		int panelTheme = isDark(module ? (&(((Remix*)module)->panelTheme)) : NULL) ? 1 : 0;
		if (lastPanelTheme != panelTheme) {
			lastPanelTheme = panelTheme;
			SvgPanel* panel = (SvgPanel*)getPanel();
			panel->setBackground(panelTheme == 0 ? light_svg : dark_svg);
		}
		Widget::step();
	}
};
Model *modelRemix = createModel<Remix, RemixWidget>("Remix");
