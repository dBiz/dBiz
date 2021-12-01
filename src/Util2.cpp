////////////////////////////////////////////////////////////////////////////
// <Utility module with triggers and env >
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


struct Util2 : Module {
    enum ParamIds
    {
        ENUMS(MODE_PARAM, 4),
        ENUMS(VALUE_PARAM, 4),
        ENUMS(BUTTON_PARAM, 4),
        ENUMS(EBUTTON_PARAM, 2),
        ENUMS(RANGE_PARAM, 2),
        ENUMS(GLIDE_PARAM, 2),
        ENUMS(RISE_PARAM, 2),
        ENUMS(FALL_PARAM, 2),
        ENUMS(SHAPE_PARAM, 2),
        NUM_PARAMS
    };
    enum InputIds
    {
        ENUMS(BUTTON_INPUT, 4),
        ENUMS(TRIG_INPUT, 2),
        ENUMS(IN_INPUT, 2),
        NUM_INPUTS
    };
    enum OutputIds
    {
        ENUMS(BUTTON_OUTPUT, 4),
        ENUMS(EG_OUTPUT, 2),
        ENUMS(OUT_OUTPUT, 2),
        NUM_OUTPUTS
    };

    enum LighIds
    {
        ENUMS(EBUTTON_LIGHT, 2),
        ENUMS(BUTTON_LIGHT, 4),
        NUM_LIGHTS
    };

    float out[2]{};
    float outg[2]{};
    float eg_out[2]{};

    bool gate[2] = {};
    bool gateEg[2] = {};
    bool gateLi[2] = {};

    bool gateState[4] = {};
    bool pulse[4];

    dsp::SchmittTrigger trigger[2];
    dsp::SchmittTrigger etrigger[2];
    dsp::SchmittTrigger btrigger[4];

		int panelTheme;

    dsp::PulseGenerator buttonPulse[4];

