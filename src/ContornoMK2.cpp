///////////////////////////////////////////
//
//  Based on Befaco Rampage
//
//
/////////////////////////////////////////////

#include "plugin.hpp"

static float shapeDelta(float delta, float tau, float shape) {
	float lin = sgn(delta) * 10.f / tau;
	if (shape < 0.f) {
		float log = sgn(delta) * 40.f / tau / (std::fabs(delta) + 1.f);
		return crossfade(lin, log, -shape * 0.95f);
	}
	else {
		float exp = M_E * delta / tau;
		return crossfade(lin, exp, shape * 0.90f);
	}
}

struct ContornoMK2 : Module {
    enum ParamIds
    {
        B_AMOUNT_PARAM,
        C_AMOUNT_PARAM,
        ENUMS(TRIG_EXT_PARAM, 2),
        ENUMS(MODE_PARAM, 2),
        ENUMS(RANGE_PARAM, 4),
        ENUMS(TRIGG_PARAM, 4),
        ENUMS(CYCLE_PARAM, 4),
        ENUMS(SHAPE_PARAM, 4),
        ENUMS(RISE_PARAM, 4),
        ENUMS(FALL_PARAM, 4),
        NUM_PARAMS
    };
    enum InputIds
    {
        ENUMS(EXT_TRIGG_INPUT, 2),
        ENUMS(TRIGG_INPUT, 4),
        ENUMS(CYCLE_INPUT, 4),
        ENUMS(RISE_INPUT, 4),
        ENUMS(FALL_INPUT, 4),
        ENUMS(IN_INPUT, 4),
        NUM_INPUTS
    };
    enum OutputIds
    {
        ENUMS(EOC_OUTPUT, 4),
        ENUMS(OUT_OUTPUT, 4),
        ENUMS(SUM_OUTPUT, 2),
        ENUMS(OR_OUTPUT, 2),

        NUM_OUTPUTS
    };

    enum LightIds
	{
		ENUMS(TRIGG_LIGHT, 4),
		ENUMS(CYCLE_LIGHT, 4),
		ENUMS(RISE_LIGHT, 4),
		ENUMS(FALL_LIGHT, 4),
		NUM_LIGHTS
	};

	float out[4] {};
	bool gate[4] = {};

	dsp::SchmittTrigger trigger[4];
    dsp::SchmittTrigger ext_trigger[2];
    dsp::PulseGenerator endOfCyclePulse[4];

