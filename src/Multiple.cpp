#include "dBiz.hpp"

struct Multiple : Module {
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		A_INPUT,
		B_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		A1_OUTPUT,
		A2_OUTPUT,
		A3_OUTPUT,
		B1_OUTPUT,
		B2_OUTPUT,
		B3_OUTPUT,
		NUM_OUTPUTS
	};

	Multiple()  : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS) {
	}
	void step() override;
};

void Multiple::step() 
{
	float in1 = inputs[A_INPUT].normalize(0.0);
	float in2 = inputs[B_INPUT].normalize(0.0);
	
	outputs[A1_OUTPUT].value = in1;
	outputs[A2_OUTPUT].value = in1;
	outputs[A3_OUTPUT].value = in1;
	outputs[B1_OUTPUT].value = in2;
	outputs[B2_OUTPUT].value = in2;
	outputs[B3_OUTPUT].value = in2;


}

MultipleWidget::MultipleWidget() 
{
	Multiple *module = new Multiple();
	setModule(module);
	box.size = Vec(15 * 2, 380);

	{
		Panel *panel = new LightPanel();
		panel->backgroundImage = Image::load("plugins/dBiz/res/Multiple.png");
		panel->box.size = box.size;
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15,   0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));

	addInput (createInput<PJ301MIPort> (Vec(3,  18), module, Multiple::A_INPUT));
	addOutput(createOutput<PJ301MOPort>(Vec(3,  58), module, Multiple::A1_OUTPUT));
	addOutput(createOutput<PJ301MOPort>(Vec(3,  98), module, Multiple::A2_OUTPUT));
	addOutput(createOutput<PJ301MOPort>(Vec(3, 138), module, Multiple::A3_OUTPUT));
	addInput (createInput<PJ301MIPort> (Vec(3,  178), module, Multiple::B_INPUT));
	addOutput(createOutput<PJ301MOPort>(Vec(3, 218), module, Multiple::B1_OUTPUT));
	addOutput(createOutput<PJ301MOPort>(Vec(3, 258), module, Multiple::B2_OUTPUT));
	addOutput(createOutput<PJ301MOPort>(Vec(3, 298), module, Multiple::B3_OUTPUT));
	
}
