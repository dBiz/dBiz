#include "plugin.hpp"


struct Remix : Module {
    enum ParamIds
    {
        SCAN_PARAM,
        CV_SCAN_PARAM,
        WIDTH_PARAM,
        CV_WIDTH_PARAM,
        LEVEL_PARAM,
        SLOPE_PARAM,
        CV_LEVEL_PARAM,
        CH1_LEVEL_PARAM,
        CH2_LEVEL_PARAM,
        CH3_LEVEL_PARAM,
        CH4_LEVEL_PARAM,
        CH5_LEVEL_PARAM,
        CH6_LEVEL_PARAM,
        NUM_PARAMS
    };
    enum InputIds
    {
        CH1_INPUT,
        CH2_INPUT,
        CH3_INPUT,
        CH4_INPUT,
        CH5_INPUT,
        CH6_INPUT,

        CH1_CV,
        CH2_CV,
        CH3_CV,
        CH4_CV,
        CH5_CV,
        CH6_CV,

        SLOPE_INPUT,
        SCAN_INPUT,
        WIDTH_INPUT,
        LEVEL_INPUT,
        NUM_INPUTS
    };
    enum OutputIds
    {
        OUT1_OUTPUT,
        OUT2_OUTPUT,
        OUT3_OUTPUT,
        OUT4_OUTPUT,
        OUT5_OUTPUT,
        OUT6_OUTPUT,
        A_OUTPUT,
        B_OUTPUT,
        C_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds
    {
        CH1_LIGHT,
        CH2_LIGHT,
        CH3_LIGHT,
        CH4_LIGHT,
        CH5_LIGHT,
        CH6_LIGHT,

        NUM_LIGHTS
    };

    float ins[6] = {};
    float outs[6] = {};
    float inMults[6] = {};
    float widthTable[7] = {0.0, 0.285, 0.285, 0.2608, 0.23523, 0.2125, 0.193};

    Remix(){
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        configParam(SCAN_PARAM,  0.0, 5.0,0.0,"Scan Param");
        configParam(CV_SCAN_PARAM,  0.0, 1.0, 0.0,"Scan Cv");
        configParam(WIDTH_PARAM,  0.0, 5.0,0.0,"Width");
        configParam(CV_WIDTH_PARAM,  0.0, 1.0, 0.0,"Width Cv");
        configParam(LEVEL_PARAM,  0.0, 1.0, 0.0,"Level");
        configParam(SLOPE_PARAM,  0.0, 5.0,0.0,"Slope");
        configParam(CV_LEVEL_PARAM,  0.0, 1.0, 0.0,"Cv");
        configParam(CH1_LEVEL_PARAM,  0.0, 1.0, 0.0,"Ch 1 Level");
        configParam(CH2_LEVEL_PARAM,  0.0, 1.0, 0.0,"Ch 2 Level");
        configParam(CH3_LEVEL_PARAM,  0.0, 1.0, 0.0,"Ch 3 Level");
        configParam(CH4_LEVEL_PARAM,  0.0, 1.0, 0.0,"Ch 4 Level");
        configParam(CH5_LEVEL_PARAM,  0.0, 1.0, 0.0,"Ch 5 Level");
        configParam(CH6_LEVEL_PARAM,  0.0, 1.0, 0.0,"Ch 6 Level");

    }

    int clampInt(const int _in, const int min = 0, const int max = 5)
    {
        if (_in > max)
            return max;
        if (_in < min)
            return min;
        return _in;
    }

    float triShape(float _in)
    {
        _in = _in - round(_in);
        return std::abs(_in + _in);
    }

    float LERP(const float _amountOfA, const float _inA, const float _inB)
    {
        return ((_amountOfA * _inA) + ((1.0f - _amountOfA) * _inB));
    }