	ContornoMK2() 
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		for (int i = 0; i < 4; i++)
		{
			configParam(RANGE_PARAM + i,  0.0, 2.0, 0.0, "range");
			configParam(TRIGG_PARAM + i,  0.0, 1.0, 0.0, "trig");
			configParam(CYCLE_PARAM + i,  0.0, 1.0, 0.0, "Cycle");
			configParam(SHAPE_PARAM + i,  -1.0, 1.0, 0.0, "Shape");
			configParam(RISE_PARAM + i,  0.0, 1.0, 0.0, "Raise");
			configParam(FALL_PARAM + i,  0.0, 1.0, 0.0, "Fall");
		}
        for (int i = 0; i < 2; i++)
        {
            configParam(MODE_PARAM + i, 0.0, 1.0, 0.0, "Logic Mode");
            configParam(TRIG_EXT_PARAM + i, 0.0, 1.0, 0.0, "External Trigger Mode");
        }
        configParam(B_AMOUNT_PARAM, 0.0, 1.0, 0.0, "B Level Amount");
        configParam(C_AMOUNT_PARAM, 0.0, 1.0, 0.0, "C Level Amount");
    }

	void process(const ProcessArgs &args) override 
	{

    	if (ext_trigger[0].process(params[TRIG_EXT_PARAM].getValue()*10+inputs[EXT_TRIGG_INPUT+0].getVoltage()))
		{
			 gate[0]=true;
			 gate[1] = true;
		}

    	if (ext_trigger[1].process(params[TRIG_EXT_PARAM+1].getValue()*10+inputs[EXT_TRIGG_INPUT+1].getVoltage())) 
		{
			gate[2]=true;
			gate[3] = true;
		}
	

	for (int c=0;c<4;c++)
	{
	
	    float in = inputs[IN_INPUT + c].getVoltage();

        if (trigger[c].process(params[TRIGG_PARAM + c].getValue() * 10.0 + inputs[TRIGG_INPUT + c].getVoltage()))
        {
            gate[c] = true;
        }
		if (gate[c]) {
			in = 10.0;
		}

		float shape = params[SHAPE_PARAM + c].getValue();
		float delta = in - out[c];

		// Integrator
		float minTime;
		switch ((int) params[RANGE_PARAM + c].getValue()) {
			case 0: minTime = 1e-2; break;
			case 1: minTime = 1e-3; break;
			default: minTime = 1e-1; break;
		}

		bool rising = false;
		bool falling = false;

		if (delta > 0) {
			// Rise
			float riseCv = params[RISE_PARAM + c].getValue() + inputs[RISE_INPUT + c].getVoltage() / 10.0;
			riseCv = clamp(riseCv, 0.0, 1.0);
			float rise = minTime * powf(2.0, riseCv * 10.0);
			out[c] += shapeDelta(delta, rise, shape) * args.sampleTime;
			rising = (in - out[c] > 1e-3);
			if (!rising) {
				gate[c] = false;
			}
		}
		else if (delta < 0) {
			// Fall
			float fallCv = params[FALL_PARAM + c].getValue() + inputs[FALL_INPUT + c].getVoltage() / 10.0;
			fallCv = clamp(fallCv, 0.0, 1.0);
			float fall = minTime * powf(2.0, fallCv * 10.0);
			out[c] += shapeDelta(delta, fall, shape) * args.sampleTime;
			falling = (in - out[c] < -1e-3);
			if (!falling) {
				// End of cycle, check if we should turn the gate back on (cycle mode)
				endOfCyclePulse[c].trigger(1e-3);
				if (params[CYCLE_PARAM + c].getValue() * 10.0 + inputs[CYCLE_INPUT + c].getVoltage() >= 4.0) {
					gate[c] = true;
				}
			}
		}
		else {
			gate[c] = false;
			lights[CYCLE_LIGHT+c].setBrightness(0.0);
		}

		if (!rising && !falling) {
			out[c] = in;
		}

		if (params[CYCLE_PARAM + c].getValue() == 1.0 || inputs[CYCLE_INPUT+c].getVoltage()>0.0) lights[CYCLE_LIGHT + c].setBrightness(1.0);
		

		lights[RISE_LIGHT + c].setSmoothBrightness(rising ? 1.0 : 0.0, args.sampleTime);
		lights[FALL_LIGHT + c].setSmoothBrightness(falling ? 1.0 : 0.0, args.sampleTime);
		outputs[OUT_OUTPUT + c].setVoltage(out[c]);
        outputs[EOC_OUTPUT + c].setVoltage(endOfCyclePulse[c].process(args.sampleTime) ? 10.0 : 0.0);

		if(c==0)
		{
			outputs[SUM_OUTPUT].setVoltage(out[c]+(out[c+1]*params[B_AMOUNT_PARAM].getValue()));
			if(params[MODE_PARAM].value==0)outputs[OR_OUTPUT].setVoltage(out[c]>out[c+1]?out[c]:out[c+1]);
			else outputs[OR_OUTPUT].setVoltage(out[c]>out[c+1]?out[c+1]:out[c]);
		}
		if(c==2)
		{
			outputs[SUM_OUTPUT+1].setVoltage((out[c]*params[C_AMOUNT_PARAM].getValue())+out[c+1]);
			if(params[MODE_PARAM+1].value==0)outputs[OR_OUTPUT+1].setVoltage(out[c]>out[c+1]?out[c]:out[c+1]);
			else outputs[OR_OUTPUT+1].setVoltage(out[c]>out[c+1]?out[c+1]:out[c]);
		}

        }
}
};

template <typename BASE>
struct CLight : BASE
{
  CLight()
  {
    this->box.size = mm2px(Vec(5, 5));
  }
};


