////////////////////////////////////////////////////////////////////////////
// <6 Track  mixer>
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


struct DrMix : Module {
    enum ParamIds
    {
        ENUMS(CH_PARAM, 6),
        ENUMS(CH_PAN_PARAM, 6),
        ENUMS(MUTE_PARAM,6),
        CH2_PAN_PARAM,
        OUT_PARAM,  
        NUM_PARAMS
    };
    enum InputIds
    {
        ENUMS(CH_IN, 6),
        NUM_INPUTS
    };
    enum OutputIds {
        L_OUTPUT,
        R_OUTPUT,
        NUM_OUTPUTS
    };

    enum LighIds {
        ENUMS(METER_LIGHT, (6*6)),
        ENUMS(MUTE_LIGHT,6),
        NUM_LIGHTS
    };


    float ch_in[6]={};

    float out_L=0.0f;
    float out_R=0.0f;

    float pan_pos[6]={};
    float outL[6]={};
    float outR[6]={};

    dsp::VuMeter2 vuBars[6];
    dsp::ClockDivider lightCounter;
    dsp::SchmittTrigger mute_triggers[6];
    bool mute_states[6]= {false};


    int panelTheme;


  DrMix() 
  {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
    for(int i=0;i<6;i++)
    {
		configParam(CH_PARAM + i,  0.0, M_SQRT2, 1.0, "Ch level", " dB", -10, 40);
		configParam(CH_PAN_PARAM + i,  0.0, 1.0, 0.5,"Ch Pan", "%", 0, 100);
		configButton(MUTE_PARAM + i);
    }
	
    configParam(OUT_PARAM,  0.0, M_SQRT2, 1.0, "Out Level", "%", 0, 100);
    
    lightCounter.setDivision(256);

    onReset();
    panelTheme = (loadDarkAsDefault() ? 1 : 0);
  }

	json_t *dataToJson() override 
	{
	  json_t *rootJ = json_object();
      // mutes 
      json_t *mute_statesJ = json_array();
		for (int i = 0; i < 6; i++)
		{
			json_t *mute_stateJ = json_boolean(mute_states[i]);
			json_array_append_new(mute_statesJ, mute_stateJ);
		}
		json_object_set_new(rootJ, "mutes", mute_statesJ);
	    // panelTheme
	    json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));
	    return rootJ;
	}

	void dataFromJson(json_t *rootJ) override 
	{
        // mute states
		json_t *mute_statesJ = json_object_get(rootJ, "mutes");
		if (mute_statesJ)
		{
			for (int i = 0; i < 6; i++)
			{
				json_t *mute_stateJ = json_array_get(mute_statesJ, i);
				if (mute_stateJ)
				mute_states[i] = json_boolean_value(mute_stateJ);
			}
		}
	      // panelTheme
	      json_t *panelThemeJ = json_object_get(rootJ, "panelTheme");
	      if (panelThemeJ)
	        panelTheme = json_integer_value(panelThemeJ);
	}

  void process(const ProcessArgs &args) override 
  {
    out_L=0.0f;
    out_R=0.0f;

	for  (int i = 0 ; i < 6; i++)
      {

        if (mute_triggers[i].process(params[MUTE_PARAM + i].getValue()))
          {
           // mute_states[i] ^= true;
           mute_states[i] = !mute_states[i];
          }

          lights[MUTE_LIGHT + i].setBrightness(mute_states[i] ? 1.f : 0.f);
      }

    for(int i=0;i<6;i++)
    {
      
        pan_pos[i] = params[CH_PAN_PARAM+i].getValue();      
        ch_in[i] = (inputs[CH_IN+i].getVoltage() * std::pow(params[CH_PARAM+i].getValue(),2.f));
        outL[i] = ch_in[i] * (1 - pan_pos[i]);
        outR[i] = ch_in[i] * pan_pos[i];
      
		vuBars[i].process(args.sampleTime, ch_in[i] / 5.0);
    
		if (mute_states[i] )
		{
			ch_in[i]=0.0f;
			outL[i] = 0.0f;
			outR[i] = 0.0f;
		}

		if (lightCounter.process())
		{
			for(int i=0;i<6;i++)
			{

				for (int l =0; l < 6; l++)
				{
					lights[METER_LIGHT + (i * 6)+l].setBrightness(vuBars[i].getBrightness(-3.f * (l + 1), -3.f * l));
				}
			}
		}
  
		out_L+=outL[i];
		out_R+=outR[i];

    }

     outputs[L_OUTPUT].setVoltage((out_L/2.0f)* std::pow(params[OUT_PARAM].getValue(),2.f));
     outputs[R_OUTPUT].setVoltage((out_R/2.0f)* std::pow(params[OUT_PARAM].getValue(),2.f));   
  }
};

