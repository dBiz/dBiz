///////////////////////////////////////////////////
//  dBiz Empty
// 
///////////////////////////////////////////////////

#include "plugin.hpp"

using namespace std;

/////added fine out /////////////////////////////////////////////////

struct Empty : Module {
    enum ParamIds
    {
  
        NUM_PARAMS
    };
    enum InputIds
    {

        NUM_INPUTS
    };
    enum OutputIds {
	
        NUM_OUTPUTS
};

    enum LighIds {
        ENUMS(AMOUNT_LIGHT, 3),
        NUM_LIGHTS
    };

  Empty() 
  {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

  }

  void process(const ProcessArgs &args) override 
  {




  }
};

//////////////////////////////////////////////////////////////////
struct EmptyWidget : ModuleWidget 
{
EmptyWidget(Empty *module){
  setModule(module);
  setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Empty.svg")));

    //Screw
    addChild(createWidget<ScrewBlack>(Vec(15, 0)));
    addChild(createWidget<ScrewBlack>(Vec(box.size.x - 30, 0)));
    addChild(createWidget<ScrewBlack>(Vec(15, 365)));
    addChild(createWidget<ScrewBlack>(Vec(box.size.x - 30, 365)));

    
}
};
Model *modelEmpty = createModel<Empty, EmptyWidget>("Empty");
