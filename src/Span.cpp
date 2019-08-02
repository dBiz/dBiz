///////////////////////////////////////////////////
//  dBiz SPan
// 
///////////////////////////////////////////////////

#include "plugin.hpp"

using namespace std;

/////added fine out /////////////////////////////////////////////////
struct SPan : Module {
    enum ParamIds
    {
        XFADE_A_PARAM,
        PAN_A_PARAM,
        XFADE_B_PARAM,
        PAN_B_PARAM,
        AUX_LEVEL_PARAM,
        NUM_PARAMS
    };
    enum InputIds
    {
        A1_INPUT,
        B1_INPUT,
        XFADE_A_INPUT,
        A2_INPUT,
        B2_INPUT,
        XFADE_B_INPUT,
        PAN_A_INPUT,
        PAN_B_INPUT,
        AUX_L_INPUT,
        AUX_R_INPUT,
        AUX_LEVEL_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
	      L_OUTPUT,
        R_OUTPUT,
        NUM_OUTPUTS
};

    enum LighIds {
        NUM_LIGHTS
    };

    float sumL = 0.0;
    float sumR = 0.0;

    float aux_in_l = 0.0f;
    float aux_in_r = 0.0f;

    float a1_in = 0.0f;
    float b1_in = 0.0f;
    float a2_in = 0.0f;
    float b2_in = 0.0f;

    SPan()
    {
      config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

      configParam(XFADE_A_PARAM, 0.0, 1.0, 0.0, "Xfade A");
      configParam(XFADE_B_PARAM, 0.0, 1.0, 0.0, "Xfade B");
      configParam(PAN_A_PARAM, 0.0, 1.0, 0.5, "Pan A");
      configParam(PAN_B_PARAM, 0.0, 1.0, 0.5, "Pan B");
      configParam(AUX_LEVEL_PARAM, 0.0, 1.0, 0.0, "Aux Level");
  }

  void process(const ProcessArgs &args) override 
  {
    aux_in_l=inputs[AUX_L_INPUT].getVoltage()*(params[AUX_LEVEL_PARAM].getValue()+inputs[AUX_LEVEL_INPUT].getVoltage()/5.f);
    aux_in_r=inputs[AUX_R_INPUT].getVoltage()*(params[AUX_LEVEL_PARAM].getValue()+inputs[AUX_LEVEL_INPUT].getVoltage()/5.f);

    float pan_a_cv=0.f;
    float pan_b_cv = 0.f;

    float pan_a_pos = 0.f;
    float pan_b_pos = 0.f;

    float xfade_a_cv = 0.f;
    float xfade_b_cv = 0.f;

    float xfade_a_pos = 0.f;
    float xfade_b_pos = 0.f;

    pan_a_cv = inputs[PAN_A_INPUT].getVoltage() / 5.f;
    pan_a_pos = pan_a_cv + params[PAN_A_PARAM].getValue();
    if (pan_a_pos < 0)
      pan_a_pos = 0;
    if (pan_a_pos > 1)
      pan_a_pos = 1;

    pan_b_cv = inputs[PAN_B_INPUT].getVoltage() / 5.f;
    pan_b_pos = pan_b_cv + params[PAN_B_PARAM].getValue();
    if (pan_b_pos < 0)
      pan_b_pos = 0;
    if (pan_b_pos > 1)
      pan_b_pos = 1;


    xfade_a_cv= inputs[XFADE_A_INPUT].getVoltage()/5.f;
    xfade_a_pos= xfade_a_cv+ std::pow(params[XFADE_A_PARAM].getValue(),2.f);
    if (xfade_a_pos < 0)
      xfade_a_pos = 0;
    if (xfade_a_pos > 1)
      xfade_a_pos = 1;

    xfade_b_cv= inputs[XFADE_B_INPUT].getVoltage()/5.f;
    xfade_b_pos= xfade_b_cv+ std::pow(params[XFADE_B_PARAM].getValue(),2.f);
    if (xfade_b_pos < 0)
      xfade_b_pos = 0;
    if (xfade_b_pos > 1)
      xfade_b_pos = 1;

    a1_in = (inputs[A1_INPUT].getVoltage() * (1 - xfade_a_pos));
    b1_in = (inputs[B1_INPUT].getVoltage() * xfade_a_pos);

    a2_in = (inputs[A2_INPUT].getVoltage() * (1 - xfade_b_pos));
    b2_in = (inputs[B2_INPUT].getVoltage() * xfade_b_pos);

    float A_in = a1_in + b1_in;
    float B_in = a2_in + b2_in;

    outputs[L_OUTPUT].setVoltage(A_in * (1 - pan_a_pos) + B_in * (1 - pan_b_pos)+aux_in_l);
    outputs[R_OUTPUT].setVoltage(A_in * pan_a_pos + B_in * pan_b_pos + aux_in_r);
  }
};

//////////////////////////////////////////////////////////////////
struct SPanWidget : ModuleWidget 
{
SPanWidget(SPan *module){
  setModule(module);
  setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/SPan.svg")));

  int knob = 40;
  int jack=30;


    //Screw
    addChild(createWidget<ScrewBlack>(Vec(15, 0)));
    addChild(createWidget<ScrewBlack>(Vec(box.size.x - 30, 0)));
    addChild(createWidget<ScrewBlack>(Vec(15, 365)));
    addChild(createWidget<ScrewBlack>(Vec(box.size.x - 30, 365)));

    addInput(createInput<PJ301MIPort>(Vec(2, 40), module, SPan::A1_INPUT));
    addInput(createInput<PJ301MIPort>(Vec(2+(2*jack), 40), module, SPan::B1_INPUT));

    addInput(createInput<PJ301MCPort>(Vec(2 + jack, 40), module, SPan::XFADE_A_INPUT));
    addInput(createInput<PJ301MCPort>(Vec(2 + (jack * 3), 40), module, SPan::PAN_A_INPUT));

    addInput(createInput<PJ301MIPort>(Vec(2, 150), module, SPan::A2_INPUT));
    addInput(createInput<PJ301MIPort>(Vec(2+(2*jack), 150), module, SPan::B2_INPUT));

    addInput(createInput<PJ301MCPort>(Vec(2 + jack, 150), module, SPan::XFADE_B_INPUT));
    addInput(createInput<PJ301MCPort>(Vec(2 + (jack* 3), 150), module, SPan::PAN_B_INPUT));

    addParam(createParam<FlatA>(Vec(30, 80),module, SPan::XFADE_A_PARAM));
    addParam(createParam<FlatA>(Vec(2 * knob, 80), module, SPan::PAN_A_PARAM));

    addParam(createParam<FlatA>(Vec(30, 190), module, SPan::XFADE_B_PARAM));
    addParam(createParam<FlatA>(Vec((2 * knob), 190), module, SPan::PAN_B_PARAM));

    addParam(createParam<FlatA>(Vec((knob), 250), module, SPan::AUX_LEVEL_PARAM));

    addInput(createInput<PJ301MCPort>(Vec(2, 240), module, SPan::AUX_LEVEL_INPUT));

    addInput(createInput<PJ301MIPort>(Vec(2, 300), module, SPan::AUX_L_INPUT));
    addInput(createInput<PJ301MIPort>(Vec(2, 330), module, SPan::AUX_R_INPUT));

    addOutput(createOutput<PJ301MOPort>(Vec(2+(jack*3),300), module, SPan::L_OUTPUT));
    addOutput(createOutput<PJ301MOPort>(Vec(2+(jack*3),330), module, SPan::R_OUTPUT));
}
};
Model *modelSPan = createModel<SPan, SPanWidget>("SPan");
