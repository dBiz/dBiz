///////////////////////////////////////////////////////////////////
//
//  dBiz revisited version of 
//
//  
//
// 
//
///////////////////////////////////////////////////////////////////

#include "dBiz.hpp"
#include "dsp/digital.hpp"

struct BenePads : Module {
    enum ParamIds
    {
        BUTTON_PARAM,
        NUM_PARAMS = BUTTON_PARAM + 16
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
    PAD_LIGHTS,
    NUM_LIGHTS =PAD_LIGHTS+16
  };

  SchmittTrigger button_triggers[4][4];
  
  int x_position = 0;
  int y_position = 0;
    
	BenePads() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	void step() override;
};

void BenePads::step() {
int x;
int y;

    for (int i = 0; i < 4; i++)
    {
      for (int j = 0; j < 4; j++)
      {
      
        if ((params[BUTTON_PARAM + i + j*4].value))
        {
          lights[PAD_LIGHTS+i+j*4].value = 1.0;
          x_position = i;
          y_position = j; 
          outputs[X_OUT].value = i + 1;
          outputs[Y_OUT].value = j + 1;
          x=i;
          y=j;
        }
        else
        {
        lights[PAD_LIGHTS+i+j*4].value=0.0  ;
        }
      } 
    }
    if (lights[PAD_LIGHTS+x+y*4].value == 1.0)
    {
      outputs[G_OUT].value = 5.0;
        }
        else
        {
          outputs[G_OUT].value = 0.0;
        }
}

////////////////////////////////

BenePadsWidget::BenePadsWidget() {
	BenePads *module = new BenePads();
	setModule(module);
	box.size = Vec(15*11, 380);
  
	{
    SVGPanel *panel = new SVGPanel();
    panel->box.size = box.size;
        panel->setBackground(SVG::load(assetPlugin(plugin,"res/BenePad.svg")));
    addChild(panel);
  }
 
  int top = 20;
  int left = 3;
  int column_spacing = 35; 
  int row_spacing = 35;
  int button_offset = 20;

  addOutput(createOutput<PJ301MOrPort>(Vec(130, 10), module, BenePads::X_OUT));  
  addOutput(createOutput<PJ301MOrPort>(Vec(130, 40), module, BenePads::Y_OUT));
  addOutput(createOutput<PJ301MOrPort>(Vec(130, 70), module, BenePads::G_OUT));

      for (int i = 0; i < 4; i++)
  {
    for ( int j = 0 ; j < 4 ; j++)
    {
     
      addParam(createParam<PB61303>(Vec(button_offset+left+column_spacing * i-10, top + row_spacing * j + 170 ), module, BenePads::BUTTON_PARAM + i + j * 4, 0.0, 1.0, 0.0));
      addChild(createLight<BigLight<OrangeLight>>(Vec(button_offset + left + column_spacing * i - 10 + 4.5, top + row_spacing * j + 170 + 4.5), module, BenePads::PAD_LIGHTS + i + j * 4));
    }
    
    }  
	
  addChild(createScrew<ScrewSilver>(Vec(15, 0)));
  addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 0)));
  addChild(createScrew<ScrewSilver>(Vec(15, 365)));
  addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 365)));

}
