///////////////////////////////////////////////////
//  dBiz revisited version of 
//
//   Chord Creator VCV Module
//
//   Strum 2017
//
///////////////////////////////////////////////////

#include "plugin.hpp"


/////////////////////////////////////////////////
struct Chord : Module {
	enum ParamIds {
      OFFSET_PARAM,
      INVERSION_PARAM,
      VOICING_PARAM,
      OFFSET_AMT_PARAM,
      INVERSION_AMT_PARAM,
      VOICING_AMT_PARAM,
      FLAT_3RD_PARAM,
      FLAT_5TH_PARAM,
      FLAT_7TH_PARAM,
      SUS_2_PARAM,
      SUS_4_PARAM,
      SIX_FOR_5_PARAM,
      ONE_FOR_7_PARAM,
      FLAT_9_PARAM,
      SHARP_9_PARAM,
      SIX_FOR_7_PARAM,
      SHARP_5_PARAM,
      NUM_PARAMS
	};

	enum InputIds {
      INPUT,
      OFFSET_CV_INPUT,
      INVERSION_CV_INPUT,
      VOICING_CV_INPUT,
      FLAT_3RD_INPUT,
      FLAT_5TH_INPUT,
      FLAT_7TH_INPUT,
      SUS_2_INPUT,
      SUS_4_INPUT,
      SIX_FOR_5_INPUT,
      ONE_FOR_7_INPUT,
      FLAT_9_INPUT,
      SHARP_9_INPUT,
      SIX_FOR_7_INPUT,
      SHARP_5_INPUT,      
      NUM_INPUTS
	};
	enum OutputIds {
      OUTPUT_1,
      OUTPUT_2,
      OUTPUT_3,
      OUTPUT_4,
      OUTPUT_ROOT,
      OUTPUT_THIRD,
      OUTPUT_FIFTH,
      OUTPUT_SEVENTH,
      NUM_OUTPUTS
	};

  enum LighIds {
      FLAT_3RD_LIGHT,
      FLAT_5TH_LIGHT,
      FLAT_7TH_LIGHT,
      SUS_2_LIGHT,
      SUS_4_LIGHT,
      SIX_FOR_5_LIGHT,
      ONE_FOR_7_LIGHT,
      FLAT_9_LIGHT,
      SHARP_9_LIGHT,
      SIX_FOR_7_LIGHT,
      SHARP_5_LIGHT,
      OUT_1_LIGHT,
      OUT_2_LIGHT,
      OUT_3_LIGHT,
      OUT_4_LIGHT,
      NUM_LIGHTS

  };

  
	Chord() {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

    configParam(OFFSET_PARAM, 0.0, 1.0, 0.5,"Offset");
    configParam(INVERSION_PARAM, 0.0, 1.0, 0.0,"Inversion");
    configParam(VOICING_PARAM, 0.0, 1.0, 0.0,"Voicing");
    configParam(OFFSET_AMT_PARAM, 0.0, 1.0, 0.5,"Offset Amt");
    configParam(INVERSION_AMT_PARAM, 0.0, 1.0, 0.0,"Inversion Amt");
    configParam(VOICING_AMT_PARAM, 0.0, 1.0, 0.0,"Voicing Amt");
    configParam(FLAT_3RD_PARAM, 0.0,1.0,0.0,"b3");
    configParam(FLAT_5TH_PARAM, 0.0,1.0,0.0,"b5");
    configParam(FLAT_7TH_PARAM, 0.0,1.0,0.0,"b7");
    configParam(SUS_2_PARAM, 0.0,1.0,0.0,"sus2");
    configParam(SUS_4_PARAM, 0.0,1.0,0.0,"sus4");
    configParam(SIX_FOR_5_PARAM, 0.0,1.0,0.0,"add6");
    configParam(ONE_FOR_7_PARAM, 0.0,1.0,0.0,"1for7");
    configParam(FLAT_9_PARAM, 0.0,1.0,0.0,"b9");
    configParam(SHARP_9_PARAM, 0.0,1.0,0.0,"#9");
    configParam(SIX_FOR_7_PARAM, 0.0,1.0,0.0,"bb7");
    configParam(SHARP_5_PARAM, 0.0,1.0,0.0,"#5");
  }

