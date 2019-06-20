
#include "plugin.hpp"

///////////////////////////////////////////////////
struct PerfMixer : Module {
  enum ParamIds
  {
    MIX_PARAM,
    AUX_R1_PARAM,
    AUX_R2_PARAM,
    AUX_S1_PARAM,
    AUX_S2_PARAM,
    ENUMS(VOL_PARAM, 8),
    ENUMS(PAN_PARAM, 8),
    ENUMS(AUX_1_PARAM ,8),
    ENUMS(AUX_2_PARAM ,8),
    ENUMS(MUTE_PARAM, 8),
    NUM_PARAMS
  };
  enum InputIds
  {
    MIX_IN_L_INPUT,
    MIX_IN_R_INPUT,
    ENUMS(CH_L_INPUT, 8), 
    ENUMS(CH_R_INPUT, 8),
    ENUMS(CH_VOL_INPUT, 8),
    ENUMS(CH_PAN_INPUT, 8),
   // ENUMS(CH_MUTE_INPUT, 8),
    ENUMS(AUX_1_INPUT, 8),
    ENUMS(AUX_2_INPUT, 8),
    RETURN_1_L_INPUT,
    RETURN_1_R_INPUT,
    RETURN_2_L_INPUT,
    RETURN_2_R_INPUT,
    NUM_INPUTS
  };

  enum OutputIds {
		MIX_OUTPUT_L,
    MIX_OUTPUT_R,
    SEND_1_L_OUTPUT,
    SEND_1_R_OUTPUT,
    SEND_2_L_OUTPUT,
    SEND_2_R_OUTPUT,    
		NUM_OUTPUTS
  };

  enum LightIds
  {
    ENUMS(PAN_L_LIGHT, 8),
    ENUMS(PAN_R_LIGHT, 8),
    ENUMS(MUTE_LIGHT, 8),
    ENUMS(METERL_LIGHT, (11*8)),
    ENUMS(METERR_LIGHT, (11*8)),
    NUM_LIGHTS
  };

  dsp::SchmittTrigger mute_triggers[8];

  bool mute_states[8];

  float ch_l_ins[8];
  float ch_r_ins[8];
  float channel_outs_l[8];
  float channel_outs_r[8];
  float channel_s1_L[8];
  float channel_s1_R[8];
  float channel_s2_L[8];
  float channel_s2_R[8];
  float left_sum = 0.0;
  float right_sum = 0.0;

  float mix_in_l =0.0f;
  float mix_in_r = 0.0f;

  float send_1_L_sum = 0.0;
  float send_1_R_sum = 0.0;
  float send_2_R_sum = 0.0;
  float send_2_L_sum = 0.0;

  dsp::VuMeter2 vuBarsL[8];
  dsp::VuMeter2 vuBarsR[8];
  dsp::ClockDivider lightCounter;

  PerfMixer()
  {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS,NUM_LIGHTS);

    configParam(MIX_PARAM,  0.0, 1.0, 0.5,"Mix Level", "%", 0, 100);
    configParam(AUX_R1_PARAM,  0.0, 1.0, 0.0,"Aux Return 1", "%", 0, 100);
    configParam(AUX_R2_PARAM,  0.0, 1.0, 0.0,"Aux Return 2", "%", 0, 100);
    configParam(AUX_S1_PARAM,  0.0, 1.0, 0.0,"Auz Send 1", "%", 0, 100); 
    configParam(AUX_S2_PARAM,  0.0, 1.0, 0.0,"Auz Send 2", "%", 0, 100);

    for(int i=0;i<8;i++)
    { 
      configParam(VOL_PARAM + i,  0.0, 1.0, 0.0,"Ch Level", "%", 0, 100); 
      configParam(PAN_PARAM + i,  0.0, 1.0, 0.5,"Ch Pan", "%", 0, 100); 
      configParam(AUX_1_PARAM + i,  0.0, 1.0, 0.0,"Send 1 Level", "%", 0, 100);
      configParam(AUX_2_PARAM + i,  0.0, 1.0, 0.0,"Send 2 Level", "%", 0, 100);
      configParam(MUTE_PARAM + i,  0.0, 1.0, 0.0,"Mute", "%", 0, 1);
  
    }

