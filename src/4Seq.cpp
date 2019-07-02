///////////////////////////////////////////////////
//  dBiz FourSeq
// 
///////////////////////////////////////////////////

#include "plugin.hpp"

using namespace std;

/////added fine out /////////////////////////////////////////////////
struct FourSeq : Module {
    enum ParamIds
    {   
        RESET_PARAM,
        STEPA_PARAM,
        STEPB_PARAM,
        ENUMS(GATEA_PARAM, 4),
        ENUMS(GATEB_PARAM, 4),
        ENUMS(SEQA_PARAM, 4),
        ENUMS(SEQB_PARAM, 4),
        NUM_PARAMS
    };
    enum InputIds
    {
        RESET_INPUT,
        CLKA_INPUT,
        CLKB_INPUT,
        ENUMS(CVA_INPUT, 4),
        ENUMS(CVB_INPUT, 4),
        NUM_INPUTS
    };
	enum OutputIds
	{
        SEQA_OUTPUT,
        SEQB_OUTPUT,
        GATEA_OUTPUT,
        GATEB_OUTPUT,
		NUM_OUTPUTS
	};

	enum LighIds
	{
        RESET_LIGHT,
        ENUMS(SEQA_LIGHT, 4),
        ENUMS(SEQB_LIGHT, 4),
        ENUMS(GATEA_LIGHT, 4), 
        ENUMS(GATEB_LIGHT, 4),
		NUM_LIGHTS
	};

    dsp::SchmittTrigger clk;
    dsp::SchmittTrigger clkb;
    dsp::SchmittTrigger reset_button;


    dsp::PulseGenerator gate1;
    dsp::PulseGenerator gate2;

    dsp::SchmittTrigger gate_a[4];
    dsp::SchmittTrigger gate_b[4];

    bool gateState_a[4] = {};
    bool gateState_b[4] = {};
    bool gateState[8] = {};

    bool pulse1; 
    bool pulse2; 

    bool running_a = true;    
    bool running_b = true;

    int clk1C = 0;
    int clk2C = 0;

    int maxStepA = 0;
    int maxStepB = 0;  

    enum GateMode
    {
        TRIGGER,
        RETRIGGER,
        CONTINUOUS
    };

    GateMode gateMode = TRIGGER;

    FourSeq()
    {
     config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(RESET_PARAM, 0.0, 1.0, 0.0,"Reset");
        configParam(STEPA_PARAM, 0.0, 2.0, 0.0,"Step A");
        configParam(STEPB_PARAM, 0.0, 2.0, 0.0,"Step B");

        for(int i=0; i<4;i++)
        {
            configParam(GATEA_PARAM+i, 0.0, 1.0, 0.0,"Gate A");
            configParam(GATEB_PARAM+i, 0.0, 1.0, 0.0,"Gate B");
            configParam(SEQA_PARAM+i, -3.0,3.0, 0.0,"Seq A Param"); 
            configParam(SEQB_PARAM+i, -3.0,3.0, 0.0,"Seq B Param");
        }
        onReset();
    }
    void onReset() override
    {
        for (int i = 0; i < 8; i++)
        {
            gateState[i] = true;
        }
    }

    json_t *dataToJson() override
    {
        json_t *rootJ = json_object();

        json_object_set_new(rootJ, "running_a", json_boolean(running_a));
        json_object_set_new(rootJ, "running_b", json_boolean(running_b));


        json_t *gatesJ = json_array();
        for (int i = 0; i < 8; i++)
        {
            json_t *gateJ = json_integer((int)gateState[i]);
            json_array_append_new(gatesJ, gateJ);
        }
        json_object_set_new(rootJ, "gate", gatesJ);


        return rootJ;
    }

