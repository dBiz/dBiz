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


struct DrMixExt : Module {
    enum ParamIds
    {
        ENUMS(CHSEND_A_PARAM, 6),
        ENUMS(CHSEND_B_PARAM, 6),
        SEND_A_PARAM,
        SEND_B_PARAM,
        RETURN_A_PARAM,
        RETURN_B_PARAM,
        NUM_PARAMS
    };
    enum InputIds
    {
        ENUMS(RETURN_A_INPUT, 2),
        ENUMS(RETURN_B_INPUT, 2),
        NUM_INPUTS
    };
    enum OutputIds {
        ENUMS(SEND_A_OUTPUT, 2),
        ENUMS(SEND_B_OUTPUT, 2),
        NUM_OUTPUTS
    };

    enum LighIds {
        NUM_LIGHTS
    };

 
    float ch_in_L[6]={};
    float ch_in_R[6]={};
    float chs_AL_in[6]={};
    float chs_BL_in[6]={};
    float chs_AR_in[6]={};
    float chs_BR_in[6]={};
    int panelTheme;
    float out_a_L;
    float out_a_R;
    float out_b_L;
    float out_b_R;

    float consumerMessage[12] = {};// this module must read from here
	float producerMessage[12] = {};// mother will write into here

  DrMixExt() 
  {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
    for(int i=0;i<6;i++)
	{
		configParam(CHSEND_A_PARAM + i,  0.0,1.0, 0.0, "Send A");
		configParam(CHSEND_B_PARAM + i,  0.0, 1.0, 0.0,"Send B");
    }
	configParam(SEND_A_PARAM,  0.0, 1.0, 0.0, "Send A Out Level");
	configParam(SEND_B_PARAM,  0.0, 1.0, 0.0, "Send B Out Level");
	configParam(RETURN_A_PARAM,  0.0, 1.0, 0.0, "Return A Out Level");
	configParam(RETURN_B_PARAM,  0.0, 1.0, 0.0, "Return B Out Level");


    leftExpander.producerMessage = producerMessage;
    leftExpander.consumerMessage = consumerMessage;

    onReset();

    panelTheme = (loadDarkAsDefault() ? 1 : 0);
  }

  void process(const ProcessArgs &args) override 
  {

    if(leftExpander.module && leftExpander.module->model == modelDrMix) 
    {
	 float *messagesFromMother = (float*)leftExpander.consumerMessage;
     float *messagestoMother =  (float*)leftExpander.module->rightExpander.producerMessage;
   
    for (int i=0;i<6;i++)
    {
		ch_in_L[i]=messagesFromMother[i]/10.f; 	
		ch_in_R[i]=messagesFromMother[i+6]/10.f;
	
        chs_AL_in[i]=ch_in_L[i]*params[CHSEND_A_PARAM+i].getValue();
        chs_AR_in[i]=ch_in_R[i]*params[CHSEND_A_PARAM+i].getValue();

        chs_BL_in[i]=ch_in_L[i]*params[CHSEND_B_PARAM+i].getValue();  
        chs_BR_in[i]=ch_in_R[i]*params[CHSEND_B_PARAM+i].getValue();
//
        out_a_L+=chs_AL_in[i];
        out_a_R+=chs_AR_in[i];
        out_b_L+=chs_BL_in[i];
        out_b_R+=chs_BR_in[i];
    }

    
     float send_1_L_mix = (out_a_L) * params[SEND_A_PARAM].getValue();
     float send_1_R_mix = (out_a_R) * params[SEND_A_PARAM].getValue();
     float send_2_L_mix = (out_b_L) * params[SEND_B_PARAM].getValue();
     float send_2_R_mix = (out_b_R) * params[SEND_B_PARAM].getValue();
    
    outputs[SEND_A_OUTPUT+0].setVoltage(send_1_L_mix);
    outputs[SEND_A_OUTPUT+1].setVoltage(send_1_R_mix);
    outputs[SEND_B_OUTPUT+0].setVoltage(send_2_L_mix);
    outputs[SEND_B_OUTPUT+1].setVoltage(send_2_R_mix);

    messagestoMother[0]=inputs[RETURN_A_INPUT+0].getVoltage()*params[RETURN_A_PARAM].getValue();
    messagestoMother[1]=inputs[RETURN_A_INPUT+1].getVoltage()*params[RETURN_A_PARAM].getValue();
    messagestoMother[2]=inputs[RETURN_B_INPUT+0].getVoltage()*params[RETURN_B_PARAM].getValue();
    messagestoMother[3]=inputs[RETURN_B_INPUT+1].getVoltage()*params[RETURN_B_PARAM].getValue();
 
    leftExpander.module->rightExpander.messageFlipRequested = true;
    
   }
  }
};


//////////////////////////////////////////////////////////////////
struct DrMixExtWidget : ModuleWidget 
{
    	SvgPanel* darkPanel;
	struct PanelThemeItem : MenuItem {
	  DrMixExt *module;
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

	  DrMixExt *module = dynamic_cast<DrMixExt*>(this->module);
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