    Util2(){

        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        for(int i=0;i<4;i++)
        {
            configParam(MODE_PARAM + i,  0.0, 1.0, 0.0, "Mode");
            configParam(VALUE_PARAM + i,  -10.0, 10.0, 0.0, "Value");
            configParam(BUTTON_PARAM + i,  0.0, 1.0, 0.0, "Button");
        }

        for (int c = 0; c < 2; c++)
        {
            configParam(GLIDE_PARAM + c, 0.0,1.0,0.0,"Glide");
        }
        for(int c=0;c<2;c++)
        {
            configParam(RISE_PARAM+c, 0.0,1.0,0.0,"Rise");
            configParam(FALL_PARAM+c, 0.0,1.0,0.0,"Fall");
            configParam(RANGE_PARAM+c, 0.0,2.0,0.0,"Range");
            configParam(SHAPE_PARAM + c, -1.0, 1.0, 0.0, "Shape");
            configParam(EBUTTON_PARAM + c, 0.0, 1.0, 0.0, "Env Button");
        }
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


/////////////////////////////////////////////////////

void process(const ProcessArgs &args) override
{

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    for (int c = 0; c < 2; c++)
    {
        float in = inputs[IN_INPUT + c].getVoltage();
        float shape = 0.0;
        float delta = in - out[c];

        bool rising = false;
        bool falling = false;

        if (delta > 0)
        {
            // Rise
            float riseCv = params[GLIDE_PARAM + c].getValue();
            float rise = 1e-1 * powf(2.0, riseCv * 10.0);
            out[c] += shapeDelta(delta, rise, shape) * args.sampleTime;
            rising = (in - out[c] > 1e-3);
            if (!rising)
            {
                gate[c] = false;
            }
        }
        else if (delta < 0)
        {
            // Fall
            float fallCv = params[GLIDE_PARAM + c].getValue();
            float fall = 1e-1 * powf(2.0, fallCv * 10.0);
            out[c] += shapeDelta(delta, fall, shape) * args.sampleTime;
            falling = (in - out[c] < -1e-3);
        }
        else
        {
            gate[c] = false;
        }

        if (!rising && !falling)
        {
            out[c] = in;
        }

        outputs[OUT_OUTPUT + c].setVoltage(out[c]);
    }

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    for (int c = 0; c < 2; c++)
    {
       float in = 0.0f;
       if (trigger[c].process(params[EBUTTON_PARAM + c].getValue() * 10 + inputs [TRIG_INPUT + c].getVoltage()))
       {
           gateEg[c] = true;
           gateLi[c] = true;
           outg[c] = 0.0;
       }
        else gateLi[c] = false;

        lights[EBUTTON_LIGHT + c].setSmoothBrightness(gateLi[c] ? 1.0 : 0.0, args.sampleTime);

        if (gateEg[c])
        {
            in = 5.0;
       }


       float shape = params[SHAPE_PARAM + c].getValue();
       float delta = in - outg[c];

       float minTime;
       switch ((int)params[RANGE_PARAM + c].getValue())
       {
       case 0: minTime = 1e-1; break;
       case 1: minTime = 1e-2; break;
       default:minTime = 1e-3; break;
       }

        bool rising = false;
        bool falling = false;

        if (delta > 0)
        {
            // Rise
            float riseCv = params[RISE_PARAM + c].getValue();
            float rise = minTime * powf(2.0, riseCv * 10.0);
            outg[c] += shapeDelta(delta, rise, shape) * args.sampleTime;
            rising = (in - outg[c] > 1e-3);
             if (!rising)
             {
                 gateEg[c] = false;
             }
        }
        else if (delta < 0)
        {
            // Fall
            float fallCv = params[FALL_PARAM + c].getValue();
            float fall = minTime * powf(2.0, fallCv * 10.0);
            outg[c] += shapeDelta(delta, fall, shape) * args.sampleTime;
            falling = (in - outg[c] < -1e-3);
        }
            else
            {
                gateEg[c] = false;
            }

        if (!rising && !falling)
        {
            outg[c] = in;
        }

        outputs[EG_OUTPUT + c].setVoltage(outg[c]*2.0f);
    }

///////////////////////////////////////////////////////////////////////////////////////////////////////

    for (int i=0;i<4;i++)
    {
        if(params[MODE_PARAM+i].getValue()==0)
        {
            if (btrigger[i].process(params[BUTTON_PARAM+i].getValue()*10+inputs[BUTTON_INPUT+i].getVoltage()))
            {
                buttonPulse[i].trigger(1e-3);
                gateState[i] = true;
            }
             else gateState[i] = false;

            lights[BUTTON_LIGHT + i].setSmoothBrightness(gateState[i]? 1.0 : 0.0, args.sampleTime);

            pulse[i] = buttonPulse[i].process(1.0f / APP->engine->getSampleTime());

            outputs[BUTTON_OUTPUT + i].setVoltage(pulse[i] ? 10.0f : 0.0f);
        }

        if (params[MODE_PARAM + i].getValue() == 1)
        {
            if (btrigger[i].process(params[BUTTON_PARAM + i].getValue() * 10.0 + inputs[BUTTON_INPUT + i].getVoltage()))
            {
                gateState[i] = !gateState[i];
            }
            lights[BUTTON_LIGHT + i].setSmoothBrightness(gateState[i] ? 1.0 : 0.0, args.sampleTime);

            if (gateState[i])
            {
                outputs[BUTTON_OUTPUT + i].setVoltage(params[VALUE_PARAM + i].getValue());
            }
            else
            {
                outputs[BUTTON_OUTPUT + i].setVoltage(0.0);
            }
        }
    }

}
};

struct Util2Widget : ModuleWidget
{


	SvgPanel* darkPanel;
	struct PanelThemeItem : MenuItem {
	  Util2 *module;
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

