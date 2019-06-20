///////////////////////////////////////////////////
//
//   VCA530 Clone
//  based on Fundamental Mixer
//
///////////////////////////////////////////////////
 
#include "plugin.hpp"
 
///////////////////////////////////////////////////
struct VCA530 : Module {
    enum ParamIds
    {
        MIX1_PARAM,
        MIX2_PARAM,
        CH1_PARAM,
        CH2_PARAM,
        CH3_PARAM,
        CH4_PARAM,
        CH5_PARAM,
        CH6_PARAM,
        CV1_PARAM,
        CV2_PARAM,
        CV3_PARAM,
        CV4_PARAM,
        CV5_PARAM,
        CV6_PARAM,
        NUM_PARAMS
    };
    enum InputIds
    {
        CH1_INPUT,
        CH1_CV_INPUT,
        CH2_INPUT,
        CH2_CV_INPUT,
        CH3_INPUT,
        CH3_CV_INPUT,
        CH4_INPUT,
        CH4_CV_INPUT,
        CH5_INPUT,
        CH5_CV_INPUT,
        CH6_INPUT,
        CH6_CV_INPUT,
        NUM_INPUTS
    };

    enum OutputIds
    {
        SUM_OUTPUT_R,
        SUM_OUTPUT_L,
        MIX_OUTPUT_R,
        MIX_OUTPUT_L,
        NUM_OUTPUTS
    };

    enum LightIds
    {
        MIX1_LIGHTS,
        CLIP1_LIGHTS,
        MIX2_LIGHTS,
        CLIP2_LIGHTS,
        NUM_LIGHTS
    };

    VCA530(){
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

    configParam(MIX1_PARAM,  0.0, 1.0, 0.0,"Mix 1 Level");
    configParam(MIX2_PARAM,  0.0, 1.0, 0.0,"Mix 2 Level");     

    configParam(CH1_PARAM,  0.0, 1.0, 0.0, "Ch 1 Level");
    configParam(CH2_PARAM,  0.0, 1.0, 0.0, "Ch 2 Level");
    configParam(CH3_PARAM,  0.0, 1.0, 0.0, "Ch 3 Level");
    configParam(CH4_PARAM,  0.0, 1.0, 0.0, "Ch 4 Level");
    configParam(CH6_PARAM,  0.0, 1.0, 0.0, "Ch 5 Level");
    configParam(CH5_PARAM,  0.0, 1.0, 0.0, "Ch 6 Level");

    configParam(CV1_PARAM,  0.0, 1.0, 0.0, "Ch 1 CV");
    configParam(CV2_PARAM,  0.0, 1.0, 0.0, "Ch 2 CV");
    configParam(CV3_PARAM,  0.0, 1.0, 0.0, "Ch 3 CV");
    configParam(CV4_PARAM,  0.0, 1.0, 0.0, "Ch 4 CV");
    configParam(CV5_PARAM,  0.0, 1.0, 0.0, "Ch 5 CV");
    configParam(CV6_PARAM,  0.0, 1.0, 0.0, "Ch 6 CV");

    }

void process(const ProcessArgs &args) override 
{      

    float ch1 = inputs[CH1_INPUT].value * params[CH1_PARAM].value;
    float ch2 = inputs[CH2_INPUT].value * params[CH2_PARAM].value;
    float ch3 = inputs[CH3_INPUT].value * params[CH3_PARAM].value;
    float ch4 = inputs[CH4_INPUT].value * params[CH4_PARAM].value;
    float ch5 = inputs[CH5_INPUT].value * params[CH5_PARAM].value;
    float ch6 = inputs[CH6_INPUT].value * params[CH6_PARAM].value;

    float sum_ch = (ch1 + ch2 + ch3 + ch4 + ch5 + ch6);
    float sum_mix_l = (ch1 + ch2 + ch3);
    float sum_mix_r = (ch4 + ch5 + ch6);

    float cv1 = sum_mix_l * (clamp(inputs[CH1_CV_INPUT].value / 10.0f, 0.0f, 1.0f)) * (params[CV1_PARAM].value);
    float cv2 = sum_mix_l * (clamp(inputs[CH2_CV_INPUT].value / 10.0f, 0.0f, 1.0f)) * (params[CV2_PARAM].value);
    float cv3 = sum_mix_l * (clamp(inputs[CH3_CV_INPUT].value / 10.0f, 0.0f, 1.0f)) * (params[CV3_PARAM].value);
    float cv4 = sum_mix_r * (clamp(inputs[CH4_CV_INPUT].value / 10.0f, 0.0f, 1.0f)) * (params[CV4_PARAM].value);
    float cv5 = sum_mix_r * (clamp(inputs[CH5_CV_INPUT].value / 10.0f, 0.0f, 1.0f)) * (params[CV5_PARAM].value);
    float cv6 = sum_mix_r * (clamp(inputs[CH6_CV_INPUT].value / 10.0f, 0.0f, 1.0f)) * (params[CV6_PARAM].value);

    float sum_l = sum_ch * params[MIX1_PARAM].value;
    float sum_r = sum_ch * params[MIX2_PARAM].value;
    float mix_l = sum_mix_l * params[MIX1_PARAM].value;
    float mix_r = sum_mix_r * params[MIX2_PARAM].value;

    float sum_cv_l = (cv1 + cv2 + cv3);
    float sum_cv_r = (cv4 + cv5 + cv6);

    outputs[SUM_OUTPUT_R].value = sum_r + sum_cv_r + sum_cv_l;
    outputs[SUM_OUTPUT_L].value = sum_l + sum_cv_l + sum_cv_r;
    outputs[MIX_OUTPUT_R].value = mix_r + sum_cv_r;
    outputs[MIX_OUTPUT_L].value = mix_l + sum_cv_l;

    lights[MIX1_LIGHTS].value = outputs[MIX_OUTPUT_L].value;
    lights[MIX2_LIGHTS].value = outputs[MIX_OUTPUT_R].value;
    if (outputs[MIX_OUTPUT_L].value > 4)
        lights[CLIP1_LIGHTS].value =1.0;
    else
        lights[CLIP1_LIGHTS].value = 0.0;

    if (outputs[MIX_OUTPUT_R].value > 4)
        lights[CLIP2_LIGHTS].value = 1.0;
    else
        lights[CLIP2_LIGHTS].value = 0.0;
}

};