    void dataFromJson(json_t *rootJ) override
    {
        json_t *ArunningJ = json_object_get(rootJ, "running_a");
		if (ArunningJ)
			running_a = json_is_true(ArunningJ);

        json_t *BrunningJ = json_object_get(rootJ, "running_b");
		if (BrunningJ)
			running_b = json_is_true(BrunningJ);


        json_t *gatesJ = json_object_get(rootJ, "gates");
        if (gatesJ)
        {
            for (int i = 0; i < 8; i++)
            {
                json_t *gateJ = json_array_get(gatesJ, i);
                if (gateJ)
                    gateState[i] = !!json_integer_value(gateJ);
            }
        }

    }

    void process(const ProcessArgs &args) override 
    {
        if (params[STEPA_PARAM].value == 0)  maxStepA = 3; 
        if (params[STEPA_PARAM].value == 1) { maxStepA = 2; lights[SEQA_LIGHT+3].value = 0.0;} 
        if (params[STEPA_PARAM].value == 2) {maxStepA = 1;  lights[SEQA_LIGHT+2].value = 0.0;}

        if (params[STEPB_PARAM].value == 0) maxStepB = 3;
        if (params[STEPB_PARAM].value == 1) {maxStepB = 2; lights[SEQB_LIGHT+3].value = 0.0;} 
        if (params[STEPB_PARAM].value == 2) {maxStepB = 1; lights[SEQB_LIGHT+2].value = 0.0;}

        bool gateAIn = false;
        bool gateBIn = false;

/////////////////////////////////////////////////////
        if (inputs[CLKA_INPUT].isConnected()) {
				// External clock
				if (clk.process(inputs[CLKA_INPUT].getVoltage())) {
					clk1C++;
				}
				gateAIn = clk.isHigh();

            if (clk1C > maxStepA)
            {
                clk1C = 0;
            }

			}
		if (inputs[CLKB_INPUT].isConnected()) {
				// External clock
				if (clkb.process(inputs[CLKB_INPUT].getVoltage())) {
					clk2C++;
				}
				gateBIn = clkb.isHigh();
            if (clk2C > maxStepB)
            {
                clk2C = 0;
            }
			}	

/////////////////////////////////////////////////////////////////


        if (reset_button.process(params[RESET_PARAM].getValue()+inputs[RESET_INPUT].getVoltage()))
        {
            clk1C=0;
            clk2C=0;
        }


        for (int i = 0; i < 4; i++)
        {
            gateState[i] = gateState_a[i];
            gateState[4 + i] = gateState_b[i];
        }

        if (gateState_a[clk1C])
        {
            outputs[SEQA_OUTPUT].setVoltage(clamp(inputs[CVA_INPUT + clk1C].getVoltage() + params[SEQA_PARAM + clk1C].getValue(), -3.0, 3.0));
        }
        if (gateState_b[clk2C])
        {
            outputs[SEQB_OUTPUT].setVoltage(clamp(inputs[CVB_INPUT + clk2C].getVoltage() + params[SEQB_PARAM + clk2C].getValue(), -3.0, 3.0));
        }

        for (int i = 0; i < 4; i++)
        {
            if (gate_a[i].process(params[GATEA_PARAM + i].getValue()))
            {
                gateState_a[i] = !gateState_a[i];
            }
            lights[GATEA_LIGHT + i].setBrightness(gateState_a[i] ? 1.0 : 0.0);
            if (gate_b[i].process(params[GATEB_PARAM + i].value))
            {
                gateState_b[i] = !gateState_b[i];
            }
            lights[GATEB_LIGHT + i].setBrightness(gateState_b[i] ? 1.0 : 0.0);
        }

        lights[RESET_LIGHT].setSmoothBrightness(reset_button.isHigh(), args.sampleTime);


    for (int i = 0; i < 4; i++) {

        lights[SEQA_LIGHT+i].setSmoothBrightness((gateAIn && i == clk1C) ? 1.f : 0.0, args.sampleTime);
        lights[SEQB_LIGHT+i].setSmoothBrightness((gateBIn && i == clk2C) ? 1.f : 0.0, args.sampleTime);
    
    }
    outputs[GATEA_OUTPUT].setVoltage((gateAIn && gateState_a[clk1C]) ? 10.f : 0.f);
    outputs[GATEB_OUTPUT].setVoltage((gateBIn && gateState_b[clk2C]) ? 10.f : 0.f);






    }
};

