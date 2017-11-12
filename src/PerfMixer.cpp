///////////////////////////////////////////////////
//
//   Mixer VCV Module
//   Strum 2017
//
///////////////////////////////////////////////////
 
#include "dBiz.hpp"
#include "dsp/digital.hpp"
 
///////////////////////////////////////////////////
struct PerfMixer : Module {
  enum ParamIds
  {
    MIX_PARAM,
    AUX_R1_PARAM,
    AUX_R2_PARAM,
    AUX_S1_PARAM = AUX_R2_PARAM + 8,
    AUX_S2_PARAM = AUX_S1_PARAM + 8,
    VOL_PARAM = AUX_S2_PARAM + 8,
    AUX_1_PARAM = VOL_PARAM + 8,
    AUX_2_PARAM = AUX_1_PARAM + 8,
    MUTE_PARAM = AUX_2_PARAM + 8,
    NUM_PARAMS = MUTE_PARAM + 8
  };
  enum InputIds
  {
    CH_L_INPUT = 8,
    CH_R_INPUT = CH_L_INPUT + 8,
    CH_VOL_INPUT = CH_R_INPUT + 8,
    CH_MUTE_INPUT = CH_VOL_INPUT + 8,
    AUX_1_INPUT = CH_MUTE_INPUT + 8,
    AUX_2_INPUT = CH_MUTE_INPUT + 8,
    RETURN_1_L_INPUT = CH_MUTE_INPUT + 16,
    RETURN_1_R_INPUT = RETURN_1_L_INPUT + 16,
    RETURN_2_L_INPUT = RETURN_1_R_INPUT + 16,
    RETURN_2_R_INPUT = RETURN_2_L_INPUT + 16,
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
    MUTE_LIGHTS,
    NUM_LIGHTS=MUTE_LIGHTS+8
  };

  SchmittTrigger mute_triggers[8];
  bool mute_states[8] = {1, 1, 1, 1, 1, 1, 1, 1};

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

  float send_1_L_sum = 0.0;
  float send_1_R_sum = 0.0;
  float send_2_R_sum = 0.0;
  float send_2_L_sum = 0.0;

  PerfMixer() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS,NUM_LIGHTS) {}
  void step() override;

  json_t *toJson() override
  {
    json_t *rootJ = json_object();

    // mute states
    json_t *mute_statesJ = json_array();
    for (int i = 0; i < 8; i++)
    {
      json_t *mute_stateJ = json_integer((int)mute_states[i]);
      json_array_append_new(mute_statesJ, mute_stateJ);
    }
    json_object_set_new(rootJ, "mutes", mute_statesJ);
    return rootJ;
  }

  void fromJson(json_t *rootJ) override
  {
    // mute states
    json_t *mute_statesJ = json_object_get(rootJ, "mutes");
    if (mute_statesJ)
    {
      for (int i = 0; i < 8; i++)
      {
        json_t *mute_stateJ = json_array_get(mute_statesJ, i);
        if (mute_stateJ)
          mute_states[i] = !!json_integer_value(mute_stateJ);
      }
    }
  }

};

