///////////////////////////////////////////////////
//  dBiz QuePasa
// 
///////////////////////////////////////////////////

#include "plugin.hpp"

using namespace std;

/////added fine out /////////////////////////////////////////////////

struct QuePasa : Module {
    enum ParamIds
    {
       // FREQ_PARAM,
       // VCA_PARAM,
       // FREQ_CV_PARAM,
       // RES_PARAM,
       // RES_CV_PARAM,
       // RAD_L_PARAM,
       // RAD_R_PARAM,
       // RAD_L_CV_PARAM,
       // RAD_R_CV_PARAM,

        NUM_PARAMS
    };
    enum InputIds
    {
      //  L_INPUT,
      //  R_INPUT,
      //  VCA_INPUT,
      //  RAD_L_INPUT,
      //  RAD_R_INPUT,
      //  VAR_L_INPUT,
      //  VAR_R_INPUT,
      //  FREQ1_INPUT,
      //  FREQ2_INPUT,
      //  RES_INPUT,
        NUM_INPUTS
    };
    enum OutputIds
    {

      //  LP_L_OUTPUT,
      //  LP_R_OUTPUT,
      //  HP_L_OUTPUT,
      //  HP_R_OUTPUT,
      //  BP_L_OUTPUT,
      //  BP_R_OUTPUT,
      //  SP_L_OUTPUT,
      //  SP_R_OUTPUT,
        NUM_OUTPUTS
    };

    enum LighIds {
       
        NUM_LIGHTS
    };

  QuePasa() 
  {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

  }

  void process(const ProcessArgs &args) override 
  {




  }
};

//////////////////////////////////////////////////////////////////
struct QuePasaWidget : ModuleWidget 
{
QuePasaWidget(QuePasa *module){
  setModule(module);
  setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/QuePasa.svg")));

    //Screw
    addChild(createWidget<ScrewBlack>(Vec(15, 0)));
    addChild(createWidget<ScrewBlack>(Vec(box.size.x - 30, 0)));
    addChild(createWidget<ScrewBlack>(Vec(15, 365)));
    addChild(createWidget<ScrewBlack>(Vec(box.size.x - 30, 365)));

    
}
};
Model *modelQuePasa = createModel<QuePasa, QuePasaWidget>("QuePasa");