	void process(const ProcessArgs &args) override 
  {

  float in = inputs[INPUT].getVoltage();  
  int octave = round(in);
  
  float offset_raw = (params[OFFSET_PARAM].getValue()) * 12 - 6 + (inputs[OFFSET_CV_INPUT].getVoltage()*params[OFFSET_AMT_PARAM].getValue()) / 1.5;
  float pitch_offset = round(offset_raw) / 12;
  
  float root = in - 1.0*octave + pitch_offset;
  float root_or_2nd = root;
  
  float inversion_raw = (params[INVERSION_PARAM].getValue()) * 4 - 1 + ((inputs[INVERSION_CV_INPUT].getVoltage()*params[INVERSION_AMT_PARAM].getValue()) / 3);
  int inversion = round(inversion_raw);
  if (inversion > 2) inversion = 2;
  if (inversion < -1) inversion = -1;
  
  float voicing_raw = (params[VOICING_PARAM].getValue()) * 5 - 2 + ((inputs[VOICING_CV_INPUT].getVoltage()*params[VOICING_AMT_PARAM].getValue()) / 3);
  int voicing = round(voicing_raw);
  if (voicing > 2) voicing = 2;
  if (voicing < -2) voicing = -2;
  
  
  float voice_1 = 0.0;
  float voice_2 = 0.0;
  float voice_3 = 0.0;
  float voice_4 = 0.0;
  
  int third = 4;
  int fifth = 7;
  int seventh = 11;
    
  if (inputs[FLAT_3RD_INPUT].getVoltage()+params[FLAT_3RD_PARAM].getValue() > 0.0)
  {
     third = 3;
     lights[FLAT_3RD_LIGHT].setBrightness(1.0f);
  }
  else lights[FLAT_3RD_LIGHT].setBrightness(0.0);

  if (inputs[FLAT_5TH_INPUT].getVoltage()+params[FLAT_5TH_PARAM].getValue() > 0.0)
  {
     fifth = 6;
     lights[FLAT_5TH_LIGHT].setBrightness(1.0);
  }
  else lights[FLAT_5TH_LIGHT].setBrightness(0.0);

  if (inputs[SHARP_5_INPUT].getVoltage()+params[SHARP_5_PARAM].getValue() > 0.0)
  {
     fifth = 8;
     lights[SHARP_5_LIGHT].setBrightness(1.0);
  }
  else lights[SHARP_5_LIGHT].setBrightness(0.0);
  

  if (inputs[FLAT_7TH_INPUT].getVoltage()+params[FLAT_7TH_PARAM].getValue() > 0.0)
  {
     seventh = 10;
     lights[FLAT_7TH_LIGHT].setBrightness(1.0);
  }
  else lights[FLAT_7TH_LIGHT].setBrightness(0.0);

  if (inputs[SUS_2_INPUT].getVoltage()+params[SUS_2_PARAM].getValue() > 0.0)
  {
     root_or_2nd = root + (2 * (1.0/12.0));
     lights[SUS_2_LIGHT].setBrightness(1.0);
  }
  else lights[SUS_2_LIGHT].setBrightness(0.0);

  if (inputs[SUS_4_INPUT].getVoltage()+params[SUS_4_PARAM].getValue() > 0.0)
  {
     third = 5;
     lights[SUS_4_LIGHT].setBrightness(1.0);
  }
  else lights[SUS_4_LIGHT].setBrightness(0.0);

  if (inputs[SIX_FOR_5_INPUT].getVoltage()+params[SIX_FOR_5_PARAM].getValue() > 0.0)
  {
     fifth = 9;
     lights[SIX_FOR_5_LIGHT].setBrightness(1.0);
  }
  else lights[SIX_FOR_5_LIGHT].setBrightness(0.0);

  if (inputs[SIX_FOR_7_INPUT].getVoltage()+params[SIX_FOR_7_PARAM].getValue() > 0.0)
  {
     seventh = 9;
     lights[SIX_FOR_7_LIGHT].setBrightness(1.0);
  }
  else lights[SIX_FOR_7_LIGHT].setBrightness(0.0);

  
  if (inputs[FLAT_9_INPUT].getVoltage()+params[FLAT_9_PARAM].getValue() > 0.0)
  {
     root_or_2nd = root + 1.0/12.0;
     lights[FLAT_9_LIGHT].setBrightness(1.0);
  }
  else lights[FLAT_9_LIGHT].setBrightness(0.0);

  if (inputs[SHARP_9_INPUT].getVoltage()+params[SHARP_9_PARAM].getValue() > 0.0)
  {
     root_or_2nd = root + (3 * (1.0/12.0));
     lights[SHARP_9_LIGHT].setBrightness(1.0);
  }
  else lights[SHARP_9_LIGHT].setBrightness(0.0);

  if (inputs[ONE_FOR_7_INPUT].getVoltage()+params[ONE_FOR_7_PARAM].getValue() > 0.0)
  {
     seventh = 12;
     lights[ONE_FOR_7_LIGHT].setBrightness(1.0);
  }
  else lights[ONE_FOR_7_LIGHT].setBrightness(0.0);

     outputs[OUTPUT_ROOT].setVoltage(root);
    outputs[OUTPUT_THIRD].setVoltage(root + third * (1.0/12.0));
    outputs[OUTPUT_FIFTH].setVoltage(root + fifth * (1.0/12.0));
  outputs[OUTPUT_SEVENTH].setVoltage(root + seventh * (1.0/12.0));
  
  
  
  if (inversion == -1 )
  {
    voice_1 = root_or_2nd;
    voice_2 = root + third * (1.0/12.0);
    voice_3 = root + fifth * (1.0/12.0);
    voice_4 = root + seventh * (1.0/12.0);
  }
  if (inversion == 0 )
  {
    voice_1 = root + third * (1.0/12.0);
    voice_2 = root + fifth * (1.0/12.0);
    voice_3 = root + seventh * (1.0/12.0);
    voice_4 = root_or_2nd + 1.0;
  }
  if (inversion == 1)
  {
    voice_1 = root + fifth * (1.0/12.0);
    voice_2 = root + seventh * (1.0/12.0);
    voice_3 = root_or_2nd + 1.0;
    voice_4 = root + 1.0 + third * (1.0/12.0);
  }
  if (inversion == 2 )
  {
    voice_1 = root + seventh * (1.0/12.0);
    voice_2 = root_or_2nd + 1.0;
    voice_3 = root + 1.0 + third * (1.0/12.0);
    voice_4 = root + 1.0 + fifth * (1.0/12.0);
  }
  
  if (voicing == -1) voice_2 -= 1.0;
  if (voicing == -0) voice_3 -= 1.0;
  if (voicing == 1)
  {
    voice_2 -= 1.0;
    voice_4 -= 1.0;
  }
  if (voicing == 2)
  {
    voice_2 += 1.0;
    voice_4 += 1.0;
  }
  
  
  outputs[OUTPUT_1].setVoltage(voice_1);
  outputs[OUTPUT_2].setVoltage(voice_2);
  outputs[OUTPUT_3].setVoltage(voice_3);
  outputs[OUTPUT_4].setVoltage(voice_4);
}
};