	    DrMixExtWidget(DrMixExt *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance,  "res/Light/DrMixExt.svg")));
		if (module) {
	    darkPanel = new SvgPanel();
	    darkPanel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Dark/DrMixExt.svg")));
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
    

    addParam(createParam<Trim>(Vec(5,28+sp*0), module, DrMixExt::CHSEND_A_PARAM+0));
    addParam(createParam<Trim>(Vec(45,28+sp*0), module, DrMixExt::CHSEND_A_PARAM+1));
    addParam(createParam<Trim>(Vec(5,28+sp*1), module, DrMixExt::CHSEND_A_PARAM+2));
    addParam(createParam<Trim>(Vec(45,28+sp*1), module, DrMixExt::CHSEND_A_PARAM+3));
    addParam(createParam<Trim>(Vec(5,28+sp*2), module, DrMixExt::CHSEND_A_PARAM+4));
    addParam(createParam<Trim>(Vec(45,28+sp*2), module, DrMixExt::CHSEND_A_PARAM+5));



    addParam(createParam<Trim>(Vec(25,48+sp*0), module, DrMixExt::CHSEND_B_PARAM+0));
    addParam(createParam<Trim>(Vec(65,48+sp*0), module, DrMixExt::CHSEND_B_PARAM+1));
    addParam(createParam<Trim>(Vec(25,48+sp*1), module, DrMixExt::CHSEND_B_PARAM+2));
    addParam(createParam<Trim>(Vec(65,48+sp*1), module, DrMixExt::CHSEND_B_PARAM+3));
    addParam(createParam<Trim>(Vec(25,48+sp*2), module, DrMixExt::CHSEND_B_PARAM+4));
    addParam(createParam<Trim>(Vec(65,48+sp*2), module, DrMixExt::CHSEND_B_PARAM+5));

    addParam(createParam<MicroBlu>(Vec(10,180), module, DrMixExt::SEND_A_PARAM));
    addOutput(createOutput<PJ301MLPort>(Vec(10, 210+jk*0 ), module, DrMixExt::SEND_A_OUTPUT+0));
    addOutput(createOutput<PJ301MRPort>(Vec(10, 210+jk*1 ), module, DrMixExt::SEND_A_OUTPUT+1));
     
    addParam(createParam<MicroBlu>(Vec(55,180), module, DrMixExt::RETURN_A_PARAM));
    addInput(createInput<PJ301MLPort>(Vec(55, 210+jk*0 ), module, DrMixExt::RETURN_A_INPUT+0));
    addInput(createInput<PJ301MRPort>(Vec(55, 210+jk*1 ), module, DrMixExt::RETURN_A_INPUT+1));

    addParam(createParam<MicroBlu>(Vec(10,270), module, DrMixExt::SEND_B_PARAM));
    addOutput(createOutput<PJ301MLPort>(Vec(10, 300+jk*0 ), module, DrMixExt::SEND_B_OUTPUT+0));
    addOutput(createOutput<PJ301MRPort>(Vec(10, 300+jk*1 ), module, DrMixExt::SEND_B_OUTPUT+1));
        
    addParam(createParam<MicroBlu>(Vec(55,270), module, DrMixExt::RETURN_B_PARAM));
    addInput(createInput<PJ301MLPort>(Vec(55, 300+jk*0 ), module, DrMixExt::RETURN_B_INPUT+0));
    addInput(createInput<PJ301MRPort>(Vec(55, 300+jk*1 ), module, DrMixExt::RETURN_B_INPUT+1));

/*

    addInput(createInput<PJ301MIPort>(Vec(10, 205+jk*0 ), module, DrMixExt::CH_IN));
    addInput(createInput<PJ301MIPort>(Vec(52.5, 205+jk*0 ), module, DrMixExt::CH_IN+1));
    addInput(createInput<PJ301MIPort>(Vec(10, 205+jk*1), module, DrMixExt::CH_IN+2));
    addInput(createInput<PJ301MIPort>(Vec(52.5, 205+jk*1), module, DrMixExt::CH_IN+3));
    addInput(createInput<PJ301MIPort>(Vec(10, 205+jk*2), module, DrMixExt::CH_IN+4));
    addInput(createInput<PJ301MIPort>(Vec(52.5, 205+jk*2), module, DrMixExt::CH_IN+5));


    addParam(createParam<VerboDS>(Vec(27,295), module, DrMixExt::OUT_PARAM));
    // addParam(createParam<MicroBlu>(Vec(50,295), module, DrMixExt::OUTM_PARAM));

    addOutput(createOutput<PJ301MLPort>(Vec(10 , 330 ),  module, DrMixExt::L_OUTPUT));
    addOutput(createOutput<PJ301MRPort>(Vec(52.5 , 330 ),  module, DrMixExt::R_OUTPUT));
  */
  }
 

    void step() override
    {
      if (module)
      {
        Widget* panel = getPanel();
        panel->visible = ((((DrMixExt *)module)->panelTheme) == 0);
        darkPanel->visible = ((((DrMixExt *)module)->panelTheme) == 1);
      }
      Widget::step();
    }
};
Model *modelDrMixExt = createModel<DrMixExt, DrMixExtWidget>("DrMixExt");
