///////////////////////////////////////////////////
//
//   dBiz revisited version of 
//   Sub Mixer VCV Module
//   Built from fundamental VCMixer 
//
//   Strum 2017
//
///////////////////////////////////////////////////
#include "dBiz.hpp"

struct SubMixer : Module {
	enum ParamIds {
		MIX_PARAM,
		CH1_PARAM,
    CH1_PAN_PARAM,
		CH2_PARAM,
    CH2_PAN_PARAM,
		CH3_PARAM,
    CH3_PAN_PARAM,
    CH4_PARAM,
    CH4_PAN_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		MIX_CV_INPUT,
		CH1_INPUT,
		CH1_CV_INPUT,
    CH1_PAN_INPUT,
		CH2_INPUT,
		CH2_CV_INPUT,
    CH2_PAN_INPUT,
		CH3_INPUT,
		CH3_CV_INPUT,
    CH3_PAN_INPUT,
    CH4_INPUT,
		CH4_CV_INPUT,
    CH4_PAN_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		MIX_OUTPUT_L,
    MIX_OUTPUT_R,
		CH1_OUTPUT,
		CH2_OUTPUT,
		CH3_OUTPUT,
    CH4_OUTPUT,
		NUM_OUTPUTS
	};

	SubMixer() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS) {};
	void step() override ;
};


void SubMixer::step() {
  
  float ch1 = inputs[CH1_INPUT].value * params[CH1_PARAM].value * clampf(inputs[CH1_CV_INPUT].normalize(10.0) / 10.0, 0.0, 1.0);

  float pan_cv_in_1 = inputs[CH1_PAN_INPUT].value/5;
  float pan_position_1 = pan_cv_in_1 + params[CH1_PAN_PARAM].value;
   
  if (pan_position_1 < 0) pan_position_1 = 0;
  if (pan_position_1 > 1) pan_position_1 = 1;  
    
  float ch1_l = ch1 * (1-pan_position_1)* 2;
  float ch1_r = ch1 * pan_position_1 * 2;

	float ch2 = inputs[CH2_INPUT].value * params[CH2_PARAM].value * clampf(inputs[CH2_CV_INPUT].normalize(10.0) / 10.0, -1.0, 1.0);
  
  float pan_cv_in_2 = inputs[CH2_PAN_INPUT].value/5;
  float pan_position_2 = pan_cv_in_2 + params[CH2_PAN_PARAM].value;
   
  if (pan_position_2 < 0) pan_position_2 = 0;
  if (pan_position_2 > 1) pan_position_2 = 1;  
    
  float ch2_l = ch2 * (1-pan_position_2)* 2;
  float ch2_r = ch2 * pan_position_2 * 2;
  
	float ch3 = inputs[CH3_INPUT].value * params[CH3_PARAM].value * clampf(inputs[CH3_CV_INPUT].normalize(10.0) / 10.0, 0.0, 1.0);

  float pan_cv_in_3 = inputs[CH3_PAN_INPUT].value/5;
  float pan_position_3 = pan_cv_in_3 + params[CH3_PAN_PARAM].value;
   
  if (pan_position_3 < 0) pan_position_3 = 0;
  if (pan_position_3 > 1) pan_position_3 = 1;  
    
  float ch3_l = ch3 * (1-pan_position_3)* 2;
  float ch3_r = ch3 * pan_position_3 * 2;
    
  float ch4 = inputs[CH4_INPUT].value * params[CH4_PARAM].value * clampf(inputs[CH4_CV_INPUT].normalize(10.0) / 10.0, 0.0, 1.0);

  float pan_cv_in_4 = inputs[CH4_PAN_INPUT].value/5;
  float pan_position_4 = pan_cv_in_4 + params[CH4_PAN_PARAM].value;
   
  if (pan_position_4 < 0) pan_position_4 = 0;
  if (pan_position_4 > 1) pan_position_4 = 1;  
    
  float ch4_l = ch4 * (1-pan_position_4)* 2;
  float ch4_r = ch4 * pan_position_4 * 2;

	float mix_l = (ch1_l + ch2_l + ch3_l + ch4_l) * params[MIX_PARAM].value * inputs[MIX_CV_INPUT].normalize(10.0) / 10.0;
  float mix_r = (ch1_r + ch2_r + ch3_r + ch4_r) * params[MIX_PARAM].value * inputs[MIX_CV_INPUT].normalize(10.0) / 10.0;

	outputs[CH1_OUTPUT].value = ch1;
	outputs[CH2_OUTPUT].value = ch2;
	outputs[CH3_OUTPUT].value = ch3;
  outputs[CH4_OUTPUT].value = ch4;
	outputs[MIX_OUTPUT_L].value = mix_l;
  outputs[MIX_OUTPUT_R].value = mix_r;
}