template <typename BASE>
struct ChordLight : BASE
{
  ChordLight()
  {
    this->box.size = mm2px(Vec(5, 5));
  }
};
//////////////////////////////////////////////////////////////////
struct ChordWidget : ModuleWidget 
{
ChordWidget(Chord *module){
  setModule(module);
  setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Chord.svg")));


  //
  addChild(createWidget<ScrewBlack>(Vec(15, 0)));
  addChild(createWidget<ScrewBlack>(Vec(box.size.x-30, 0)));
  addChild(createWidget<ScrewBlack>(Vec(15, 365)));
  addChild(createWidget<ScrewBlack>(Vec(box.size.x-30, 365)));
//
int jacks = 27;
int pot=22;
int off =2.5;
int space = 40;

  addInput(createInput<PJ301MCPort>(Vec(off,60+jacks*1), module, Chord::OFFSET_CV_INPUT));
  addInput(createInput<PJ301MCPort>(Vec(off,60+jacks*2), module, Chord::INVERSION_CV_INPUT));
  addInput(createInput<PJ301MCPort>(Vec(off,60+jacks*3), module, Chord::VOICING_CV_INPUT));

  addParam(createParam<Trimpot>(Vec(off*2,pot*1), module, Chord::OFFSET_AMT_PARAM));
  addParam(createParam<Trimpot>(Vec(off*2,pot*2), module, Chord::INVERSION_AMT_PARAM));
  addParam(createParam<Trimpot>(Vec(off*2,pot*3), module, Chord::VOICING_AMT_PARAM));

  

  addParam(createParam<FlatG>(Vec(off + 30 ,space*1-15), module, Chord::OFFSET_PARAM));
  addParam(createParam<FlatA>(Vec(off + 30 ,space*2-15), module, Chord::INVERSION_PARAM));
  addParam(createParam<FlatR>(Vec(off + 30 ,space*3-15), module, Chord::VOICING_PARAM));

//

int right = 95 ;
int left = 30;

  addInput(createInput<PJ301MIPort>(Vec(left, 180), module, Chord::FLAT_3RD_INPUT));
  addInput(createInput<PJ301MIPort>(Vec(left, 180+jacks*1), module, Chord::FLAT_5TH_INPUT));
  addInput(createInput<PJ301MIPort>(Vec(left, 180+jacks*2), module, Chord::FLAT_7TH_INPUT));
  addInput(createInput<PJ301MIPort>(Vec(left, 180+jacks*3), module, Chord::SIX_FOR_7_INPUT));
  addInput(createInput<PJ301MIPort>(Vec(left, 180+jacks*4), module, Chord::SHARP_5_INPUT));

  //
  addInput(createInput<PJ301MIPort>(Vec(right, 180), module, Chord::SUS_2_INPUT));
  addInput(createInput<PJ301MIPort>(Vec(right, 180+jacks*1), module, Chord::SUS_4_INPUT));
  addInput(createInput<PJ301MIPort>(Vec(right, 180+jacks*2), module, Chord::SIX_FOR_5_INPUT));
  addInput(createInput<PJ301MIPort>(Vec(right, 180+jacks*3), module, Chord::ONE_FOR_7_INPUT));
  addInput(createInput<PJ301MIPort>(Vec(right, 180+jacks*4), module, Chord::FLAT_9_INPUT));
  addInput(createInput<PJ301MIPort>(Vec(right, 180+jacks*5), module, Chord::SHARP_9_INPUT));
//


  addParam(createParam<LEDT>(Vec(-22 + left, 3+ 180), module, Chord::FLAT_3RD_PARAM));
  addParam(createParam<LEDT>(Vec(-22 + left, 3+ 180+jacks*1), module, Chord::FLAT_5TH_PARAM));
  addParam(createParam<LEDT>(Vec(-22 + left, 3+ 180+jacks*2), module, Chord::FLAT_7TH_PARAM));
  addParam(createParam<LEDT>(Vec(-22 + left, 3+ 180+jacks*3), module, Chord::SIX_FOR_7_PARAM));
  addParam(createParam<LEDT>(Vec(-22 + left, 3+ 180+jacks*4), module, Chord::SHARP_5_PARAM));
  addParam(createParam<LEDT>(Vec(-22 + right,3+  180), module, Chord::SUS_2_PARAM));
  addParam(createParam<LEDT>(Vec(-22 + right,3+  180+jacks*1), module, Chord::SUS_4_PARAM));
  addParam(createParam<LEDT>(Vec(-22 + right,3+  180+jacks*2), module, Chord::SIX_FOR_5_PARAM));
  addParam(createParam<LEDT>(Vec(-22 + right,3+  180+jacks*3), module, Chord::ONE_FOR_7_PARAM));
  addParam(createParam<LEDT>(Vec(-22 + right,3+  180+jacks*4), module, Chord::FLAT_9_PARAM));
  addParam(createParam<LEDT>(Vec(-22 + right,3+  180+jacks*5), module, Chord::SHARP_9_PARAM));




  //

  addChild(createLight<ChordLight<OrangeLight>>(Vec(left-19.5,185.5), module, Chord::FLAT_3RD_LIGHT));
  addChild(createLight<ChordLight<OrangeLight>>(Vec(left-19.5,185.5+jacks*1), module, Chord::FLAT_5TH_LIGHT));
  addChild(createLight<ChordLight<OrangeLight>>(Vec(left-19.5,185.5+jacks*2), module, Chord::FLAT_7TH_LIGHT));
  addChild(createLight<ChordLight<OrangeLight>>(Vec(left-19.5,185.5+jacks*3), module, Chord::SIX_FOR_7_LIGHT));
  addChild(createLight<ChordLight<OrangeLight>>(Vec(left-19.5,185.5+jacks*4), module, Chord::SHARP_5_LIGHT));
  addChild(createLight<ChordLight<OrangeLight>>(Vec(right-19.5,185.5), module, Chord::SUS_2_LIGHT));
  addChild(createLight<ChordLight<OrangeLight>>(Vec(right-19.5,185.5+jacks*1), module, Chord::SUS_4_LIGHT));
  addChild(createLight<ChordLight<OrangeLight>>(Vec(right-19.5,185.5+jacks*2), module, Chord::SIX_FOR_5_LIGHT));
  addChild(createLight<ChordLight<OrangeLight>>(Vec(right-19.5,185.5+jacks*3), module, Chord::ONE_FOR_7_LIGHT));
  addChild(createLight<ChordLight<OrangeLight>>(Vec(right-19.5,185.5+jacks*4), module, Chord::FLAT_9_LIGHT));
  addChild(createLight<ChordLight<OrangeLight>>(Vec(right-19.5,185.5+jacks*5), module, Chord::SHARP_9_LIGHT));
  //

  //

  addOutput(createOutput<PJ301MOPort>(Vec(70,jacks*1), module, Chord::OUTPUT_ROOT));
  addOutput(createOutput<PJ301MOPort>(Vec(70,jacks*2), module, Chord::OUTPUT_THIRD));
  addOutput(createOutput<PJ301MOPort>(Vec(70,jacks*3), module, Chord::OUTPUT_FIFTH));
  addOutput(createOutput<PJ301MOPort>(Vec(70,jacks*4), module, Chord::OUTPUT_SEVENTH));  
    
  addOutput(createOutput<PJ301MOPort>(Vec(97,jacks*1 ), module, Chord::OUTPUT_1));
  addOutput(createOutput<PJ301MOPort>(Vec(97,jacks*2 ), module, Chord::OUTPUT_2));
  addOutput(createOutput<PJ301MOPort>(Vec(97,jacks*3 ), module, Chord::OUTPUT_3));
  addOutput(createOutput<PJ301MOPort>(Vec(97,jacks*4 ), module, Chord::OUTPUT_4));

  addInput(createInput<PJ301MIPort>(Vec(97,57+jacks*3), module, Chord::INPUT));

}
};
Model *modelChord = createModel<Chord, ChordWidget>("Chord");

