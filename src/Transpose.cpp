///////////////////////////////////////////////////
//  dBiz revisited version of 
//
//   Pitch Shifter VCV Module
//
//   Strum 2017
//
///////////////////////////////////////////////////

#include "dBiz.hpp"


//////////////////////////////////////////////////////
struct Transpose : Module {
	enum ParamIds {
    OCTAVE_SHIFT_1,
    OCTAVE_SHIFT_2,
    SEMITONE_SHIFT_1,
    SEMITONE_SHIFT_2,    
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
    NUM_INPUTS
	};
	enum OutputIds {
		OCTAVE_SHIFT_1_OUTPUT,
    OCTAVE_SHIFT_2_OUTPUT,
    SEMITONE_SHIFT_1_OUTPUT,
    SEMITONE_SHIFT_2_OUTPUT, 
    NUM_OUTPUTS
	};

	Transpose() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS) {}
  
  float octave_1_out = 0.0;
  float octave_2_out = 0.0;
  float semitone_1_out = 0.0;
  float semitone_2_out = 0.0;
  
  
	void step() override;
};


/////////////////////////////////////////////////////
void Transpose::step() {

  octave_1_out = inputs[OCTAVE_SHIFT_1_INPUT].value + round(params[OCTAVE_SHIFT_1].value) + round(inputs[OCTAVE_SHIFT_1_CVINPUT].value/2);
  octave_2_out = inputs[OCTAVE_SHIFT_2_INPUT].value + round(params[OCTAVE_SHIFT_2].value) + round(inputs[OCTAVE_SHIFT_1_CVINPUT].value/2);
  semitone_1_out = inputs[SEMITONE_SHIFT_1_INPUT].value + round(params[SEMITONE_SHIFT_1].value)*(1.0/12.0) + round(inputs[SEMITONE_SHIFT_1_CVINPUT].value/2)*(1.0/12.0);
  semitone_2_out = inputs[SEMITONE_SHIFT_2_INPUT].value + round(params[SEMITONE_SHIFT_2].value)*(1.0/12.0) + round(inputs[SEMITONE_SHIFT_2_CVINPUT].value/2)*(1.0/12.0);
    
  outputs[OCTAVE_SHIFT_1_OUTPUT].value = octave_1_out;
  outputs[OCTAVE_SHIFT_2_OUTPUT].value = octave_2_out;
  outputs[SEMITONE_SHIFT_1_OUTPUT].value = semitone_1_out;
  outputs[SEMITONE_SHIFT_2_OUTPUT].value = semitone_2_out;

}

//////////////////////////////////////////////////////////////////
TransposeWidget::TransposeWidget() {
	Transpose *module = new Transpose();
	setModule(module);
	box.size = Vec(15*4, 380);

	{
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		//panel->setBackground(SVG::load("plugins/mental/res/Transpose.svg"));
    panel->setBackground(SVG::load(assetPlugin(plugin,"res/Transpose.svg")));
		addChild(panel);
	}

//Screw

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	

  addParam(createParam<SmallOra>(Vec(2, 20), module, Transpose::OCTAVE_SHIFT_1, -4.5, 4.5, 0.0));
  addParam(createParam<SmallOra>(Vec(2, 80), module, Transpose::OCTAVE_SHIFT_2, -4.5, 4.5, 0.0));
  addParam(createParam<SmallVio>(Vec(2, 150), module, Transpose::SEMITONE_SHIFT_1, -6.5, 6.5, 0.0));
  addParam(createParam<SmallVio>(Vec(2, 210), module, Transpose::SEMITONE_SHIFT_2, -6.5, 6.5, 0.0));

  addInput(createInput<PJ301MIPort>(Vec(3, 50), module, Transpose::OCTAVE_SHIFT_1_INPUT));
	addInput(createInput<PJ301MIPort>(Vec(3, 110), module, Transpose::OCTAVE_SHIFT_2_INPUT));
  addInput(createInput<PJ301MIPort>(Vec(3, 180), module, Transpose::SEMITONE_SHIFT_1_INPUT));
	addInput(createInput<PJ301MIPort>(Vec(3, 240), module, Transpose::SEMITONE_SHIFT_2_INPUT));
  
  addInput(createInput<PJ301MCPort>(Vec(33, 20), module, Transpose::OCTAVE_SHIFT_1_CVINPUT));
	addInput(createInput<PJ301MCPort>(Vec(33, 80), module, Transpose::OCTAVE_SHIFT_2_CVINPUT));
  addInput(createInput<PJ301MCPort>(Vec(33, 150), module, Transpose::SEMITONE_SHIFT_1_CVINPUT));
	addInput(createInput<PJ301MCPort>(Vec(33, 210), module, Transpose::SEMITONE_SHIFT_2_CVINPUT));

  addOutput(createOutput<PJ301MOPort>(Vec(33, 50), module, Transpose::OCTAVE_SHIFT_1_OUTPUT));
  addOutput(createOutput<PJ301MOPort>(Vec(33, 110), module, Transpose::OCTAVE_SHIFT_2_OUTPUT));
  addOutput(createOutput<PJ301MOPort>(Vec(33, 180), module, Transpose::SEMITONE_SHIFT_1_OUTPUT));
  addOutput(createOutput<PJ301MOPort>(Vec(33, 240), module, Transpose::SEMITONE_SHIFT_2_OUTPUT));

}
