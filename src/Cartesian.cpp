///////////////////////////////////////////////////////////////////
//
//  dBiz revisited version of 
//
//  Cartesian Sequencer Module for VCV similar to Make Noise Rene
//
//  Strum 2017
//  strum@softhome.net
//
///////////////////////////////////////////////////////////////////

#include "dBiz.hpp"
#include "dsp/digital.hpp"

struct MentalCartesian : Module {
	enum ParamIds {
    BUTTON_PARAM,
    KNOB_PARAM = BUTTON_PARAM + 16,
    NUM_PARAMS = KNOB_PARAM + 16		
	};  
	enum InputIds {		
		UP,
    DOWN,
    LEFT,
    RIGHT,
    X_CV,
    Y_CV,
    RESET,
    X_RESET,
    Y_RESET,
		NUM_INPUTS
	};
	enum OutputIds {
		UNQUANT_OUT,
    QUANT_OUT,
    ROW_OUT,
    COLUMN_OUT = ROW_OUT + 4,    
		NUM_OUTPUTS = COLUMN_OUT + 4
	};

  SchmittTrigger leftTrigger;
  SchmittTrigger rightTrigger;
  SchmittTrigger upTrigger;
  SchmittTrigger downTrigger;
  SchmittTrigger resetTrigger;
  SchmittTrigger x_resetTrigger;
  SchmittTrigger y_resetTrigger;
    
  SchmittTrigger button_triggers[4][4];
    
  float grid_lights[4][4] = {0.0,0.0,0.0,0.0,
                            0.0,0.0,0.0,0.0,
                            0.0,0.0,0.0,0.0,
                            0.0,0.0,0.0,0.0};
                            
  float button_lights[4][4] = {0.0,0.0,0.0,0.0,
                            0.0,0.0,0.0,0.0,
                            0.0,0.0,0.0,0.0,
                            0.0,0.0,0.0,0.0};
    
  float row_outs[4] = {0.0,0.0,0.0,0.0};
  float column_outs[4] = {0.0,0.0,0.0,0.0};
  
  int x_position = 0;
  int y_position = 0;
    
	MentalCartesian() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS) {}
	void step() override;
   
};

void MentalCartesian::step() {

  	bool step_right = false;
    bool step_left = false;
    bool step_up = false;
    bool step_down = false;
    grid_lights[x_position][y_position] = 1.0;
    // handle clock inputs
    if (inputs[RIGHT].active)
    {
			if (rightTrigger.process(inputs[RIGHT].value))
      {
				step_right = true;
			}
		}
    if (inputs[LEFT].active)
    {
			if (leftTrigger.process(inputs[LEFT].value))
      {
				step_left = true;
			}
		}
    if (inputs[DOWN].active)
    {
			if (downTrigger.process(inputs[DOWN].value))
      {
				step_down = true;
			}
		}
    if (inputs[UP].active)
    {
			if (upTrigger.process(inputs[UP].value))
      {
				step_up = true;
			}
		}
    // resets
    if (resetTrigger.process(inputs[RESET].value))
    {
      grid_lights[x_position][y_position] = 0.0;
		  x_position = 0;
      y_position = 0;
      grid_lights[x_position][y_position] = 1.0;
      step_right = false;
      step_left = false;
      step_up = false;	
      step_down = false;	
	  }
    if (x_resetTrigger.process(inputs[X_RESET].value))
    {
      grid_lights[x_position][y_position] = 0.0;
		  x_position = 0;
      grid_lights[x_position][y_position] = 1.0;
      step_right = false;
      step_left = false;
      step_up = false;	
      step_down = false;	
	  }
    if (y_resetTrigger.process(inputs[Y_RESET].value))
    {
      grid_lights[x_position][y_position] = 0.0;
		  y_position = 0;
      grid_lights[x_position][y_position] = 1.0;
      step_right = false;
      step_left = false;
      step_up = false;	
      step_down = false;	
	  }
    // handle CV inputs
    int x_cv = round(inputs[X_CV].value);
    if (x_cv < 0 ) x_cv = 0;
    if (x_cv > 3 ) x_cv = 3;
    std::div_t division_x;
    division_x = div(x_position + x_cv,4);
    grid_lights[x_position][y_position] = 0.0;
    x_position = division_x.rem;
    grid_lights[x_position][y_position] = 1.0;
    int y_cv = round(inputs[Y_CV].value);
    if (y_cv < 0 ) y_cv = 0;
    if (y_cv > 3 ) y_cv = 3;    
    std::div_t division_y;
    division_y = div(y_position + y_cv,4);
    grid_lights[x_position][y_position] = 0.0;
    y_position = division_y.rem;
    grid_lights[x_position][y_position] = 1.0;
    // handle button triggers
    for (int i = 0 ; i < 4 ; i++)
    {
      for (int j = 0 ; j < 4 ; j++)
      {
        if ((params[BUTTON_PARAM + i + j * 4].value))
        {
          button_lights[i][j] = 1.0;
          grid_lights[x_position][y_position] = 0.0;
          x_position = i;
          y_position = j;
          grid_lights[x_position][y_position] = 1.0;
        } else button_lights[i][j] = 0.0;
      }		  
	  }
    
    // change x and y    
    if (step_right)
    {
      grid_lights[x_position][y_position] = 0.0;
      x_position += 1;
      if (x_position > 3) x_position = 0;
      grid_lights[x_position][y_position] = 1.0;
    }
    if (step_left)
    {
      grid_lights[x_position][y_position] = 0.0;
      x_position -= 1;
      if (x_position < 0) x_position = 3;      
      grid_lights[x_position][y_position] = 1.0;
    }
    if (step_down)
    {
      grid_lights[x_position][y_position] = 0.0;
      y_position += 1;
      if (y_position > 3) y_position = 0;
      grid_lights[x_position][y_position] = 1.0;
    }
    if (step_up)
    {
      grid_lights[x_position][y_position] = 0.0;
      y_position -= 1;
      if (y_position < 0) y_position = 3;      
      grid_lights[x_position][y_position] = 1.0;
    }
    
    /// set outputs
    int which_knob = y_position * 4 + x_position;
    float main_out = params[KNOB_PARAM + which_knob].value;
    
    int oct = round(main_out);
    float left = main_out - oct;
    int semi = round(left * 12);
    float quant_out = oct + semi/12.0;
    for (int i = 0 ; i < 4 ; i++)
    {
      row_outs[i] = params[KNOB_PARAM + y_position * 4 + i ].value;
      column_outs[i] = params[KNOB_PARAM + x_position + i * 4].value;
      outputs[ROW_OUT + i ].value = row_outs[i];
      outputs[COLUMN_OUT + i ].value = column_outs[i];            
    }
    
    outputs[UNQUANT_OUT].value = main_out;
    outputs[QUANT_OUT].value = quant_out;     
}

