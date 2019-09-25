///////////////////////////////////////////////////
//  dBiz revisited version of 
//
//   Pitch Shifter VCV Module
//
//   Strum 2017
//
///////////////////////////////////////////////////

#include "plugin.hpp"


/////added fine out /////////////////////////////////////////////////
struct Transpose : Module {
	enum ParamIds {
    OCTAVE_SHIFT_1,
    OCTAVE_SHIFT_2,
    SEMITONE_SHIFT_1,
    SEMITONE_SHIFT_2,
    FINE_SHIFT_1,
   // FINE_SHIFT_2,
    NUM_PARAMS
	};
	enum InputIds {
		OCTAVE_SHIFT_1_INPUT,
    OCTAVE_SHIFT_2_INPUT,
    SEMITONE_SHIFT_1_INPUT,
    SEMITONE_SHIFT_2_INPUT, 
    OCTAVE_SHIFT_1_CVINPUT,
    OCTAVE_SHIFT_2_CVINPUT,
    SEMITONE_SHIFT_1_CVINPUT,
    SEMITONE_SHIFT_2_CVINPUT,
    FINE_SHIFT_1_INPUT,
  //  FINE_SHIFT_2_INPUT, 
    FINE_SHIFT_1_CVINPUT,
  //  FINE_SHIFT_2_CVINPUT,
    NUM_INPUTS
	};
	enum OutputIds {
		OCTAVE_SHIFT_1_OUTPUT,
    OCTAVE_SHIFT_2_OUTPUT,
    SEMITONE_SHIFT_1_OUTPUT,
    SEMITONE_SHIFT_2_OUTPUT,
    FINE_SHIFT_1_OUTPUT,
  //  FINE_SHIFT_2_OUTPUT,
    NUM_OUTPUTS
	};

	Transpose() 
  {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS);
    
    configParam(OCTAVE_SHIFT_1,  -4.5, 4.5, 0.0,"Octave shift");
    configParam(OCTAVE_SHIFT_2,  -4.5, 4.5, 0.0,"Octave shift");
    configParam(SEMITONE_SHIFT_1,  -6.5, 6.5, 0.0,"Semitone shift");
    configParam(SEMITONE_SHIFT_2,  -6.5, 6.5, 0.0,"Semitone shift");
    configParam(FINE_SHIFT_1,  -1, 1, 0.0,"Fine Shift");
   // configParam(FINE_SHIFT_2,  -1, 1, 0.0,"Fine Shift");
    
  }
  
  float octave_1_out = 0.0;
  float octave_2_out = 0.0;
  float semitone_1_out = 0.0;
  float semitone_2_out = 0.0;
  float fine_1_out = 0.0;
  float fine_2_out = 0.0;

  void process(const ProcessArgs &args) override 
  {
  octave_1_out = inputs[OCTAVE_SHIFT_1_INPUT].getVoltage() + round(params[OCTAVE_SHIFT_1].getValue()) + round(inputs[OCTAVE_SHIFT_1_CVINPUT].getVoltage()/2);
  octave_2_out = inputs[OCTAVE_SHIFT_2_INPUT].getVoltage() + round(params[OCTAVE_SHIFT_2].getValue()) + round(inputs[OCTAVE_SHIFT_2_CVINPUT].getVoltage()/2);
  semitone_1_out = inputs[SEMITONE_SHIFT_1_INPUT].getVoltage() + round(params[SEMITONE_SHIFT_1].getValue())*(1.0/12.0) + round(inputs[SEMITONE_SHIFT_1_CVINPUT].getVoltage()/2)*(1.0/12.0);
  semitone_2_out = inputs[SEMITONE_SHIFT_2_INPUT].getVoltage() + round(params[SEMITONE_SHIFT_2].getValue())*(1.0/12.0) + round(inputs[SEMITONE_SHIFT_2_CVINPUT].getVoltage()/2)*(1.0/12.0);
  fine_1_out = inputs[FINE_SHIFT_1_INPUT].getVoltage() + (params[FINE_SHIFT_1].getValue())*(1.0/12.0) + (inputs[FINE_SHIFT_1_CVINPUT].getVoltage()/2)*(1.0/2.0);
 // fine_2_out = inputs[FINE_SHIFT_2_INPUT].getVoltage() + (params[FINE_SHIFT_2].getValue())*(1.0/12.0) + (inputs[FINE_SHIFT_2_CVINPUT].getVoltage()/2)*(1.0/2.0);

  outputs[OCTAVE_SHIFT_1_OUTPUT].setVoltage(octave_1_out);
  outputs[OCTAVE_SHIFT_2_OUTPUT].setVoltage(octave_2_out);
  outputs[SEMITONE_SHIFT_1_OUTPUT].setVoltage(semitone_1_out);
  outputs[SEMITONE_SHIFT_2_OUTPUT].setVoltage(semitone_2_out);
  outputs[FINE_SHIFT_1_OUTPUT].setVoltage(fine_1_out);
 // outputs[FINE_SHIFT_2_OUTPUT].setVoltage(fine_2_out);

}
};