///////////////////////////////////////////////////////////////////
void PerfMixer::step() 
{
  send_1_L_sum = 0.0;
  send_1_R_sum = 0.0;
  send_2_L_sum = 0.0;
  send_2_R_sum = 0.0;
  left_sum = 0.0;
  right_sum = 0.0;

  
  // mute triggers

    for  (int i = 0 ; i < 8; i++)
      {

        if (mute_triggers[i].process(params[MUTE_PARAM + i].value))
        {
    		  mute_states[i] = !mute_states[i];
    	  }
        lights[MUTE_LIGHTS + i].value = mute_states[i] ? 1.0 : 0.0;
      }


    for (int i = 0 ; i < 8 ; i++)
    {  
        ch_l_ins[i] = inputs[CH_L_INPUT + i ].value * params[VOL_PARAM + i].value * clampf(inputs[CH_VOL_INPUT + i].normalize(10.0) / 10.0, 0.0, 1.0);
        ch_r_ins[i] = inputs[CH_R_INPUT + i ].value * params[VOL_PARAM + i].value * clampf(inputs[CH_VOL_INPUT + i].normalize(10.0) / 10.0, 0.0, 1.0);

        if (!mute_states[i] || inputs[CH_MUTE_INPUT + i].value > 0.0 )
        {
          ch_l_ins[i] = 0.0;
          ch_r_ins[i] = 0.0;
          lights[MUTE_LIGHTS + i].value = 0.0;      
        }
          channel_outs_l[i] = ch_l_ins[i] * 2;
          channel_outs_r[i] = ch_r_ins[i] * 2;

          channel_s1_L[i] = ch_l_ins[i] * params[AUX_1_PARAM + i].value * clampf(inputs[AUX_1_INPUT + i].normalize(5.0)/5.0,1.0,1.0) * clampf(inputs[CH_VOL_INPUT + i].normalize(10.0) / 10.0, 0.0, 1.0);
          channel_s2_L[i] = ch_l_ins[i] * params[AUX_2_PARAM + i].value * clampf(inputs[AUX_2_INPUT + i].normalize(5.0)/5.0,1.0,1.0) * clampf(inputs[CH_VOL_INPUT + i].normalize(10.0) / 10.0, 0.0, 1.0);

          channel_s1_R[i] = ch_r_ins[i] * params[AUX_1_PARAM + i].value * clampf(inputs[AUX_1_INPUT + i].normalize(5.0)/5.0,1.0,1.0) * clampf(inputs[CH_VOL_INPUT + i].normalize(10.0) / 10.0, 0.0, 1.0);
          channel_s2_R[i] = ch_r_ins[i] * params[AUX_2_PARAM + i].value * clampf(inputs[AUX_2_INPUT + i].normalize(5.0)/5.0,1.0,1.0) * clampf(inputs[CH_VOL_INPUT + i].normalize(10.0) / 10.0, 0.0, 1.0);

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

    outputs[MIX_OUTPUT_L].value = mix_l;
    outputs[MIX_OUTPUT_R].value = mix_r;


    outputs[SEND_1_L_OUTPUT].value = send_1_L_mix;
    outputs[SEND_1_R_OUTPUT].value = send_1_R_mix;
    outputs[SEND_2_L_OUTPUT].value = send_2_L_mix;
    outputs[SEND_2_R_OUTPUT].value = send_2_R_mix;

    	
  }
  
template <typename BASE>
struct MuteLight : BASE
{
  MuteLight()
  {
    this->box.size = Vec(10.0, 10.0);
  }
};

PerfMixerWidget::PerfMixerWidget() {
	PerfMixer *module = new PerfMixer();
	setModule(module);
	box.size = Vec(15*25, 380);

	{
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
    panel->setBackground(SVG::load(assetPlugin(plugin,"res/PerfMixer.svg")));
		addChild(panel);
	}
  int column_1 = 70;
  int lb=5;
  int right_column = 310;
  int top=50;
  int top_row = 60;
  int row_spacing = 28;
  int row_in = 40;
  int column_spacing = 30;

  addParam(createParam<SynthTechAlco>(Vec(right_column + 5, 10), module, PerfMixer::MIX_PARAM, 0.0, 1.0, 0.5)); // master volume

  addParam(createParam<MicroBlu>(Vec(right_column+5, 225 ), module, PerfMixer::AUX_R1_PARAM, 0.0, 1.0, 0.0));
  addParam(createParam<MicroBlu>(Vec(right_column+5, 285 ), module, PerfMixer::AUX_R2_PARAM, 0.0, 1.0, 0.0));

  addParam(createParam<MicroBlu>(Vec(right_column+5, 100 ), module, PerfMixer::AUX_S1_PARAM, 0.0, 1.0, 0.0));
  addParam(createParam<MicroBlu>(Vec(right_column+5, 160 ), module, PerfMixer::AUX_S2_PARAM, 0.0, 1.0, 0.0));

  // channel strips
  for (int i = 0 ; i < 8 ; i++)
  {
      
          addParam(createParam<MicroBlu>(Vec(column_1+column_spacing*i,75 ), module, PerfMixer::AUX_1_PARAM + i, 0.0, 1.0, 0.0));
          addParam(createParam<MicroBlu>(Vec(column_1+column_spacing*i,105 ), module, PerfMixer::AUX_2_PARAM + i, 0.0, 1.0, 0.0));
          addInput(createInput<PJ301MIPort>(Vec(column_1 + column_spacing * i, 15), module, PerfMixer::AUX_1_INPUT + i));
          addInput(createInput<PJ301MIPort>(Vec(column_1 + column_spacing * i, 40), module, PerfMixer::AUX_2_INPUT + i));

          addInput(createInput<PJ301MIPort>(Vec(lb , top + row_in*i ), module, PerfMixer::CH_L_INPUT + i));
          addInput(createInput<PJ301MIPort>(Vec(lb + 25, top + row_in*i), module, PerfMixer::CH_R_INPUT + i));

          addParam(createParam<SlidePot2>(Vec(column_1 + column_spacing * i, top_row + row_spacing * 2 - 30 + top), module, PerfMixer::VOL_PARAM + i, 0.0, 1.0, 0.0));

          addInput(createInput<PJ301MCPort>(Vec(column_1 + column_spacing * i - 5, top_row + row_spacing * 6 - 20 + top), module, PerfMixer::CH_VOL_INPUT + i));

          addParam(createParam<LEDButton>(Vec(column_1 + column_spacing * i, top_row + row_spacing * 7 + top), module, PerfMixer::MUTE_PARAM + i, 0.0, 1.0, 0.0));
          addChild(createLight<MuteLight<GreenLight>>(Vec(column_1 + column_spacing * i + 4, top_row + row_spacing * 7 + 4 + top), module, PerfMixer::MUTE_LIGHTS + i));
          addInput(createInput<PJ301MCPort>(Vec(column_1 + column_spacing * i, top_row + row_spacing * 8 + top), module, PerfMixer::CH_MUTE_INPUT + i));
	}
 

//Screw

  addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 365)));

  // outputs
  addOutput(createOutput<PJ301MLPort>(Vec(right_column +5 , 60), module, PerfMixer::MIX_OUTPUT_L));
	addOutput(createOutput<PJ301MRPort>(Vec(right_column +30 , 60 ), module, PerfMixer::MIX_OUTPUT_R));

  addOutput(createOutput<PJ301MLPort>(Vec(right_column + 35, 100 ), module, PerfMixer::SEND_1_L_OUTPUT));
  addOutput(createOutput<PJ301MRPort>(Vec(right_column + 35, 125 ), module, PerfMixer::SEND_1_R_OUTPUT));
  
  addOutput(createOutput<PJ301MLPort>(Vec(right_column + 35, 160 ), module, PerfMixer::SEND_2_L_OUTPUT));
  addOutput(createOutput<PJ301MRPort>(Vec(right_column + 35, 185 ), module, PerfMixer::SEND_2_R_OUTPUT));

  addInput(createInput<PJ301MLPort>(Vec(right_column + 35, 225 ), module, PerfMixer::RETURN_1_L_INPUT));
  addInput(createInput<PJ301MRPort>(Vec(right_column + 35, 250 ), module, PerfMixer::RETURN_1_R_INPUT));

  addInput(createInput<PJ301MLPort>(Vec(right_column + 35, 285 ), module, PerfMixer::RETURN_2_L_INPUT));
  addInput(createInput<PJ301MRPort>(Vec(right_column + 35, 310 ), module, PerfMixer::RETURN_2_R_INPUT));
}
