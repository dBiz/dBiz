///////////////////////////////////////////////////
//  dBiz revisited version of 
//
//   
//
//   Strum 2017
//
///////////////////////////////////////////////////

#include "dBiz.hpp"
#include "dsp/digital.hpp"
#include "dsp/filter.hpp"

///////////////////////////////////////////////////
struct PerfMixer : Module 
{
	enum ParamIds 
	{
		MIX_PARAM,
   		VOL_PARAM,
    	AUX_SEND_1_PARAM,
    	AUX_SEND_2_PARAM,
    	AUX_RETURN_1_PARAM,
    	AUX_RETURN_2_PARAM,
    	PAN_PARAM = VOL_PARAM + 8 ,
    	AUX_1_PARAM = PAN_PARAM + 8 ,
    	AUX_2_PARAM = AUX_1_PARAM + 8,
    	MUTE_PARAM = AUX_2_PARAM + 8,
    	MONO_PARAM = MUTE_PARAM + 8,		
		NUM_PARAMS = MONO_PARAM + 8
	};

	enum InputIds 
	{
		CH_INPUT,
		CH_VOL_INPUT = CH_INPUT + 8,
    	CH_MUTE_INPUT = CH_VOL_INPUT + 8,
    	CH_PAN_INPUT = CH_MUTE_INPUT + 8,
    	RETURN_1_L_INPUT = CH_PAN_INPUT + 8,
    	RETURN_1_R_INPUT,
    	RETURN_2_L_INPUT,
    	RETURN_2_R_INPUT,		
		NUM_INPUTS
	};
  
	enum OutputIds 
	{
		MIX_OUTPUT_L,
    	MIX_OUTPUT_R,
    	SEND_1_L_OUTPUT,
    	SEND_1_R_OUTPUT,
    	SEND_2_L_OUTPUT,
    	SEND_2_R_OUTPUT,    
		NUM_OUTPUTS
	};


		PeakFilter vuFilterL;
		PeakFilter vuFilterR;
		PeakFilter lightFilterL;
		PeakFilter lightFilterR;

  		SchmittTrigger mute_triggers[8];
  		float mute_lights[8] = {1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0};
  		bool mute_states[8]= {1,1,1,1,1,1,1,1};

  		SchmittTrigger mono_triggers[8];
  		float mono_lights[8] = {0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0};
  		bool mono_states[8]= {0,0,0,0,0,0,0,0};

  		float pan_cv_ins[8];
  		float pan_positions[8];

  		float channel_ins[16]; 
  		float channel_outs_l[16];
  		float channel_outs_r[16];
  		float channel_sends_1_L[16];
  		float channel_sends_1_R[16];
  		float channel_sends_2_L[16];
  		float channel_sends_2_R[16];
  		float left_sum = 0.0;
  		float right_sum = 0.0;

  		float send_1_L_sum = 0.0;
  		float send_1_R_sum = 0.0;
  		float send_2_R_sum = 0.0;
		float send_2_L_sum = 0.0;
		  
  		float vuLightsL[7] = {};
  		float vuLightsR[7] = {};
  		float lightsL[1] = {};
		float lightsR[1] = {};
		  