    void process(const ProcessArgs &args) override 
   {

    float allInValue = 0.0f;

    for (int i = 0; i < 6; i++)
    {
        if (!inputs[CH1_INPUT + i].isConnected())
            ins[i] = allInValue;
        else {
            if(inputs[CH1_CV+i].isConnected())
                ins[i] = inputs[CH1_INPUT + i].getVoltage() * params[CH1_LEVEL_PARAM + i].getValue() * inputs[CH1_CV + i].getVoltage();
            else
                ins[i] = inputs[CH1_INPUT + i].getVoltage()*params[CH1_LEVEL_PARAM+i].getValue();
        }
    }

    int stages = 6;
    const float invStages = 1.0f / stages;
    const float halfStages = stages * 0.5f;
    const float remainInvStages = 1.0f - invStages;

    float widthControl = params[WIDTH_PARAM].getValue() + inputs[WIDTH_INPUT].getVoltage() * params[CV_WIDTH_PARAM].getValue();
    widthControl = clamp(widthControl, 0.0f, 5.0f) * 0.2f;
    widthControl = widthControl * widthControl * widthTable[stages];

    float scanControl = params[SCAN_PARAM].getValue() + inputs[SCAN_INPUT].getVoltage() * params[CV_SCAN_PARAM].getValue();
    scanControl = clamp(scanControl, 0.0f, 5.0f) * 0.2f;

    float slopeControl = params[SLOPE_PARAM].getValue() + inputs[SLOPE_INPUT].getVoltage();
    slopeControl = clamp(slopeControl, 0.0f, 5.0f) * 0.2f;
    

    float scanFactor1 = LERP(widthControl, halfStages, invStages);
    float scanFactor2 = LERP(widthControl, halfStages + remainInvStages, 1.0f);
    float scanFinal = LERP(scanControl, scanFactor2, scanFactor1);

    float invWidth = 1.0f / (LERP(widthControl, float(stages), invStages + invStages));

    float subStage = 0.0f;
    for (int i = 0; i < 6; i++)
    {
        inMults[i] = (scanFinal + subStage) * invWidth;
        subStage = subStage - invStages;
    }

    for (int i = 0; i < 6; i++)
    {
        inMults[i] = clamp(inMults[i], 0.0f, 1.0f);
        inMults[i] = triShape(inMults[i]);
        inMults[i] = clamp(inMults[i], 0.0f, 1.0f);

        const float shaped = (2.0f - inMults[i]) * inMults[i];
        inMults[i] = LERP(slopeControl, shaped, inMults[i]);
    }

    outputs[A_OUTPUT].value = 0.0f;
    outputs[B_OUTPUT].value = 0.0f;
    outputs[C_OUTPUT].value = 0.0f;

    for (int i = 0; i < 6; i++)
    {
        outputs[i].value = ins[i] * inMults[i];

        lights[CH1_LIGHT + i].setSmoothBrightness(fmaxf(0.0, inMults[i]),APP->engine->getSampleTime());

        outputs[B_OUTPUT].value = outputs[B_OUTPUT].value + outputs[i].value;

        if (i <= 1)
        {
            outputs[A_OUTPUT].value = outputs[A_OUTPUT].value + outputs[i].value;
        }
        else if (i >= 4)
        {
            outputs[C_OUTPUT].value = outputs[C_OUTPUT].value + outputs[i].value;
        }

    outputs[A_OUTPUT].value = crossfade(outputs[A_OUTPUT].value * params[LEVEL_PARAM].value, outputs[A_OUTPUT].value * params[LEVEL_PARAM].value*clamp(inputs[LEVEL_INPUT].value/10,0.0f,1.0f),params[CV_LEVEL_PARAM].value);
    outputs[B_OUTPUT].value = crossfade(outputs[B_OUTPUT].value * params[LEVEL_PARAM].value, outputs[B_OUTPUT].value * params[LEVEL_PARAM].value*clamp(inputs[LEVEL_INPUT].value/10,0.0f,1.0f),params[CV_LEVEL_PARAM].value);
    outputs[C_OUTPUT].value = crossfade(outputs[C_OUTPUT].value * params[LEVEL_PARAM].value, outputs[C_OUTPUT].value * params[LEVEL_PARAM].value*clamp(inputs[LEVEL_INPUT].value/10,0.0f,1.0f),params[CV_LEVEL_PARAM].value);
    }
}
};


struct RemixWidget : ModuleWidget {
RemixWidget(Remix *module) {
    setModule(module);
	setPanel(APP->window->loadSvg(asset::plugin(pluginInstance,  "res/Remix.svg")));
	

    int knob = 32;
    int jack = 27;
    int board =20;
    int light = 15;
    float mid = (14*15)/2;
    float midy= 190; 
    int space = 15;

	addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
	addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
	addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
	addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));


	addParam(createParam<RoundRed>(Vec(board, midy+10), module, Remix::SCAN_PARAM));
    addParam(createParam<RoundWhy>(Vec(board, midy+10+knob+10), module, Remix::CV_SCAN_PARAM));

    addParam(createParam<RoundRed>(Vec(mid-15, midy+10), module, Remix::WIDTH_PARAM));
    addParam(createParam<RoundWhy>(Vec(mid-15, midy+10+knob+10), module, Remix::CV_WIDTH_PARAM));

    addParam(createParam<Trimpot>(Vec(mid - 20, 322.5), module, Remix::SLOPE_PARAM));
    addInput(createInput<PJ301MCPort>(Vec(mid +10 , 320),  module, Remix::SLOPE_INPUT));