////////////////////////////////

MentalCartesianWidget::MentalCartesianWidget() {
	MentalCartesian *module = new MentalCartesian();
	setModule(module);
	box.size = Vec(15*23, 380);
  
	{
    SVGPanel *panel = new SVGPanel();
    panel->box.size = box.size;
        panel->setBackground(SVG::load(assetPlugin(plugin,"res/Bene.svg")));
    addChild(panel);
  }
 
  int top = 20;
  int left = 3;
  int column_spacing = 35; 
  int row_spacing = 35;
  int button_offset = 200;

  addInput(createInput<PJ301MIPort>(Vec(left, top), module, MentalCartesian::LEFT));
  addInput(createInput<PJ301MIPort>(Vec(left+column_spacing, top), module, MentalCartesian::RIGHT));
  addInput(createInput<PJ301MIPort>(Vec(left+column_spacing * 2, top), module, MentalCartesian::X_CV));
  addInput(createInput<PJ301MIPort>(Vec(left+column_spacing * 3, top), module, MentalCartesian::X_RESET));
  
  addInput(createInput<PJ301MIPort>(Vec(left, top + 40), module, MentalCartesian::UP));
  addInput(createInput<PJ301MIPort>(Vec(left + column_spacing, top + 40), module, MentalCartesian::DOWN));  
  addInput(createInput<PJ301MIPort>(Vec(left + column_spacing * 2, top + 40), module, MentalCartesian::Y_CV));
  addInput(createInput<PJ301MIPort>(Vec(left + column_spacing * 3, top + 40), module, MentalCartesian::Y_RESET));
    
  addInput(createInput<PJ301MIPort>(Vec(left + column_spacing * 3, top + row_spacing * 2 + 10), module, MentalCartesian::RESET));
  
  addOutput(createOutput<PJ301MOPort>(Vec(304, 10), module, MentalCartesian::UNQUANT_OUT));  
  addOutput(createOutput<PJ301MOPort>(Vec(304, 40), module, MentalCartesian::QUANT_OUT));	
  
  for ( int i = 0 ; i < 4 ; i++)
  {
    for ( int j = 0 ; j < 4 ; j++)
    {
      addParam(createParam<Rogan2PWhite>(Vec(left+column_spacing * i, top + row_spacing * j + 150 ), module, MentalCartesian::KNOB_PARAM + i + j * 4, -2.0, 2.0, 0.0));
      addChild(createValueLight<LargeLight<YellowValueLight>>(Vec(left+column_spacing * i + 7, top + row_spacing * j + 150 + 7), &module->grid_lights[i][j]));
      addParam(createParam<PB61303>(Vec(button_offset+left+column_spacing * i, top + row_spacing * j + 150 ), module, MentalCartesian::BUTTON_PARAM + i + j * 4, 0.0, 1.0, 0.0));
      addChild(createValueLight<LargeLight<YellowValueLight>>(Vec(button_offset+left+column_spacing * i + 4, top + row_spacing * j + 150 + 4), &module->button_lights[i][j]));
    }
    addOutput(createOutput<PJ301MOPort>(Vec(left+column_spacing * i+5, top + row_spacing * 4 + 155 ), module, MentalCartesian::ROW_OUT + i));
    addOutput(createOutput<PJ301MOPort>(Vec(left+column_spacing * 4+5, top + row_spacing * i + 155 ), module, MentalCartesian::COLUMN_OUT + i));
	}  
	

  addChild(createScrew<ScrewSilver>(Vec(15, 0)));
  addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 0)));
  addChild(createScrew<ScrewSilver>(Vec(15, 365)));
  addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 365)));

}