   lightCounter.setDivision(256);


  }


void process(const ProcessArgs &args) override {

  send_1_L_sum = 0.0;
  send_1_R_sum = 0.0;
  send_2_L_sum = 0.0;
  send_2_R_sum = 0.0;
  left_sum = 0.0;
  right_sum = 0.0;
  

  mix_in_l=inputs[MIX_IN_L_INPUT].getVoltage();
  mix_in_r=inputs[MIX_IN_R_INPUT].getVoltage();

  float pan_cv[8]={};
  float pan_pos[8]={};

    // mute triggers

    for  (int i = 0 ; i < 8; i++)
      {

        if (mute_triggers[i].process(params[MUTE_PARAM + i].getValue()))
          {
            mute_states[i] ^= true;
    	    }

          lights[MUTE_LIGHT + i].setBrightness(mute_states[i] ? 1.f : 0.f);
      }


    for (int i = 0 ; i < 8 ; i++)
    {
      pan_cv[i] = inputs[CH_PAN_INPUT + i].value / 5;
      pan_pos[i] = pan_cv[i] + params[PAN_PARAM + i].value;
      if (pan_pos[i] < 0)
        pan_pos[i] = 0;
      if (pan_pos[i] > 1)
        pan_pos[i] = 1;

      lights[PAN_L_LIGHT+i].value=1-pan_pos[i];
      lights[PAN_R_LIGHT+i].value=pan_pos[i];

      ch_l_ins[i] = (inputs[CH_L_INPUT + i].getVoltage() * std::pow(params[VOL_PARAM + i].value,2.f));
      if(inputs[CH_VOL_INPUT+i].isConnected()) 
       ch_l_ins[i] *= (inputs[CH_VOL_INPUT + i].getVoltage() / 10.0f);


      ch_r_ins[i] = (inputs[CH_R_INPUT + i].getVoltage() * std::pow(params[VOL_PARAM + i].value,2.f));
      if(inputs[CH_VOL_INPUT+i].isConnected()) 
       ch_r_ins[i]*= (inputs[CH_VOL_INPUT + i].getVoltage() / 10.0f);

      if (mute_states[i] )
      {
        ch_l_ins[i] = 0.0;
        ch_r_ins[i] = 0.0;
      }

          if(!inputs[CH_R_INPUT+i].getVoltage())
          {
            channel_outs_l[i] = ch_l_ins[i] * (1 - pan_pos[i]) * 3;
            channel_outs_r[i] = ch_l_ins[i] * pan_pos[i] * 3;
          }
          else
          {
            channel_outs_l[i] = ch_l_ins[i] * 2;
            channel_outs_r[i] = ch_r_ins[i] * 2;
          }
          

          channel_s1_L[i] = (channel_outs_l[i] * params[AUX_1_PARAM + i].value);
          if(inputs[AUX_1_INPUT].isConnected()) 
          channel_s1_L[i] *= (inputs[AUX_1_INPUT + i].value /10.f);

          channel_s2_L[i] = (channel_outs_l[i] * params[AUX_2_PARAM + i].value);
          if(inputs[AUX_2_INPUT].isConnected()) 
          channel_s2_L[i] *= (inputs[AUX_2_INPUT + i].value /10.f);

          channel_s1_R[i] = (channel_outs_r[i] * params[AUX_1_PARAM + i].value);
          if(inputs[AUX_1_INPUT].isConnected()) 
          channel_s1_R[i] *= (inputs[AUX_1_INPUT + i].value /10.f);

          channel_s2_R[i] = (channel_outs_r[i] * params[AUX_2_PARAM + i].value);
          if(inputs[AUX_2_INPUT].isConnected()) 
          channel_s2_R[i] *= (inputs[AUX_2_INPUT + i].value /10.f);


          vuBarsL[i].process(args.sampleTime,channel_outs_l[i] / 10.0);
          vuBarsR[i].process(args.sampleTime,channel_outs_r[i] / 10.0);

  if (lightCounter.process()) 
  {
    for(int i=0;i<8;i++){
      lights[METERL_LIGHT + i * 11 +0].setBrightness(vuBarsL[i].getBrightness(0.f,0.f));
      lights[METERR_LIGHT + i * 11 +0].setBrightness(vuBarsR[i].getBrightness(0.f,0.f));

          for (int l = 1; l < 11; l++)
          {
            lights[METERL_LIGHT + (i * 11)+l].setBrightness(vuBarsL[i].getBrightness(-3.f * (l + 1), -3.f * l));
            lights[METERR_LIGHT + (i * 11)+l].setBrightness(vuBarsR[i].getBrightness(-3.f * (l + 1), -3.f * l));
            }
        }
  }

            send_1_L_sum += channel_s1_L[i];
            send_1_R_sum += channel_s1_R[i];
            send_2_L_sum += channel_s2_L[i];
            send_2_R_sum += channel_s2_R[i];
            left_sum += channel_outs_l[i];
            right_sum += channel_outs_r[i];
        
    }
	

    // get returns
 
    float return_1_l = inputs[RETURN_1_L_INPUT].value * params[AUX_R1_PARAM].value;
    float return_1_r = inputs[RETURN_1_R_INPUT].value * params[AUX_R1_PARAM].value;
    float return_2_l = inputs[RETURN_2_L_INPUT].value * params[AUX_R2_PARAM].value;
    float return_2_r = inputs[RETURN_2_R_INPUT].value * params[AUX_R2_PARAM].value;


  	float mix_l = (left_sum + return_1_l + return_2_l) * params[MIX_PARAM].value*0.5;
    float mix_r = (right_sum + return_1_r + return_2_r) * params[MIX_PARAM].value*0.5;
      
    
    float send_1_L_mix = (send_1_L_sum) * params[AUX_S1_PARAM].value;
    float send_1_R_mix = (send_1_R_sum) * params[AUX_S1_PARAM].value;
    float send_2_L_mix = (send_2_L_sum) * params[AUX_S2_PARAM].value;
    float send_2_R_mix = (send_2_R_sum) * params[AUX_S2_PARAM].value;

    outputs[MIX_OUTPUT_L].value = mix_l+mix_in_l;
    outputs[MIX_OUTPUT_R].value = mix_r+mix_in_r;


    outputs[SEND_1_L_OUTPUT].value = 3 * send_1_L_mix;
    outputs[SEND_1_R_OUTPUT].value = 3 * send_1_R_mix;
    outputs[SEND_2_L_OUTPUT].value = 3 * send_2_L_mix;
    outputs[SEND_2_R_OUTPUT].value = 3 * send_2_R_mix;

    	
  }

  
  json_t *dataToJson() override
  {
    json_t *rootJ = json_object();

    // mute states
    json_t *mute_statesJ = json_array();
    for (int i = 0; i < 8; i++)
    {
      json_t *mute_stateJ = json_boolean(mute_states[i]);
      json_array_append_new(mute_statesJ, mute_stateJ);
    }
    json_object_set_new(rootJ, "mutes", mute_statesJ);
    return rootJ;
  }

  void dataFromJson(json_t *rootJ) override
  {
    // mute states
    json_t *mute_statesJ = json_object_get(rootJ, "mutes");
    if (mute_statesJ)
    {
      for (int i = 0; i < 8; i++)
      {
        json_t *mute_stateJ = json_array_get(mute_statesJ, i);
        if (mute_stateJ)
          mute_states[i] = json_boolean_value(mute_stateJ);
      }
    }
  }

};
template <typename BASE>
struct MuteLight : BASE
{
  MuteLight()
  {
    this->box.size = Vec(17.0, 17.0);
  }
};