SubMixWidget::SubMixWidget() {
	SubMixer *module = new SubMixer();
	setModule(module);
	box.size = Vec(150, 380);


	{
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
        panel->setBackground(SVG::load(assetPlugin(plugin,"res/SubMixer.svg")));
		addChild(panel);
	}
  int y_offset = 40;
	addParam(createParam<LDaviesAzz>(Vec(23, 15), module, SubMixer::MIX_PARAM, 0.0, 1.0, 0.5));
  addInput(createInput<PJ301MIPort>(Vec(74, 15), module, SubMixer::MIX_CV_INPUT));

//Screw

  addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 365)));

  
//mixer out

  addOutput(createOutput<PJ301MLPort>(Vec(122, 20), module, SubMixer::MIX_OUTPUT_L));
	addOutput(createOutput<PJ301MRPort>(Vec(122, 45), module, SubMixer::MIX_OUTPUT_R));
  
//


  	addParam(createParam<DaviesRed>(Vec(32,y_offset + 68), module, SubMixer::CH1_PARAM, 0.0, 1.0, 0.0));
  	addParam(createParam<DaviesBlu>(Vec(80,y_offset + 68), module, SubMixer::CH1_PAN_PARAM, 0.0, 1.0, 0.5));
  
		addParam(createParam<DaviesRed>(Vec(32,y_offset + 134), module, SubMixer::CH2_PARAM, 0.0, 1.0, 0.0));
  	addParam(createParam<DaviesBlu>(Vec(80,y_offset + 134), module, SubMixer::CH2_PAN_PARAM, 0.0, 1.0, 0.5));
  
		addParam(createParam<DaviesRed>(Vec(32,y_offset + 199), module, SubMixer::CH3_PARAM, 0.0, 1.0, 0.0));
  	addParam(createParam<DaviesBlu>(Vec(80,y_offset + 199), module, SubMixer::CH3_PAN_PARAM, 0.0, 1.0, 0.5));
  
 	 	addParam(createParam<DaviesRed>(Vec(32,y_offset + 264), module, SubMixer::CH4_PARAM, 0.0, 1.0, 0.0));
 	 	addParam(createParam<DaviesBlu>(Vec(80,y_offset + 264), module, SubMixer::CH4_PAN_PARAM, 0.0, 1.0, 0.5));		
  

		addInput(createInput<PJ301MIPort>(Vec(3,y_offset + 75), module, SubMixer::CH1_INPUT));
		addInput(createInput<PJ301MCPort>(Vec(35,y_offset + 105), module, SubMixer::CH1_CV_INPUT));
  	addInput(createInput<PJ301MCPort>(Vec(88,y_offset + 105), module, SubMixer::CH1_PAN_INPUT));
  
		addInput(createInput<PJ301MIPort>(Vec(3,y_offset + 140), module, SubMixer::CH2_INPUT));
		addInput(createInput<PJ301MCPort>(Vec(35,y_offset + 170), module, SubMixer::CH2_CV_INPUT));
 	 	addInput(createInput<PJ301MCPort>(Vec(88,y_offset + 170), module, SubMixer::CH2_PAN_INPUT));
  
		addInput(createInput<PJ301MIPort>(Vec(3,y_offset + 205), module, SubMixer::CH3_INPUT));
		addInput(createInput<PJ301MCPort>(Vec(35,y_offset + 235), module, SubMixer::CH3_CV_INPUT));
  	addInput(createInput<PJ301MCPort>(Vec(88,y_offset + 235), module, SubMixer::CH3_PAN_INPUT));
  
 	 	addInput(createInput<PJ301MIPort>(Vec(3,y_offset + 270), module, SubMixer::CH4_INPUT));
		addInput(createInput<PJ301MCPort>(Vec(35,y_offset + 300), module, SubMixer::CH4_CV_INPUT));
 	 	addInput(createInput<PJ301MCPort>(Vec(88,y_offset + 300), module, SubMixer::CH4_PAN_INPUT));
    
  
		addOutput(createOutput<PJ301MOPort>(Vec(122,y_offset + 75), module, SubMixer::CH1_OUTPUT));
		addOutput(createOutput<PJ301MOPort>(Vec(122,y_offset + 140), module, SubMixer::CH2_OUTPUT));
		addOutput(createOutput<PJ301MOPort>(Vec(122,y_offset + 205), module, SubMixer::CH3_OUTPUT));
  	addOutput(createOutput<PJ301MOPort>(Vec(122,y_offset + 270), module, SubMixer::CH4_OUTPUT));
  
}