template <typename BASE>
struct SLight : BASE
{
  SLight()
  {
    this->box.size = mm2px(Vec(5, 5));
  }
};

struct FourSeqWidget : ModuleWidget {
   FourSeqWidget(FourSeq *module){
       setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance,  "res/FourSeq.svg")));
         
           //Screw
           addChild(createWidget<ScrewBlack>(Vec(15, 0)));
           addChild(createWidget<ScrewBlack>(Vec(box.size.x - 30, 0)));
           addChild(createWidget<ScrewBlack>(Vec(15, 365)));
           addChild(createWidget<ScrewBlack>(Vec(box.size.x - 30, 365)));

           int knob = 35;
           int jack = 27;

           for (int i = 0; i < 4; i++)
           {
               addParam(createParam<SDKnob>(Vec(70, 28 + knob * i), module, FourSeq::SEQA_PARAM + i));
               addParam(createParam<LEDB>(Vec(15, 31 + knob * i), module, FourSeq::GATEA_PARAM + i));
               addChild(createLight<SLight<OrangeLight>>(Vec(18, 34 + knob * i), module, FourSeq::GATEA_LIGHT + i));

               addParam(createParam<SDKnob>(Vec(70, 172 + knob * i), module, FourSeq::SEQB_PARAM + i));
               addParam(createParam<LEDB>(Vec(15, 175 + knob * i), module, FourSeq::GATEB_PARAM + i));
               addChild(createLight<SLight<OrangeLight>>(Vec(18, 178 + knob * i),module, FourSeq::GATEB_LIGHT + i));

               addInput(createInput<PJ301MVAPort>(Vec(40, 30.5 + knob * i), module, FourSeq::CVA_INPUT + i));
               addInput(createInput<PJ301MVAPort>(Vec(40, 173.5 + knob * i), module, FourSeq::CVB_INPUT + i));

               addChild(createLight<SmallLight<RedLight>>(Vec(105, 38 + knob * i), module, FourSeq::SEQA_LIGHT + i));
               addChild(createLight<SmallLight<RedLight>>(Vec(105, 180 + knob * i), module, FourSeq::SEQB_LIGHT + i));
           }
           addInput(createInput<PJ301MVAPort>(Vec(14, 170 + knob * 4), module, FourSeq::CLKA_INPUT));
           addInput(createInput<PJ301MVAPort>(Vec(14, 197 + knob * 4), module, FourSeq::CLKB_INPUT));

           addOutput(createOutput<PJ301MVAPort>(Vec(14 + jack, 170 + knob * 4), module, FourSeq::SEQA_OUTPUT));
           addOutput(createOutput<PJ301MVAPort>(Vec(14 + jack, 197 + knob * 4), module, FourSeq::SEQB_OUTPUT));

           addOutput(createOutput<PJ301MVAPort>(Vec(14 + jack * 2, 170 + knob * 4), module, FourSeq::GATEA_OUTPUT));
           addOutput(createOutput<PJ301MVAPort>(Vec(14 + jack * 2, 197 + knob * 4), module, FourSeq::GATEB_OUTPUT));

           addParam(createParam<MCKSSS>(Vec(14 + jack * 3, 172 + knob * 4), module, FourSeq::STEPA_PARAM));
           addParam(createParam<MCKSSS>(Vec(14 + jack * 3, 199 + knob * 4), module, FourSeq::STEPB_PARAM));

           addParam(createParam<LEDB>(Vec(35 + jack, 4), module, FourSeq::RESET_PARAM));
           addChild(createLight<SLight<OrangeLight>>(Vec(38 + jack, 7), module, FourSeq::RESET_LIGHT));

           addInput(createInput<PJ301MVAPort>(Vec(35, 4), module, FourSeq::RESET_INPUT));
}
};

Model *modelFourSeq = createModel<FourSeq, FourSeqWidget>("FourSeq");
