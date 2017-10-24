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
	NUM_OUTPUTS
	};

      
  SchmittTrigger button_triggers[4][4];
  
  float button_lights[4][4] = {0.0,0.0,0.0,0.0,
                              0.0,0.0,0.0,0.0,
                              0.0,0.0,0.0,0.0,
                              0.0,0.0,0.0,0.0};
    
  int x_position = 0;
  int y_position = 0;
    
	BenePads() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS) {}
	void step() override;
   
};

void BenePads::step() {

  	// handle button triggers
    for (int i = 0; i < 4; i++)
    {
      for (int j = 0; j < 4; j++)
      {
        if ((params[BUTTON_PARAM + i + j * 4].value))
        {
          button_lights[i][j] = 1.0;
          x_position = i;
          y_position = j;
          outputs[X_OUT].value = i+1;
          outputs[Y_OUT].value = j+1;
        }
        else
          button_lights[i][j] = 0.0;
        }
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
  
  for ( int i = 0 ; i < 4 ; i++)
  {
    for ( int j = 0 ; j < 4 ; j++)
    {
     
      addParam(createParam<PB61303>(Vec(button_offset+left+column_spacing * i-10, top + row_spacing * j + 150 ), module, BenePads::BUTTON_PARAM + i + j * 4, 0.0, 1.0, 0.0));
      addChild(createValueLight<LargeLight<YellowValueLight>>(Vec(button_offset+left+column_spacing * i -10 + 4.5, top + row_spacing * j + 150 + 4.5), &module->button_lights[i][j]));
    }
    
    }  
	
  addChild(createScrew<ScrewSilver>(Vec(15, 0)));
  addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 0)));
  addChild(createScrew<ScrewSilver>(Vec(15, 365)));
  addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 365)));

}