struct ContornoMK2Widget : ModuleWidget {
	ContornoMK2Widget(ContornoMK2 *module){
	 setModule(module);
	 setPanel(APP->window->loadSvg(asset::plugin(pluginInstance,  "res/ContornoMK2.svg")));

	 addChild(createWidget<ScrewBlack>(Vec(15, 0)));
	 addChild(createWidget<ScrewBlack>(Vec(box.size.x - 30, 0)));
	 addChild(createWidget<ScrewBlack>(Vec(15, 365)));
	 addChild(createWidget<ScrewBlack>(Vec(box.size.x - 30, 365)));

	 int space = 64;

	 for (int i = 0; i < 4; i++)
	 { 

		 addParam(createParam<MCKSSS>(Vec(space * i + 52, 25), module, ContornoMK2::RANGE_PARAM + i));
		 addParam(createParam<LEDT>(Vec(space * i + 7, 297), module, ContornoMK2::CYCLE_PARAM + i));

		 addChild(createLight<CLight<OrangeLight>>(Vec(space * i + 10, 300), module, ContornoMK2::CYCLE_LIGHT + i));

		 addParam(createParam<RoundWhy>(Vec(space * i + 12.5, 39), module, ::ContornoMK2::SHAPE_PARAM + i));

		 addParam(createParam<SlidePotR>(Vec(space * i + 10, 100), module, ::ContornoMK2::RISE_PARAM + i));
		 addParam(createParam<SlidePotR>(Vec(space * i + 40, 100), module, ::ContornoMK2::FALL_PARAM + i));


		 addParam(createParam<BPushR>(Vec(space * i + 5, 255), module, ::ContornoMK2::TRIGG_PARAM + i));

		 addChild(createLight<SmallLight<OrangeLight>>(Vec(space * i + 15, 212), module, ContornoMK2::RISE_LIGHT + i));
		 addChild(createLight<SmallLight<OrangeLight>>(Vec(space * i + 45, 212), module, ContornoMK2::FALL_LIGHT + i));
	  }

		 addOutput(createOutput<PJ301MOPort>(Vec(space * 0 + 35, 335), module, ::ContornoMK2::OUT_OUTPUT + 0));
		 addOutput(createOutput<PJ301MOPort>(Vec(space * 1 + 35, 335), module, ::ContornoMK2::OUT_OUTPUT + 1));
		 addOutput(createOutput<PJ301MOPort>(Vec(space * 2 + 35, 335), module, ::ContornoMK2::OUT_OUTPUT + 2));
		 addOutput(createOutput<PJ301MOPort>(Vec(space * 3 + 35, 335), module, ::ContornoMK2::OUT_OUTPUT + 3));

		 addInput(createInput<PJ301MLPort>(Vec(35 + space * 0, 294), module, ::ContornoMK2::CYCLE_INPUT + 0));
		 addInput(createInput<PJ301MLPort>(Vec(35 + space * 1, 294), module, ::ContornoMK2::CYCLE_INPUT + 1));
		 addInput(createInput<PJ301MLPort>(Vec(35 + space * 2, 294), module, ::ContornoMK2::CYCLE_INPUT + 2));
		 addInput(createInput<PJ301MLPort>(Vec(35 + space * 3, 294), module, ::ContornoMK2::CYCLE_INPUT + 3));


		 addInput(createInput<PJ301MOrPort>(Vec(space * 0 + 35, 220), module, ::ContornoMK2::FALL_INPUT + 0));
		 addInput(createInput<PJ301MOrPort>(Vec(space * 1 + 35, 220), module, ::ContornoMK2::FALL_INPUT + 1));
		 addInput(createInput<PJ301MOrPort>(Vec(space * 2 + 35, 220), module, ::ContornoMK2::FALL_INPUT + 2));
		 addInput(createInput<PJ301MOrPort>(Vec(space * 3 + 35, 220), module, ::ContornoMK2::FALL_INPUT + 3));

		 addInput(createInput<PJ301MOrPort>(Vec(space * 0 + 5, 220), module, ::ContornoMK2::RISE_INPUT + 0));
		 addInput(createInput<PJ301MOrPort>(Vec(space * 1 + 5, 220), module, ::ContornoMK2::RISE_INPUT + 1));
		 addInput(createInput<PJ301MOrPort>(Vec(space * 2 + 5, 220), module, ::ContornoMK2::RISE_INPUT + 2));
		 addInput(createInput<PJ301MOrPort>(Vec(space * 3 + 5, 220), module, ::ContornoMK2::RISE_INPUT + 3));

		 addInput(createInput<PJ301MLPort>(Vec(space * 0 + 35, 255), module, ::ContornoMK2::TRIGG_INPUT + 0));
		 addInput(createInput<PJ301MLPort>(Vec(space * 1 + 35, 255), module, ::ContornoMK2::TRIGG_INPUT + 1));
		 addInput(createInput<PJ301MLPort>(Vec(space * 2 + 35, 255), module, ::ContornoMK2::TRIGG_INPUT + 2));
		 addInput(createInput<PJ301MLPort>(Vec(space * 3 + 35, 255), module, ::ContornoMK2::TRIGG_INPUT + 3));

		 addInput(createInput<PJ301MIPort>(Vec(space * 0 + 5, 335), module, ::ContornoMK2::IN_INPUT + 0));
		 addInput(createInput<PJ301MIPort>(Vec(space * 1 + 5, 335), module, ::ContornoMK2::IN_INPUT + 1));
		 addInput(createInput<PJ301MIPort>(Vec(space * 2 + 5, 335), module, ::ContornoMK2::IN_INPUT + 2));
		 addInput(createInput<PJ301MIPort>(Vec(space * 3 + 5, 335), module, ::ContornoMK2::IN_INPUT + 3));


         //////////////////////////////////////////////////////////////////////////////////////

 

         addParam(createParam<MCKSSS2>(Vec(270+25, 25), module, ContornoMK2::MODE_PARAM));
         addParam(createParam<MCKSSS2>(Vec(270+25, 205), module, ContornoMK2::MODE_PARAM + 1));

         addParam(createParam<MicroBlu>(Vec(275, 115), module, ContornoMK2::B_AMOUNT_PARAM));
         addParam(createParam<MicroBlu>(Vec(275, 180 + 115), module, ContornoMK2::C_AMOUNT_PARAM));

         addParam(createParam<BPushR>(Vec(260,82.5), module, ::ContornoMK2::TRIG_EXT_PARAM ));
         addParam(createParam<BPushR>(Vec(260,180+82.5), module, ::ContornoMK2::TRIG_EXT_PARAM + 1));

         addInput(createInput<PJ301MLPort>(Vec(260+27.5, 82.5), module, ::ContornoMK2::EXT_TRIGG_INPUT + 0));
         addInput(createInput<PJ301MLPort>(Vec(260+27.5, 180 + 82.5), module, ::ContornoMK2::EXT_TRIGG_INPUT + 1));

         addOutput(createOutput<PJ301MOPort>(Vec(260,50), module, ::ContornoMK2::EOC_OUTPUT + 0));
		 addOutput(createOutput<PJ301MOPort>(Vec(260+27.5, 50), module, ::ContornoMK2::EOC_OUTPUT + 1));
		 addOutput(createOutput<PJ301MOPort>(Vec(260,180+50), module, ::ContornoMK2::EOC_OUTPUT + 2));
         addOutput(createOutput<PJ301MOPort>(Vec(260+27.5,180+50), module, ::ContornoMK2::EOC_OUTPUT + 3));

         addOutput(createOutput<PJ301MOPort>(Vec(260, 50+105), module, ::ContornoMK2::SUM_OUTPUT + 0));
         addOutput(createOutput<PJ301MOPort>(Vec(260+27, 50 +105), module, ::ContornoMK2::OR_OUTPUT + 0));
         addOutput(createOutput<PJ301MOPort>(Vec(260, 335), module, ::ContornoMK2::SUM_OUTPUT + 1));
         addOutput(createOutput<PJ301MOPort>(Vec(260 + 27, 335), module, ::ContornoMK2::OR_OUTPUT + 1));
}
};
Model *modelContornoMK2 = createModel<ContornoMK2, ContornoMK2Widget>("ContornoMK2");