struct VCA530Widget : ModuleWidget {
VCA530Widget(VCA530 *module) {
    setModule(module);
    setPanel(APP->window->loadSvg(asset::plugin(pluginInstance,  "res/VCA530.svg")));

    int column_1 = -15;
    int top_row = 60;
    int column_spacing = 27;
    int low = 350;
    int low_f = 230;
    int med = 180;
    int up = -15;

    addParam(createParam<SDKnob>(Vec(52, med + up), module, VCA530::MIX1_PARAM));
    addParam(createParam<SDKnob>(Vec(129, med + up), module, VCA530::MIX2_PARAM));

    // channel strips

    addInput(createInput<PJ301MIPort>(Vec(column_1 + column_spacing * 1 - 5, 30 + up), module, VCA530::CH1_INPUT));
    addInput(createInput<PJ301MIPort>(Vec(column_1 + column_spacing * 2 - 5, 30 + up), module, VCA530::CH2_INPUT));
    addInput(createInput<PJ301MIPort>(Vec(column_1 + column_spacing * 3 - 5, 30 + up), module, VCA530::CH3_INPUT));
    addInput(createInput<PJ301MIPort>(Vec(column_1 + column_spacing * 4 - 5, 30 + up), module, VCA530::CH4_INPUT));
    addInput(createInput<PJ301MIPort>(Vec(column_1 + column_spacing * 5 - 5, 30 + up), module, VCA530::CH5_INPUT));
    addInput(createInput<PJ301MIPort>(Vec(column_1 + column_spacing * 6 - 5, 30 + up), module, VCA530::CH6_INPUT));

    addParam(createParam<SlidePot2>(Vec(column_1 + column_spacing * 1, top_row + up), module, VCA530::CH1_PARAM));
    addParam(createParam<SlidePot2>(Vec(column_1 + column_spacing * 2, top_row + up), module, VCA530::CH2_PARAM));
    addParam(createParam<SlidePot2>(Vec(column_1 + column_spacing * 3, top_row + up), module, VCA530::CH3_PARAM));
    addParam(createParam<SlidePot2>(Vec(column_1 + column_spacing * 4, top_row + up), module, VCA530::CH4_PARAM));
    addParam(createParam<SlidePot2>(Vec(column_1 + column_spacing * 5, top_row + up), module, VCA530::CH5_PARAM));
    addParam(createParam<SlidePot2>(Vec(column_1 + column_spacing * 6, top_row + up), module, VCA530::CH6_PARAM));

    addInput(createInput<PJ301MCPort>(Vec(column_1 + column_spacing * 1 - 5, low + up), module, VCA530::CH1_CV_INPUT));
    addInput(createInput<PJ301MCPort>(Vec(column_1 + column_spacing * 2 - 5, low + up), module, VCA530::CH2_CV_INPUT));
    addInput(createInput<PJ301MCPort>(Vec(column_1 + column_spacing * 3 - 5, low + up), module, VCA530::CH3_CV_INPUT));
    addInput(createInput<PJ301MCPort>(Vec(column_1 + column_spacing * 4 - 5, low + up), module, VCA530::CH4_CV_INPUT));
    addInput(createInput<PJ301MCPort>(Vec(column_1 + column_spacing * 5 - 5, low + up), module, VCA530::CH5_CV_INPUT));
    addInput(createInput<PJ301MCPort>(Vec(column_1 + column_spacing * 6 - 5, low + up), module, VCA530::CH6_CV_INPUT));

    addParam(createParam<SlidePot2>(Vec(column_1 + column_spacing * 1, low_f + up), module, VCA530::CV1_PARAM));
    addParam(createParam<SlidePot2>(Vec(column_1 + column_spacing * 2, low_f + up), module, VCA530::CV2_PARAM));
    addParam(createParam<SlidePot2>(Vec(column_1 + column_spacing * 3, low_f + up), module, VCA530::CV3_PARAM));
    addParam(createParam<SlidePot2>(Vec(column_1 + column_spacing * 4, low_f + up), module, VCA530::CV4_PARAM));
    addParam(createParam<SlidePot2>(Vec(column_1 + column_spacing * 5, low_f + up), module, VCA530::CV5_PARAM));
    addParam(createParam<SlidePot2>(Vec(column_1 + column_spacing * 6, low_f + up), module, VCA530::CV6_PARAM));

    //Screw

    addChild(createWidget<ScrewBlack>(Vec(15, 0)));
    addChild(createWidget<ScrewBlack>(Vec(box.size.x - 30, 0)));
    addChild(createWidget<ScrewBlack>(Vec(15, 365)));
    addChild(createWidget<ScrewBlack>(Vec(box.size.x - 30, 365)));

    // outputs
    addOutput(createOutput<PJ301MOPort>(Vec(15, med + up), module, VCA530::MIX_OUTPUT_L));
    addOutput(createOutput<PJ301MOPort>(Vec(95, med + up), module, VCA530::MIX_OUTPUT_R));
    //
    addOutput(createOutput<PJ301MOPort>(Vec(15, med + 25 + up), module, VCA530::SUM_OUTPUT_L));
    addOutput(createOutput<PJ301MOPort>(Vec(95, med + 25 + up), module, VCA530::SUM_OUTPUT_R));

    // lights

    addChild(createLight<SmallLight<BlueLight>>(Vec(42, med + 5 + 20), module, VCA530::MIX1_LIGHTS));
    addChild(createLight<SmallLight<BlueLight>>(Vec(122, med + 5 + 20), module, VCA530::MIX2_LIGHTS));
    addChild(createLight<SmallLight<WhiteLight>>(Vec(42, med - 10 + 20), module, VCA530::CLIP1_LIGHTS));
    addChild(createLight<SmallLight<WhiteLight>>(Vec(122, med - 10 + 20), module, VCA530::CLIP2_LIGHTS));
}
};
Model *modelVCA530 = createModel<VCA530, VCA530Widget>("VCA530");