template <typename BASE>
struct MeterLight : BASE
{
  MeterLight()
  {
    this->box.size = Vec(4, 6);
    this->bgColor = nvgRGBAf(0.0, 0.0, 0.0, 0.1);
  }
};
template <typename BASE>
struct ULight : BASE
{
  ULight()
  {
    this->box.size = mm2px(Vec(4, 4));
  }
};


//////////////////////////////////////////////////////////////////
struct DrMixWidget : ModuleWidget 
{
    	SvgPanel* darkPanel;
	struct PanelThemeItem : MenuItem {
	  DrMix *module;
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

	  DrMix *module = dynamic_cast<DrMix*>(this->module);
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

	    DrMixWidget(DrMix *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance,  "res/Light/DrMix.svg")));
		if (module) {
	    darkPanel = new SvgPanel();
	    darkPanel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Dark/DrMix.svg")));
	    darkPanel->visible = false;
	    addChild(darkPanel);
	  }

    


    //Screw
    addChild(createWidget<ScrewBlack>(Vec(15, 0)));
    addChild(createWidget<ScrewBlack>(Vec(box.size.x - 30, 0)));
    addChild(createWidget<ScrewBlack>(Vec(15, 365)));
    addChild(createWidget<ScrewBlack>(Vec(box.size.x - 30, 365)));

    // Pot
    int sp=50;
    int jk=30;
    int s1 =5;

    addParam(createParam<MicroBlu>(Vec(10,20+sp*0), module, DrMix::CH_PARAM));
    addParam(createParam<MicroBlu>(Vec(50,20+sp*0), module, DrMix::CH_PARAM+1));
    addParam(createParam<MicroBlu>(Vec(10,20+sp*1), module, DrMix::CH_PARAM+2));
    addParam(createParam<MicroBlu>(Vec(50,20+sp*1), module, DrMix::CH_PARAM+3));
    addParam(createParam<MicroBlu>(Vec(10,20+sp*2), module, DrMix::CH_PARAM+4));
    addParam(createParam<MicroBlu>(Vec(50,20+sp*2), module, DrMix::CH_PARAM+5));

    addParam(createParam<Trim>(Vec(23,48+sp*0), module, DrMix::CH_PAN_PARAM+0));

    addParam(createParam<LEDT>(Vec(6,48+sp*0), module,DrMix::MUTE_PARAM+0));
    addChild(createLight<MediumLight<OrangeLight>>(Vec(7.5,49.5+sp*0), module, DrMix::MUTE_LIGHT+0));

    addParam(createParam<Trim>(Vec(63,48+sp*0), module, DrMix::CH_PAN_PARAM+1));

    addParam(createParam<LEDT>(Vec(46,48+sp*0), module,DrMix::MUTE_PARAM+1));
    addChild(createLight<MediumLight<OrangeLight>>(Vec(47.5,49.5+sp*0), module, DrMix::MUTE_LIGHT+1));

    addParam(createParam<Trim>(Vec(23,48+sp*1), module, DrMix::CH_PAN_PARAM+2));

    addParam(createParam<LEDT>(Vec(6,48+sp*1), module,DrMix::MUTE_PARAM+2));
    addChild(createLight<MediumLight<OrangeLight>>(Vec(7.5,49.5+sp*1), module, DrMix::MUTE_LIGHT+2));

    addParam(createParam<Trim>(Vec(63,48+sp*1), module, DrMix::CH_PAN_PARAM+3));

    addParam(createParam<LEDT>(Vec(46,48+sp*1), module,DrMix::MUTE_PARAM+3));
    addChild(createLight<MediumLight<OrangeLight>>(Vec(47.5,49.5+sp*1), module, DrMix::MUTE_LIGHT+3));

    addParam(createParam<Trim>(Vec(23,48+sp*2), module, DrMix::CH_PAN_PARAM+4));

    addParam(createParam<LEDT>(Vec(6,48+sp*2), module,DrMix::MUTE_PARAM+4));
    addChild(createLight<MediumLight<OrangeLight>>(Vec(7.5,49.5+sp*2), module, DrMix::MUTE_LIGHT+4));

    addParam(createParam<Trim>(Vec(63,48+sp*2), module, DrMix::CH_PAN_PARAM+5));

    addParam(createParam<LEDT>(Vec(46,48+sp*2), module,DrMix::MUTE_PARAM+5));
    addChild(createLight<MediumLight<OrangeLight>>(Vec(47.5,49.5+sp*2), module, DrMix::MUTE_LIGHT+5));


    addInput(createInput<PJ301MIPort>(Vec(10, 205+jk*0 ), module, DrMix::CH_IN));
    addInput(createInput<PJ301MIPort>(Vec(52.5, 205+jk*0 ), module, DrMix::CH_IN+1));
    addInput(createInput<PJ301MIPort>(Vec(10, 205+jk*1), module, DrMix::CH_IN+2));
    addInput(createInput<PJ301MIPort>(Vec(52.5, 205+jk*1), module, DrMix::CH_IN+3));
    addInput(createInput<PJ301MIPort>(Vec(10, 205+jk*2), module, DrMix::CH_IN+4));
    addInput(createInput<PJ301MIPort>(Vec(52.5, 205+jk*2), module, DrMix::CH_IN+5));


    addParam(createParam<VerboDS>(Vec(27,295), module, DrMix::OUT_PARAM));
    // addParam(createParam<MicroBlu>(Vec(50,295), module, DrMix::OUTM_PARAM));

    addOutput(createOutput<PJ301MLPort>(Vec(10 , 330 ),  module, DrMix::L_OUTPUT));
    addOutput(createOutput<PJ301MRPort>(Vec(52.5 , 330 ),  module, DrMix::R_OUTPUT));

   for (int i=0;i<6;i++)
   {

     addChild(createLight<MeterLight<PurpleLight>>(Vec(10+13*i,170+s1), module, DrMix::METER_LIGHT + (6 * i)));
     addChild(createLight<MeterLight<BlueLight>>(Vec(10+13*i,174+s1), module, DrMix::METER_LIGHT + 1 + (6 * i)));
     addChild(createLight<MeterLight<BlueLight>>(Vec(10+13*i,178+s1), module, DrMix::METER_LIGHT + 2 + (6 * i)));
     addChild(createLight<MeterLight<WhiteLight>>(Vec(10+13*i,182+s1), module, DrMix::METER_LIGHT + 3 + (6 * i)));
     addChild(createLight<MeterLight<WhiteLight>>(Vec(10+13*i,186+s1), module, DrMix::METER_LIGHT + 4 + (6 * i)));
     addChild(createLight<MeterLight<WhiteLight>>(Vec(10+13*i,190+s1), module, DrMix::METER_LIGHT + 5 + (6 * i)));
   }
  
  }

    void step() override
    {
      if (module)
      {
        Widget* panel = getPanel();
        panel->visible = ((((DrMix *)module)->panelTheme) == 0);
        darkPanel->visible = ((((DrMix *)module)->panelTheme) == 1);
      }
      Widget::step();
    }
};
Model *modelDrMix = createModel<DrMix, DrMixWidget>("DrMix");
