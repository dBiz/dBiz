///////////////////////////////////////////////////////////////////
//
//  dBiz revisited version of Cartesian seq. by Strum 
// 
///////////////////////////////////////////////////////////////////

#include "plugin.hpp"

struct BenePads : Module {
    enum ParamIds
    {
        ENUMS(BUTTON_PARAM, 16),
        NUM_PARAMS
    };
    enum InputIds
    {
     NUM_INPUTS
    };
	enum OutputIds {
	X_OUT,
  Y_OUT, 
  G_OUT,  
	NUM_OUTPUTS
  };
  enum LightIds
  {
    ENUMS(PAD_LIGHT, 16),
    NUM_LIGHTS
  };

  dsp::SchmittTrigger button_triggers[4][4];
  
  int x_position = 0;
  int y_position = 0;
    
	BenePads() {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
    for (int i=0; i<16;i++)
    {
      configParam(BUTTON_PARAM + i, 0.0, 1.0, 0.0 ,"Triggers");
    }
  } 

  void process(const ProcessArgs &args) override 
  {

    bool shot = false;
 
    for (int i = 0; i < 4; i++)
    {
      for (int j = 0; j < 4; j++)
      {
        if ((params[BUTTON_PARAM + i + j*4].value))
        {
          lights[PAD_LIGHT+i+j*4].value = 1.0;
          shot = true;
          x_position = i;
          y_position = j; 
          outputs[X_OUT].value = i + 1;
          outputs[Y_OUT].value = j + 1; 
        }
        else
        {
        lights[PAD_LIGHT+i+j*4].value=0.0  ;
        }
        if (shot)
        {
          outputs[G_OUT].value = 10.0;
        }
        else
        {
          outputs[G_OUT].value = 0.0;
        }
      } 
    }   
  }
};

////////////////////////////////

struct BenePadsWidget : ModuleWidget {
BenePadsWidget(BenePads *module){
  setModule(module);
  setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BenePad.svg")));

   int top = 20;
  int left = 3;
  int column_spacing = 35; 
  int row_spacing = 35;
  int button_offset = 20;

  addOutput(createOutput<PJ301MOrPort>(Vec(130, 20), module, BenePads::X_OUT));  
  addOutput(createOutput<PJ301MOrPort>(Vec(130, 50), module, BenePads::Y_OUT));
  addOutput(createOutput<PJ301MOrPort>(Vec(130, 80), module, BenePads::G_OUT));

      for (int i = 0; i < 4; i++)
  {
    for ( int j = 0 ; j < 4 ; j++)
    {

      addParam(createParam<BPush>(Vec(2+button_offset + left + column_spacing * i - 10,2+ top + row_spacing * j + 170), module, BenePads::BUTTON_PARAM + i + j * 4));
      addChild(createLight<BigLight<OrangeLight>>(Vec(+button_offset + left + column_spacing * i - 10 + 4.5, top + row_spacing * j + 170 + 4.5), module, BenePads::PAD_LIGHT + i + j * 4));
    }
    
    }  
	
  addChild(createWidget<ScrewBlack>(Vec(15, 0)));
  addChild(createWidget<ScrewBlack>(Vec(box.size.x-30, 0)));
  addChild(createWidget<ScrewBlack>(Vec(15, 365)));
  addChild(createWidget<ScrewBlack>(Vec(box.size.x-30, 365)));

}
};
Model *modelBenePads = createModel<BenePads, BenePadsWidget>("BenePads");