    addParam(createParam<RoundRed>(Vec(box.size.x - board - 32.5, midy+10), module, Remix::LEVEL_PARAM));
    addParam(createParam<RoundWhy>(Vec(box.size.x - board - 32.5, midy+10+knob+10), module, Remix::CV_LEVEL_PARAM));

    addOutput(createOutput<PJ301MOPort>(Vec(board + 7.5, 20), module, Remix::A_OUTPUT));
    addInput(createInput<PJ301MCPort>(Vec(board+7.5, 320),  module, Remix::SCAN_INPUT));

    addOutput(createOutput<PJ301MOPort>(Vec(mid-15 + 5.5, 20), module, Remix::B_OUTPUT));
    addInput(createInput<PJ301MCPort>(Vec(mid-15+ 7.5, 290),  module, Remix::WIDTH_INPUT));

    addOutput(createOutput<PJ301MOPort>(Vec(box.size.x-knob-board + 7.5, 20), module, Remix::C_OUTPUT));
    addInput(createInput<PJ301MCPort>(Vec(box.size.x-knob-board + 7.5, 320),  module, Remix::LEVEL_INPUT));


            addInput(createInput<PJ301MIPort>(Vec(space +5+ jack*0, 70),  module, Remix::CH1_INPUT));
            addParam(createParam<Trimpot>(Vec(space +10+ jack*0,125),module,Remix::CH1_LEVEL_PARAM));
            addInput(createInput<PJ301MCPort>(Vec(space + 5 + jack * 0, 150), module, Remix::CH1_CV));
            addChild(createLight<MediumLight<BlueLight>>(Vec(board + 30 + light * 0, midy), module, Remix::CH1_LIGHT));

            addInput(createInput<PJ301MIPort>(Vec(space + 5 + jack * 1, 70),  module, Remix::CH2_INPUT));
            addParam(createParam<Trimpot>(Vec(space + 10 + jack * 1, 125), module, Remix::CH2_LEVEL_PARAM));
            addInput(createInput<PJ301MCPort>(Vec(space + 5 + jack * 1, 150), module, Remix::CH2_CV));
            addChild(createLight<MediumLight<BlueLight>>(Vec(board + 30 + light * 1, midy), module, Remix::CH2_LIGHT));

            addInput(createInput<PJ301MIPort>(Vec(space + 5 + jack * 2, 70),  module, Remix::CH3_INPUT));
            addParam(createParam<Trimpot>(Vec(space + 10 + jack * 2, 125), module, Remix::CH3_LEVEL_PARAM));
            addInput(createInput<PJ301MCPort>(Vec(space + 5 + jack * 2, 150), module, Remix::CH3_CV));
            addChild(createLight<MediumLight<BlueLight>>(Vec(board + 30 + light * 2, midy), module, Remix::CH3_LIGHT));

            
            addInput(createInput<PJ301MIPort>(Vec(space +10+ jack*3+7.5, 70),  module, Remix::CH4_INPUT));
            addParam(createParam<Trimpot>(Vec(space +15+ jack*3+7,125),module,Remix::CH4_LEVEL_PARAM));
            addInput(createInput<PJ301MCPort>(Vec(space + 19 + jack * 3, 150), module, Remix::CH4_CV));
            addChild(createLight<MediumLight<BlueLight>>(Vec(board + 60 + light * 3, midy), module, Remix::CH4_LIGHT));

            addInput(createInput<PJ301MIPort>(Vec(space + 10 + jack * 4 + 7.5, 70),  module, Remix::CH5_INPUT));
            addParam(createParam<Trimpot>(Vec(space + 15 + jack * 4 + 7, 125), module, Remix::CH5_LEVEL_PARAM));
            addInput(createInput<PJ301MCPort>(Vec(space + 19 + jack * 4, 150), module, Remix::CH5_CV));
            addChild(createLight<MediumLight<BlueLight>>(Vec(board + 60 + light * 4, midy), module, Remix::CH5_LIGHT));

            addInput(createInput<PJ301MIPort>(Vec(space + 10 + jack * 5 + 7.5, 70),  module, Remix::CH6_INPUT));
            addParam(createParam<Trimpot>(Vec(space + 15 + jack * 5 + 7, 125), module, Remix::CH6_LEVEL_PARAM));
            addInput(createInput<PJ301MCPort>(Vec(space + 19 + jack * 5, 150), module, Remix::CH6_CV));
            addChild(createLight<MediumLight<BlueLight>>(Vec(board + 60 + light * 5, midy), module, Remix::CH6_LIGHT));

            // addChild(createLight<MediumLight<BlueLight>>(Vec(41, 59), module, Remix::CH_LIGHT));

}
};
Model *modelRemix = createModel<Remix, RemixWidget>("Remix");