  		PerfMixer() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS) {}
		void step() override ;
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

  int v=0;

  // mute triggers

    for  (int i = 0 ; i < 8; i++)
      {
        if (mute_triggers[i].process(params[MUTE_PARAM + i].value))
        {
    		  mute_states[i] = !mute_states[i];
    	  }
        mute_lights[i] = mute_states[i] ? 1.0 : 0.0;
      }

  // mono triggers

     for  (int i = 0 ; i < 8; i ++)
      {
        if (mono_triggers[i].process(params[MONO_PARAM + i].value))
        {
          mono_states[i] = !mono_states[i];
        }
        mono_lights[i] = mono_states[i] ? 1.0 : 0.0;
      }

			

  for (int i = 0 ; i < 16 ; i++)
    {  

      channel_ins[i] = inputs[CH_INPUT + i].value * params[VOL_PARAM + v].value * clampf(inputs[CH_VOL_INPUT + v].normalize(10.0) / 10.0, 0.0, 1.0);
    
        if (!mute_states[v] || inputs[CH_MUTE_INPUT + v].value > 0.0 )
        {
          channel_ins[i] = 0.0;
          mute_lights[v] = 0;      
        }

        if (!mono_states[v] > 0.0 )
        {
        
          if ( i%2 == 0)
            {
              channel_outs_l[i] = channel_ins[i] * 2;
              channel_sends_1_L[i] = channel_ins[i] * params[AUX_1_PARAM + v].value * clampf(inputs[CH_VOL_INPUT + v].normalize(10.0) / 10.0, 0.0, 1.0);
              channel_sends_2_L[i] = channel_ins[i] * params[AUX_2_PARAM + v].value * clampf(inputs[CH_VOL_INPUT + v].normalize(10.0) / 10.0, 0.0, 1.0);

            }
          else
           { 
              channel_outs_r[i] = channel_ins[i] * 2;
              channel_sends_1_R[i] = channel_ins[i] * params[AUX_1_PARAM + v].value * clampf(inputs[CH_VOL_INPUT + v].normalize(10.0) / 10.0, 0.0, 1.0);
              channel_sends_2_R[i] = channel_ins[i] * params[AUX_2_PARAM + v].value * clampf(inputs[CH_VOL_INPUT + v].normalize(10.0) / 10.0, 0.0, 1.0);   
			} 
        }

        else

        {
          if ( i%2 == 0)
            {
               pan_cv_ins[v] = inputs[CH_PAN_INPUT + v].value/5;
               pan_positions[v] = pan_cv_ins[v] + params[PAN_PARAM+v].value;   
               if (pan_positions[v] < 0) pan_positions[v] = 0;
               if (pan_positions[v] > 1) pan_positions[v] = 1;    
               channel_outs_l[i]= channel_ins[i] * (1-pan_positions[v])* 2;
			   channel_outs_r[i]= channel_ins[i] * pan_positions[v] * 2;
			   channel_sends_1_L[i] = channel_ins[i] * params[AUX_1_PARAM + v].value * clampf(inputs[CH_VOL_INPUT + v].normalize(10.0) / 10.0, 0.0, 1.0) * (1-pan_positions[v]);
			   channel_sends_2_L[i] = channel_ins[i] * params[AUX_2_PARAM + v].value * clampf(inputs[CH_VOL_INPUT + v].normalize(10.0) / 10.0, 0.0, 1.0) * (1-pan_positions[v]);     
            }
          else
            {
               pan_cv_ins[v] = inputs[CH_PAN_INPUT + v].value/5;
               pan_positions[v] = pan_cv_ins[v] + params[PAN_PARAM+v].value;   
               if (pan_positions[v] < 0) pan_positions[v] = 0;
               if (pan_positions[v] > 1) pan_positions[v] = 1;    
               channel_outs_l[i]= channel_ins[i] * (1-pan_positions[v])* 2;
			   channel_outs_r[i]= channel_ins[i] * pan_positions[v] * 2;  
			   channel_sends_1_R[i] = channel_ins[i] * params[AUX_1_PARAM + v].value * clampf(inputs[CH_VOL_INPUT + v].normalize(10.0) / 10.0, 0.0, 1.0) * pan_positions[v];
			   channel_sends_2_R[i] = channel_ins[i] * params[AUX_2_PARAM + v].value * clampf(inputs[CH_VOL_INPUT + v].normalize(10.0) / 10.0, 0.0, 1.0) * pan_positions[v];     
			   
            } 
         }  

        send_1_L_sum += channel_sends_1_L[i];
        send_1_R_sum += channel_sends_1_R[i];
        send_2_L_sum += channel_sends_2_L[i];
        send_2_R_sum += channel_sends_2_R[i];
        left_sum += channel_outs_l[i];
		right_sum += channel_outs_r[i];  

		if ( i%2 != 0)
		{
		v+=1;	
		} 

    }
	

    // get returns
 
    float return_1_l = inputs[RETURN_1_L_INPUT].value * params[AUX_RETURN_1_PARAM].value;
    float return_1_r = inputs[RETURN_1_R_INPUT].value * params[AUX_RETURN_1_PARAM].value;
    float return_2_l = inputs[RETURN_2_L_INPUT].value * params[AUX_RETURN_2_PARAM].value;
    float return_2_r = inputs[RETURN_2_R_INPUT].value * params[AUX_RETURN_2_PARAM].value;


  	float mix_l = (left_sum + return_1_l + return_2_l) * params[MIX_PARAM].value;
    float mix_r = (right_sum + return_1_r + return_2_r) * params[MIX_PARAM].value;
  		

    float send_1_L_mix = (send_1_L_sum) * params[AUX_SEND_1_PARAM].value;
    float send_1_R_mix = (send_1_R_sum) * params[AUX_SEND_1_PARAM].value;
    float send_2_L_mix = (send_2_L_sum) * params[AUX_SEND_2_PARAM].value;
    float send_2_R_mix = (send_2_R_sum) * params[AUX_SEND_2_PARAM].value;

    outputs[MIX_OUTPUT_L].value = mix_l;
    outputs[MIX_OUTPUT_R].value = mix_r;

      

    outputs[SEND_1_L_OUTPUT].value = send_1_L_mix;
    outputs[SEND_1_R_OUTPUT].value = send_1_R_mix;
    outputs[SEND_2_L_OUTPUT].value = send_2_L_mix;
    outputs[SEND_2_R_OUTPUT].value = send_2_R_mix;

    	

    	float lightRateL = 5.0 / engineGetSampleRate();
    	vuFilterL.setRate(lightRateL);
    	vuFilterL.process(fabsf(mix_l));
    	lightFilterL.setRate(lightRateL);
    	lightFilterL.process(fabsf(mix_l*50.0));

    	float vuValueL = vuFilterL.peak();

    	for (int i = 0; i < 7; i++) 
    	{
    		float lightL = powf(1.413, i) * vuValueL / 10.0 - 1.0;
    		vuLightsL[i] = clampf(lightL, 0.0, 1.0);
    	}

    	lightsL[0] = lightFilterL.peak();

    	float lightRateR = 5.0 / engineGetSampleRate();
    	vuFilterR.setRate(lightRateL);
    	vuFilterR.process(fabsf(mix_r));
    	lightFilterR.setRate(lightRateR);
    	lightFilterR.process(fabsf(mix_r*50.0));

    	float vuValueR = vuFilterR.peak();

    	for (int i = 0; i < 7; i++) 
    	{
    		float lightR = powf(1.413, i) * vuValueR / 10.0 - 1.0;
    		vuLightsR[i] = clampf(lightR, 0.0, 1.0);
    	}

    	lightsR[0] = lightFilterR.peak();
}