	  Util2 *module = dynamic_cast<Util2*>(this->module);
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
    Util2Widget(Util2 * module){
    setModule(module);
    setPanel(APP->window->loadSvg(asset::plugin(pluginInstance,  "res/Light/Util2.svg")));
		if (module) {
	    darkPanel = new SvgPanel();
	    darkPanel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Dark/Util2.svg")));
	    darkPanel->visible = false;
	    addChild(darkPanel);
	  }

    //Screw
    addChild(createWidget<ScrewBlack>(Vec(15, 0)));
    addChild(createWidget<ScrewBlack>(Vec(box.size.x - 30, 0)));
    addChild(createWidget<ScrewBlack>(Vec(15, 365)));
    addChild(createWidget<ScrewBlack>(Vec(box.size.x - 30, 365)));

    int knob = 33;
    int jack = 28;
    int si = 10;
    int sp =40;

    //
    for (int i = 0; i < 2; i++)
    {
	    addParam(createParam<SDKnob>(Vec(30 + knob, 20 + knob * i), module, Util2::GLIDE_PARAM + i));
        addParam(createParam<SDKnob>(Vec(40 + knob * i, 158-2*knob), module, Util2::RISE_PARAM + i));
        addParam(createParam<SDKnob>(Vec(40 + knob * i, 158-knob), module, Util2::FALL_PARAM + i));
        addParam(createParam<SDKnob>(Vec(40+knob*i, 160), module, Util2::SHAPE_PARAM + i));
        addParam(createParam<MCKSSS>(Vec(si +7+ 3* knob * i, 94 + knob * 2.8), module, Util2::RANGE_PARAM + i));

        addParam(createLightParam<LEDLightBezel<OrangeLight>>(Vec(si+3 + 3*knob * i, 94), module, Util2::EBUTTON_PARAM + i,Util2::EBUTTON_LIGHT + i));
       
    }


        addInput(createInput<PJ301MVAPort>(Vec(si, 23 + knob * 0), module, Util2::IN_INPUT+0));
        addInput(createInput<PJ301MVAPort>(Vec(si, 23 + knob * 1), module, Util2::IN_INPUT+1));

        addOutput(createOutput<PJ301MVAPort>(Vec(si + jack, 23 + knob * 0), module, Util2::OUT_OUTPUT+0));
        addOutput(createOutput<PJ301MVAPort>(Vec(si + jack, 23 + knob * 1), module, Util2::OUT_OUTPUT+1));


        addInput(createInput<PJ301MVAPort>(Vec(si + 3 * knob * 0, 94+knob), module, Util2::TRIG_INPUT + 0));
        addInput(createInput<PJ301MVAPort>(Vec(si + 3 * knob * 1, 94+knob), module, Util2::TRIG_INPUT + 1));

        addOutput(createOutput<PJ301MVAPort>(Vec(si + 3 * knob * 0, 94+knob*2), module, Util2::EG_OUTPUT + 0));
        addOutput(createOutput<PJ301MVAPort>(Vec(si + 3 * knob * 1, 94+knob*2), module, Util2::EG_OUTPUT + 1));




    for (int i=0;i<4;i++)
    {
    addParam(createParam<SDKnob>(Vec(si +2 + knob * i, 170 + jack+sp), module, Util2::VALUE_PARAM + i));
    addParam(createParam<MCKSSS2>(Vec(si + 10 + knob * i, 175 + jack*4+sp), module, Util2::MODE_PARAM + i));
    
    addParam(createLightParam<LEDLightBezel<OrangeLight>>(Vec(si+5+knob * i,170+sp), module, Util2::BUTTON_PARAM + i,Util2::BUTTON_LIGHT + i));
    }

    addInput(createInput<PJ301MVAPort>(Vec(si + 3.5 + knob * 0, 175 + jack * 2+sp), module, Util2::BUTTON_INPUT + 0));
    addInput(createInput<PJ301MVAPort>(Vec(si + 3.5 + knob * 1, 175 + jack * 2+sp), module, Util2::BUTTON_INPUT + 1));
    addInput(createInput<PJ301MVAPort>(Vec(si + 3.5 + knob * 2, 175 + jack * 2+sp), module, Util2::BUTTON_INPUT + 2));
    addInput(createInput<PJ301MVAPort>(Vec(si + 3.5 + knob * 3, 175 + jack * 2+sp), module, Util2::BUTTON_INPUT + 3));

    addOutput(createOutput<PJ301MVAPort>(Vec(si + 3.5 + knob * 0, 175 + jack * 3+sp), module, Util2::BUTTON_OUTPUT + 0));
    addOutput(createOutput<PJ301MVAPort>(Vec(si + 3.5 + knob * 1, 175 + jack * 3+sp), module, Util2::BUTTON_OUTPUT + 1));
    addOutput(createOutput<PJ301MVAPort>(Vec(si + 3.5 + knob * 2, 175 + jack * 3+sp), module, Util2::BUTTON_OUTPUT + 2));
    addOutput(createOutput<PJ301MVAPort>(Vec(si + 3.5 + knob * 3, 175 + jack * 3+sp), module, Util2::BUTTON_OUTPUT + 3));


}
void step() override {
  if (module) {
    Widget* panel = getPanel();
    panel->visible = ((((Util2*)module)->panelTheme) == 0);
    darkPanel->visible  = ((((Util2*)module)->panelTheme) == 1);
  }
  Widget::step();
}
};
Model *modelUtil2 = createModel<Util2, Util2Widget>("Util2");