template <typename BASE>
struct MeterLight : BASE
{
  MeterLight()
  {
    this->box.size = Vec(4, 4);
    this->bgColor = nvgRGBAf(0.0, 0.0, 0.0, 0.1);
  }
};


struct PerfMixerWidget : ModuleWidget {
PerfMixerWidget(PerfMixer *module){
  setModule(module);
  setPanel(APP->window->loadSvg(asset::plugin(pluginInstance,  "res/PerfMixer.svg")));
	
  int column_1 = 70;
  int lb=5;
  int right_column = 310;
  int top=50;
  int top_row = 60;
  int row_spacing = 28;
  int row_in = 40;
  int column_spacing = 30;

  addParam(createParam<LRoundWhy>(Vec(right_column + 5, 10), module, PerfMixer::MIX_PARAM)); // master volume
  addParam(createParam<MicroBlu>(Vec(right_column+7.5, 225 ), module, PerfMixer::AUX_R1_PARAM));
  addParam(createParam<MicroBlu>(Vec(right_column+7.5, 285 ), module, PerfMixer::AUX_R2_PARAM));
  addParam(createParam<MicroBlu>(Vec(right_column+7.5, 102.5 ), module, PerfMixer::AUX_S1_PARAM));
  addParam(createParam<MicroBlu>(Vec(right_column+7.5, 160 ), module, PerfMixer::AUX_S2_PARAM));

  // channel strips
  for (int i = 0 ; i < 8 ; i++)
  {

    addInput(createInput<PJ301MLPort>(Vec(5, 15),module, PerfMixer::MIX_IN_L_INPUT));
    addInput(createInput<PJ301MRPort>(Vec(30,15),module, PerfMixer::MIX_IN_R_INPUT));

    addParam(createParam<MicroBlu>(Vec(column_1 + column_spacing * i, 75), module, PerfMixer::AUX_1_PARAM + i));
    addParam(createParam<MicroBlu>(Vec(column_1 + column_spacing * i, 105), module, PerfMixer::AUX_2_PARAM + i));

    addInput(createInput<PJ301MCPort>(Vec(column_1 + column_spacing * i, 15),module, PerfMixer::AUX_1_INPUT + i));
    addInput(createInput<PJ301MCPort>(Vec(column_1 + column_spacing * i, 40),module, PerfMixer::AUX_2_INPUT + i));

    addInput(createInput<PJ301MIPort>(Vec(lb, top + row_in * i), module, PerfMixer::CH_L_INPUT + i));
    addInput(createInput<PJ301MIPort>(Vec(lb + 25, top + row_in * i), module, PerfMixer::CH_R_INPUT + i));

    addParam(createParam<LEDSliderBlue>(Vec(column_1 + column_spacing * i-5, top_row + row_spacing * 2 - 20 + top), module, PerfMixer::VOL_PARAM + i));

    addInput(createInput<PJ301MCPort>(Vec(column_1 + column_spacing * i - 1, top_row + row_spacing * 6 - 45 + top), module, PerfMixer::CH_VOL_INPUT + i));

/////////////////////////////////////////////////////

    addChild(createLight<MeterLight<OrangeLight>>(Vec(column_1 + column_spacing * i + 1 , top_row + row_spacing * 6 + top-13),module,PerfMixer::PAN_L_LIGHT+i));
    addChild(createLight<MeterLight<OrangeLight>>(Vec(column_1 + column_spacing * i + 20 , top_row + row_spacing * 6 + top-13),module,PerfMixer::PAN_R_LIGHT+i));


    addParam(createParam<Trimpot>(Vec(column_1 + column_spacing * i +3, top_row + row_spacing * 6 + top-8), module, PerfMixer::PAN_PARAM + i));
    addInput(createInput<PJ301MOrPort>(Vec(column_1 + column_spacing * i - 1, top_row + row_spacing * 6 + top+12.5), module, PerfMixer::CH_PAN_INPUT + i));

    ////////////////////////////////////////////////////////

    addParam(createParam<LEDB>(Vec(column_1 + column_spacing * i + 3 , top_row + row_spacing * 7+ 10.5 + top+13), module, PerfMixer::MUTE_PARAM + i));
    addChild(createLight<MuteLight<BlueLight>>(Vec(column_1 + column_spacing * i + 4.5 , top_row + row_spacing * 7 +12 + top+13), module, PerfMixer::MUTE_LIGHT + i));
    
   // addInput(createInput<PJ301MCPort>(Vec(column_1 + column_spacing * i-1, top_row + row_spacing * 8 + top+5), module, PerfMixer::CH_MUTE_INPUT + i));

  
    addChild(createLight<MeterLight<PurpleLight>>(Vec(column_1 + 19 + column_spacing * i-5, top_row + row_spacing * 2 - 27 + top + 7.5), module, PerfMixer::METERL_LIGHT + (11 * i)));
    addChild(createLight<MeterLight<PurpleLight>>(Vec(column_1 + 19 + column_spacing * i-5, top_row + row_spacing * 2 - 27 + top + 7.5 * 2), module, PerfMixer::METERL_LIGHT + 1 + (11 * i)));
    addChild(createLight<MeterLight<BlueLight>>(Vec(column_1 + 19 + column_spacing * i-5, top_row + row_spacing * 2 - 27 + top + 7.5 * 3), module, PerfMixer::METERL_LIGHT + 2 + (11 * i)));
    addChild(createLight<MeterLight<BlueLight>>(Vec(column_1 + 19 + column_spacing * i-5, top_row + row_spacing * 2 - 27 + top + 7.5 * 4), module, PerfMixer::METERL_LIGHT + 3 + (11 * i)));
    addChild(createLight<MeterLight<BlueLight>>(Vec(column_1 + 19 + column_spacing * i-5, top_row + row_spacing * 2 - 27 + top + 7.5 * 5), module, PerfMixer::METERL_LIGHT + 4 + (11 * i)));
    addChild(createLight<MeterLight<BlueLight>>(Vec(column_1 + 19 + column_spacing * i-5, top_row + row_spacing * 2 - 27 + top + 7.5 * 6), module, PerfMixer::METERL_LIGHT + 5 + (11 * i)));
    addChild(createLight<MeterLight<BlueLight>>(Vec(column_1 + 19 + column_spacing * i-5, top_row + row_spacing * 2 - 27 + top + 7.5 * 7), module, PerfMixer::METERL_LIGHT + 6 + (11 * i)));
    addChild(createLight<MeterLight<WhiteLight>>(Vec(column_1 + 19 + column_spacing * i-5, top_row + row_spacing * 2 - 27 + top + 7.5 * 8), module, PerfMixer::METERL_LIGHT + 7 + (11 * i)));
    addChild(createLight<MeterLight<WhiteLight>>(Vec(column_1 + 19 + column_spacing * i-5, top_row + row_spacing * 2 - 27 + top + 7.5 * 9), module, PerfMixer::METERL_LIGHT + 8 + (11 * i)));
    addChild(createLight<MeterLight<WhiteLight>>(Vec(column_1 + 19 + column_spacing * i-5, top_row + row_spacing * 2 - 27 + top + 7.5 * 10), module, PerfMixer::METERL_LIGHT + 9 + (11 * i)));
    addChild(createLight<MeterLight<WhiteLight>>(Vec(column_1 + 19 + column_spacing * i-5, top_row + row_spacing * 2 - 27 + top + 7.5 * 11), module, PerfMixer::METERL_LIGHT + 10 + (11 * i)));

    addChild(createLight<MeterLight<PurpleLight>>(Vec(column_1 + 24 + column_spacing * i-5, top_row + row_spacing * 2 - 27 + top + 7.5), module, PerfMixer::METERR_LIGHT + (11 * i)));
    addChild(createLight<MeterLight<PurpleLight>>(Vec(column_1 + 24 + column_spacing * i-5, top_row + row_spacing * 2 - 27 + top + 7.5 * 2), module, PerfMixer::METERR_LIGHT + 1 + (11 * i)));
    addChild(createLight<MeterLight<BlueLight>>(Vec(column_1 + 24 + column_spacing * i-5, top_row + row_spacing * 2 - 27 + top + 7.5 * 3), module, PerfMixer::METERR_LIGHT + 2 + (11 * i)));
    addChild(createLight<MeterLight<BlueLight>>(Vec(column_1 + 24 + column_spacing * i-5, top_row + row_spacing * 2 - 27 + top + 7.5 * 4), module, PerfMixer::METERR_LIGHT + 3 + (11 * i)));
    addChild(createLight<MeterLight<BlueLight>>(Vec(column_1 + 24 + column_spacing * i-5, top_row + row_spacing * 2 - 27 + top + 7.5 * 5), module, PerfMixer::METERR_LIGHT + 4 + (11 * i)));
    addChild(createLight<MeterLight<BlueLight>>(Vec(column_1 + 24 + column_spacing * i-5, top_row + row_spacing * 2 - 27 + top + 7.5 * 6), module, PerfMixer::METERR_LIGHT + 5 + (11 * i)));
    addChild(createLight<MeterLight<BlueLight>>(Vec(column_1 + 24 + column_spacing * i-5, top_row + row_spacing * 2 - 27 + top + 7.5 * 7), module, PerfMixer::METERR_LIGHT + 6 + (11 * i)));
    addChild(createLight<MeterLight<WhiteLight>>(Vec(column_1 + 24 + column_spacing * i-5, top_row + row_spacing * 2 - 27 + top + 7.5 * 8), module, PerfMixer::METERR_LIGHT + 7 + (11 * i)));
    addChild(createLight<MeterLight<WhiteLight>>(Vec(column_1 + 24 + column_spacing * i-5, top_row + row_spacing * 2 - 27 + top + 7.5 * 9), module, PerfMixer::METERR_LIGHT + 8 + (11 * i)));
    addChild(createLight<MeterLight<WhiteLight>>(Vec(column_1 + 24 + column_spacing * i-5, top_row + row_spacing * 2 - 27 + top + 7.5 * 10), module, PerfMixer::METERR_LIGHT + 9 + (11 * i)));
    addChild(createLight<MeterLight<WhiteLight>>(Vec(column_1 + 24 + column_spacing * i-5, top_row + row_spacing * 2 - 27 + top + 7.5 * 11), module, PerfMixer::METERR_LIGHT + 10 + (11 * i)));
  } 
 

//Screw

  addChild(createWidget<ScrewBlack>(Vec(15, 0)));
	addChild(createWidget<ScrewBlack>(Vec(box.size.x-30, 0)));
	addChild(createWidget<ScrewBlack>(Vec(15, 365)));
	addChild(createWidget<ScrewBlack>(Vec(box.size.x-30, 365)));

  // outputs
  addOutput(createOutput<PJ301MLPort>(Vec(right_column +5 , 60),    module, PerfMixer::MIX_OUTPUT_L));
	addOutput(createOutput<PJ301MRPort>(Vec(right_column +30 , 60 ),  module, PerfMixer::MIX_OUTPUT_R));

  addOutput(createOutput<PJ301MLPort>(Vec(right_column + 35, 100 ),  module, PerfMixer::SEND_1_L_OUTPUT));
  addOutput(createOutput<PJ301MRPort>(Vec(right_column + 35, 125 ),  module, PerfMixer::SEND_1_R_OUTPUT));
  
  addOutput(createOutput<PJ301MLPort>(Vec(right_column + 35, 160 ),  module, PerfMixer::SEND_2_L_OUTPUT));
  addOutput(createOutput<PJ301MRPort>(Vec(right_column + 35, 185 ),  module, PerfMixer::SEND_2_R_OUTPUT));

  addInput(createInput<PJ301MLPort>(Vec(right_column + 35, 225 ), module, PerfMixer::RETURN_1_L_INPUT));
  addInput(createInput<PJ301MRPort>(Vec(right_column + 35, 250 ), module, PerfMixer::RETURN_1_R_INPUT));

  addInput(createInput<PJ301MLPort>(Vec(right_column + 35, 285 ), module, PerfMixer::RETURN_2_L_INPUT));
  addInput(createInput<PJ301MRPort>(Vec(right_column + 35, 310 ), module, PerfMixer::RETURN_2_R_INPUT));
}
};
Model *modelPerfMixer = createModel<PerfMixer, PerfMixerWidget>("PerfMixer");