PerfMixerWidget::PerfMixerWidget() {
	PerfMixer *module = new PerfMixer();
	setModule(module);
	box.size = Vec(15*32, 380);

	{
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
    	panel->setBackground(SVG::load(assetPlugin(plugin,"res/PerfMixer.svg")));
		addChild(panel);
	}

  int column_1 = 18;
  int right_column = 460;
  int top_row = 90;
  int row_spacing = 28;
  int column_spacing = 24;


	addParam(createParam<LDaviesBlu>(Vec(400, 20), module, PerfMixer::MIX_PARAM, 0.0, 1.0, 0.5)); // master volume

  	addParam(createParam<Davies1900hSmallBlackKnob>(Vec(right_column - 50, 225), module, PerfMixer::AUX_RETURN_1_PARAM, 0.0, 1.0, 0.0));
  	addParam(createParam<Davies1900hSmallBlackKnob>(Vec(right_column - 50, 295), module, PerfMixer::AUX_RETURN_2_PARAM, 0.0, 1.0, 0.0));
  
  	addParam(createParam<Davies1900hSmallBlackKnob>(Vec(right_column - 50, 100), module, PerfMixer::AUX_SEND_1_PARAM, 0.0, 1.0, 0.0));
  	addParam(createParam<Davies1900hSmallBlackKnob>(Vec(right_column - 50, 160), module, PerfMixer::AUX_SEND_2_PARAM, 0.0, 1.0, 0.0))	;

		// channel strips
		
int v=0;

  for (int i = 0 ; i < 16 ; i++)
  {
    
    if ( i%2 == 0)
      {

          addParam(createParam<SmallBlu>(Vec(column_1+column_spacing*i, 10 ), module, PerfMixer::AUX_1_PARAM + v, 0.0, 1.0, 0.0));
          addParam(createParam<SmallBlu>(Vec(column_1+column_spacing*i, 40), module, PerfMixer::AUX_2_PARAM + v, 0.0, 1.0, 0.0));


          addInput(createInput<PJ301MIPort>(Vec(column_1+column_spacing*i, top_row - 20), module, PerfMixer::CH_INPUT + i));
          addParam(createParam<SlidePot>(Vec(column_1+column_spacing*i,top_row + row_spacing*2-20), module, PerfMixer::VOL_PARAM + v, 0.0, 1.0, 0.0));

          addInput(createInput<PJ301MCPort>(Vec(column_1+column_spacing*i - 5, top_row + row_spacing *6-20), module, PerfMixer::CH_VOL_INPUT + v));
    
          addParam(createParam<SmallOra>(Vec(column_1+column_spacing*i+15, top_row + row_spacing * 3-5), module, PerfMixer::PAN_PARAM + v, 0.0, 1.0, 0.5));
          addInput(createInput<PJ301MCPort>(Vec(column_1+column_spacing*i+15, (top_row + row_spacing * 4) - 5), module, PerfMixer::CH_PAN_INPUT + v));
    
          addParam(createParam<LEDButton>(Vec(column_1+column_spacing*i+15,top_row + row_spacing * 2), module, PerfMixer::MONO_PARAM + v, 0.0, 1.0, 0.0));
          addChild(createValueLight<SmallLight<GreenValueLight>>(Vec(column_1+column_spacing*i+ 15 + 5, top_row + row_spacing * 2 + 5), &module->mono_lights[v]));


          addParam(createParam<LEDButton>(Vec(column_1+column_spacing*i,top_row + row_spacing * 7), module, PerfMixer::MUTE_PARAM + v, 0.0, 1.0, 0.0));
	        addChild(createValueLight<SmallLight<GreenValueLight>>(Vec(column_1+column_spacing*i + 5, top_row + row_spacing * 7 + 5), &module->mute_lights[v]));
          addInput(createInput<PJ301MCPort>(Vec(column_1+column_spacing*i, top_row + row_spacing * 8), module, PerfMixer::CH_MUTE_INPUT + v));
      }
      else
        {
			addInput(createInput<PJ301MIPort>(Vec(column_spacing*i - 7 , top_row + row_spacing - 20), module, PerfMixer::CH_INPUT + i));
			v=v+1;
        }
	}


//Screw

 	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 365)));

  // outputs
  	addOutput(createOutput<PJ301MLPort>(Vec(right_column - 10 , 20), module, PerfMixer::MIX_OUTPUT_L));
	addOutput(createOutput<PJ301MRPort>(Vec(right_column - 10 , 45), module, PerfMixer::MIX_OUTPUT_R));

  	addOutput(createOutput<PJ301MLPort>(Vec(right_column -10, 100), module, PerfMixer::SEND_1_L_OUTPUT));
  	addOutput(createOutput<PJ301MRPort>(Vec(right_column -10, 125), module, PerfMixer::SEND_1_R_OUTPUT));
  
  	addOutput(createOutput<PJ301MLPort>(Vec(right_column -10, 160), module, PerfMixer::SEND_2_L_OUTPUT));
  	addOutput(createOutput<PJ301MRPort>(Vec(right_column -10, 185), module, PerfMixer::SEND_2_R_OUTPUT));

  	addInput(createInput<PJ301MLPort>(Vec(right_column -10, 225), module, PerfMixer::RETURN_1_L_INPUT));
  	addInput(createInput<PJ301MRPort>(Vec(right_column -10, 250), module, PerfMixer::RETURN_1_R_INPUT));

  	addInput(createInput<PJ301MLPort>(Vec(right_column -10, 295), module, PerfMixer::RETURN_2_L_INPUT));
  	addInput(createInput<PJ301MRPort>(Vec(right_column -10, 320), module, PerfMixer::RETURN_2_R_INPUT));

	addChild(createValueLight<SmallLight<RedValueLight>>(Vec(460, 75), &module->vuLightsL[0]));
	addChild(createValueLight<SmallLight<YellowValueLight>>(Vec(450, 75), &module->vuLightsL[1]));
	addChild(createValueLight<SmallLight<YellowValueLight>>(Vec(440, 75), &module->vuLightsL[2]));
	addChild(createValueLight<SmallLight<GreenValueLight>>(Vec(430, 75), &module->vuLightsL[3]));
	addChild(createValueLight<SmallLight<GreenValueLight>>(Vec(420, 75), &module->vuLightsL[4]));
	addChild(createValueLight<SmallLight<GreenValueLight>>(Vec(410, 75), &module->vuLightsL[5]));
	addChild(createValueLight<SmallLight<GreenValueLight>>(Vec(400, 75), &module->vuLightsL[6]));

	addChild(createValueLight<SmallLight<RedValueLight>>(Vec(460, 85), &module->vuLightsR[0]));
	addChild(createValueLight<SmallLight<YellowValueLight>>(Vec(450, 85), &module->vuLightsR[1]));
	addChild(createValueLight<SmallLight<YellowValueLight>>(Vec(440, 85), &module->vuLightsR[2]));
	addChild(createValueLight<SmallLight<GreenValueLight>>(Vec(430, 85), &module->vuLightsR[3]));
	addChild(createValueLight<SmallLight<GreenValueLight>>(Vec(420, 85), &module->vuLightsR[4]));
	addChild(createValueLight<SmallLight<GreenValueLight>>(Vec(410, 85), &module->vuLightsR[5]));
	addChild(createValueLight<SmallLight<GreenValueLight>>(Vec(400, 85), &module->vuLightsR[6]));



}