//////////////////////////////////////////////////////////////////
struct TransposeWidget : ModuleWidget {
TransposeWidget(Transpose *module){
   setModule(module);
   setPanel(APP->window->loadSvg(asset::plugin(pluginInstance,  "res/Transpose.svg")));

   //Screw

   addChild(createWidget<ScrewBlack>(Vec(15, 0)));
   addChild(createWidget<ScrewBlack>(Vec(15, 365)));

   //

   addParam(createParam<FlatASnap>(Vec(2, 15), module, Transpose::OCTAVE_SHIFT_1));
   addParam(createParam<FlatASnap>(Vec(2, 75), module, Transpose::OCTAVE_SHIFT_2));
   addParam(createParam<FlatGSnap>(Vec(2, 135), module, Transpose::SEMITONE_SHIFT_1));
   addParam(createParam<FlatGSnap>(Vec(2, 195), module, Transpose::SEMITONE_SHIFT_2));
   addParam(createParam<FlatR>(Vec(2, 255), module, Transpose::FINE_SHIFT_1));
   //addParam(createParam<FlatR>(Vec(2, 315), module, Transpose::FINE_SHIFT_2));

   addInput(createInput<PJ301MIPort>(Vec(3, 2 + 45),  module, Transpose::OCTAVE_SHIFT_1_INPUT));
   addInput(createInput<PJ301MIPort>(Vec(3, 2 + 105), module, Transpose::OCTAVE_SHIFT_2_INPUT));
   addInput(createInput<PJ301MIPort>(Vec(3, 2 + 165), module, Transpose::SEMITONE_SHIFT_1_INPUT));
   addInput(createInput<PJ301MIPort>(Vec(3, 2 + 225), module, Transpose::SEMITONE_SHIFT_2_INPUT));
   addInput(createInput<PJ301MIPort>(Vec(3, 2 + 285), module, Transpose::FINE_SHIFT_1_INPUT));
  // addInput(createInput<PJ301MIPort>(Vec(3, 2 + 345), module, Transpose::FINE_SHIFT_2_INPUT));

   addInput(createInput<PJ301MCPort>(Vec(33, 15),  module, Transpose::OCTAVE_SHIFT_1_CVINPUT));
   addInput(createInput<PJ301MCPort>(Vec(33, 75),  module, Transpose::OCTAVE_SHIFT_2_CVINPUT));
   addInput(createInput<PJ301MCPort>(Vec(33, 135), module, Transpose::SEMITONE_SHIFT_1_CVINPUT));
   addInput(createInput<PJ301MCPort>(Vec(33, 195), module, Transpose::SEMITONE_SHIFT_2_CVINPUT));
   addInput(createInput<PJ301MCPort>(Vec(33, 255), module, Transpose::FINE_SHIFT_1_CVINPUT));
  // addInput(createInput<PJ301MCPort>(Vec(33, 315), module, Transpose::FINE_SHIFT_2_CVINPUT));

   addOutput(createOutput<PJ301MOPort>(Vec(33, 2 + 45), module, Transpose::OCTAVE_SHIFT_1_OUTPUT));
   addOutput(createOutput<PJ301MOPort>(Vec(33, 2 + 105),module, Transpose::OCTAVE_SHIFT_2_OUTPUT));
   addOutput(createOutput<PJ301MOPort>(Vec(33, 2 + 165),module, Transpose::SEMITONE_SHIFT_1_OUTPUT));
   addOutput(createOutput<PJ301MOPort>(Vec(33, 2 + 225),module, Transpose::SEMITONE_SHIFT_2_OUTPUT));
   addOutput(createOutput<PJ301MOPort>(Vec(33, 2 + 285),module, Transpose::FINE_SHIFT_1_OUTPUT));
  // addOutput(createOutput<PJ301MOPort>(Vec(33, 2 + 345),module, Transpose::FINE_SHIFT_2_OUTPUT));

}
};
Model *modelTranspose = createModel<Transpose, TransposeWidget>("Transpose");
