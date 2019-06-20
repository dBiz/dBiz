///////////////////////////////////////////////////////////////////
//
//  dBiz revisited version of 
//
//  Cartesian Sequencer Module for VCV
// many thx to 
//  Strum 2017
//  strum@softhome.net
//
///////////////////////////////////////////////////////////////////

#include "plugin.hpp"

using namespace std;

struct Bene : Module {
  enum ParamIds
  {
    ROOT_NOTE_PARAM,
    SCALE_PARAM,
    ENUMS(KNOB_PARAM, 16),
    NUM_PARAMS 
  };
  enum InputIds
  {
    ROOT_NOTE_INPUT,
    SCALE_INPUT,
    UP,
    DOWN,
    LEFT,
    RIGHT,
    X_PAD,
    Y_PAD,
    G_PAD,
    RESET,
    X_RESET,
    Y_RESET,
    NUM_INPUTS
  };
  enum OutputIds {
		UNQUANT_OUT,
    QUANT_OUT,
    ENUMS(ROW_OUT, 4),
    ENUMS(COLUMN_OUT, 4),    
		NUM_OUTPUTS
  };

  enum LightIds
  {
    ENUMS(GRID_LIGHTS, 16),
    NUM_LIGHTS
  };

  //copied & fixed these scales http://www.grantmuller.com/MidiReference/doc/midiReference/ScaleReference.html
	  int SCALE_AEOLIAN        [7] = {0, 2, 3, 5, 7, 8, 10};
	  int SCALE_BLUES          [6] = {0, 3, 5, 6, 7, 10}; //FIXED!
	  int SCALE_CHROMATIC      [12]= {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
	  int SCALE_DIATONIC_MINOR [7] = {0, 2, 3, 5, 7, 8, 10};
	  int SCALE_DORIAN         [7] = {0, 2, 3, 5, 7, 9, 10};
	  int SCALE_HARMONIC_MINOR [7] = {0, 2, 3, 5, 7, 8, 11};
	  int SCALE_INDIAN         [7] = {0, 1, 1, 4, 5, 8, 10};
	  int SCALE_LOCRIAN        [7] = {0, 1, 3, 5, 6, 8, 10};
	  int SCALE_LYDIAN         [7] = {0, 2, 4, 6, 7, 9, 10};
	  int SCALE_MAJOR          [7] = {0, 2, 4, 5, 7, 9, 11};
	  int SCALE_MELODIC_MINOR  [9] = {0, 2, 3, 5, 7, 8, 9, 10, 11};
	  int SCALE_MINOR          [7] = {0, 2, 3, 5, 7, 8, 10};
	  int SCALE_MIXOLYDIAN     [7] = {0, 2, 4, 5, 7, 9, 10};
	  int SCALE_NATURAL_MINOR  [7] = {0, 2, 3, 5, 7, 8, 10};
	  int SCALE_PENTATONIC     [5] = {0, 2, 4, 7, 9};
	  int SCALE_PHRYGIAN       [7] = {0, 1, 3, 5, 7, 8, 10};
	  int SCALE_TURKISH        [7] = {0, 1, 3, 5, 7, 10, 11};

  enum Notes
  {
    NOTE_C,
    NOTE_C_SHARP,
    NOTE_D,
    NOTE_D_SHARP,
    NOTE_E,
    NOTE_F,
    NOTE_F_SHARP,
    NOTE_G,
    NOTE_G_SHARP,
    NOTE_A,
    NOTE_A_SHARP,
    NOTE_B,
    NUM_NOTES
  };

  enum Scales
  {
    AEOLIAN,
    BLUES,
    CHROMATIC,
    DIATONIC_MINOR,
    DORIAN,
    HARMONIC_MINOR,
    INDIAN,
    LOCRIAN,
    LYDIAN,
    MAJOR,
    MELODIC_MINOR,
    MINOR,
    MIXOLYDIAN,
    NATURAL_MINOR,
    PENTATONIC,
    PHRYGIAN,
    TURKISH,
    NONE,
    NUM_SCALES
  };

    dsp::SchmittTrigger leftTrigger;
    dsp::SchmittTrigger rightTrigger;
    dsp::SchmittTrigger upTrigger;
    dsp::SchmittTrigger downTrigger;
    dsp::SchmittTrigger resetTrigger;
    dsp::SchmittTrigger x_resetTrigger;
    dsp::SchmittTrigger y_resetTrigger;
    
    dsp::SchmittTrigger button_triggers[4][4];                
    
    float row_outs[4] = {0.0,0.0,0.0,0.0};
    float column_outs[4] = {0.0,0.0,0.0,0.0};

    int x_position = 0;
    int y_position = 0;

    int rootNote = 0;
    int curScaleVal = 0;
    float pitch = 0;
    float previousPitch = 0;

    Bene()
    {
      config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
          configParam(ROOT_NOTE_PARAM, 0.0, Bene::NUM_NOTES - 1 + 0.1, 0,"Root");
          configParam(SCALE_PARAM, 0.0, Bene::NUM_SCALES - 1 + 0.1, 0,"Scale");
          for (int i = 0; i < 16; i++)
          {
            configParam(KNOB_PARAM + i, 0.0, 2.0, 1.0, "Value");
          }
    }


   float closestVoltageInScale(float voltsIn)
   {
    rootNote = params[ROOT_NOTE_PARAM].value + rescale(inputs[ROOT_NOTE_INPUT].value, 0,10,0, Bene::NUM_NOTES - 1);
    curScaleVal =params[SCALE_PARAM].value + rescale(inputs[SCALE_INPUT].value, 0,10,0, Bene::NUM_SCALES - 1);
    int *curScaleArr;
    int notesInScale = 0;
    switch (curScaleVal)
    {
    case AEOLIAN:
      curScaleArr = SCALE_AEOLIAN;
      notesInScale = LENGTHOF(SCALE_AEOLIAN);
      break;
    case BLUES:
      curScaleArr = SCALE_BLUES;
      notesInScale = LENGTHOF(SCALE_BLUES);
      break;
    case CHROMATIC:
      curScaleArr = SCALE_CHROMATIC;
      notesInScale = LENGTHOF(SCALE_CHROMATIC);
      break;
    case DIATONIC_MINOR:
      curScaleArr = SCALE_DIATONIC_MINOR;
      notesInScale = LENGTHOF(SCALE_DIATONIC_MINOR);
      break;
    case DORIAN:
      curScaleArr = SCALE_DORIAN;
      notesInScale = LENGTHOF(SCALE_DORIAN);
      break;
    case HARMONIC_MINOR:
      curScaleArr = SCALE_HARMONIC_MINOR;
      notesInScale = LENGTHOF(SCALE_HARMONIC_MINOR);
      break;
    case INDIAN:
      curScaleArr = SCALE_INDIAN;
      notesInScale = LENGTHOF(SCALE_INDIAN);
      break;
    case LOCRIAN:
      curScaleArr = SCALE_LOCRIAN;
      notesInScale = LENGTHOF(SCALE_LOCRIAN);
      break;
    case LYDIAN:
      curScaleArr = SCALE_LYDIAN;
      notesInScale = LENGTHOF(SCALE_LYDIAN);
      break;
    case MAJOR:
      curScaleArr = SCALE_MAJOR;
      notesInScale = LENGTHOF(SCALE_MAJOR);
      break;
    case MELODIC_MINOR:
      curScaleArr = SCALE_MELODIC_MINOR;
      notesInScale = LENGTHOF(SCALE_MELODIC_MINOR);
      break;
    case MINOR:
      curScaleArr = SCALE_MINOR;
      notesInScale = LENGTHOF(SCALE_MINOR);
      break;
    case MIXOLYDIAN:
      curScaleArr = SCALE_MIXOLYDIAN;
      notesInScale = LENGTHOF(SCALE_MIXOLYDIAN);
      break;
    case NATURAL_MINOR:
      curScaleArr = SCALE_NATURAL_MINOR;
      notesInScale = LENGTHOF(SCALE_NATURAL_MINOR);
      break;
    case PENTATONIC:
      curScaleArr = SCALE_PENTATONIC;
      notesInScale = LENGTHOF(SCALE_PENTATONIC);
      break;
    case PHRYGIAN:
      curScaleArr = SCALE_PHRYGIAN;
      notesInScale = LENGTHOF(SCALE_PHRYGIAN);
      break;
    case TURKISH:
      curScaleArr = SCALE_TURKISH;
      notesInScale = LENGTHOF(SCALE_TURKISH);
      break;
    case NONE:
      return voltsIn;
    }

    float closestVal = 10.0;
    float closestDist = 10.0;
    float scaleNoteInVolts = 0;
    float distAway = 0;
    int octaveInVolts = int(floorf(voltsIn));
    float voltMinusOct = voltsIn - octaveInVolts;
    		for (int i=0; i < notesInScale; i++) {
			scaleNoteInVolts = curScaleArr[i] / 12.0;
			distAway = fabs(voltMinusOct - scaleNoteInVolts);
			if(distAway < closestDist){
				closestVal = scaleNoteInVolts;
				closestDist = distAway;
			}
    }
    return octaveInVolts + rootNote/12.0 + closestVal;
   }
  
    void process(const ProcessArgs &args) override 
  {

    // Quantization based on JW quantizer module!!!



  	bool step_right = false;
    bool step_left = false;
    bool step_up = false;
    bool step_down = false;
    lights[GRID_LIGHTS+x_position+y_position*4].value =1.0;
    

    // handle clock inputs
    if (inputs[RIGHT].isConnected())
    {
			if (rightTrigger.process(inputs[RIGHT].value))
      {
		  	step_right = true;
		  }
		}
    if (inputs[LEFT].isConnected())
    {
			if (leftTrigger.process(inputs[LEFT].value))
      {
				step_left = true;
			}
		}
    if (inputs[DOWN].isConnected())
    {
			if (downTrigger.process(inputs[DOWN].value))
      {
				step_down = true;
			}
		}
    if (inputs[UP].isConnected())
    {
			if (upTrigger.process(inputs[UP].value))
      {
				step_up = true;
			}
		}
    // resets

    if (resetTrigger.process(inputs[RESET].value))
    {
      lights[GRID_LIGHTS + x_position + y_position*4].value = 0.0;
      x_position = 0;
      y_position = 0;
      lights[GRID_LIGHTS + x_position + y_position*4].value = 1.0;
      step_right = false;
      step_left = false;
      step_up = false;	
      step_down = false;	
	  }
    if (x_resetTrigger.process(inputs[X_RESET].value))
    {
      lights[GRID_LIGHTS + x_position + y_position*4].value  = 0.0;
		  x_position = 0;
      lights[GRID_LIGHTS + x_position + y_position*4].value  = 1.0;
      step_right = false;
      step_left = false;
      step_up = false;	
      step_down = false;	
	  }
    if (y_resetTrigger.process(inputs[Y_RESET].value))
    {
      lights[GRID_LIGHTS + x_position + y_position*4].value  = 0.0;
		  y_position = 0;
      lights[GRID_LIGHTS + x_position + y_position*4].value  = 1.0;
      step_right = false;
      step_left = false;
      step_up = false;	
      step_down = false;	
	  }
   
   
    // handle button triggers

    int xpad = round(inputs[X_PAD].value);
    int ypad = round(inputs[Y_PAD].value);

    bool gated = inputs[G_PAD].value > 0.0;

    if (gated)
    {
      for (int i = 0; i < 4; i++)
      {
        for (int j = 0; j < 4; j++)
        {
          lights[GRID_LIGHTS + x_position + y_position*4].value = 0.0;
          x_position = xpad-1;
          y_position = ypad-1;
          lights[GRID_LIGHTS + x_position + y_position*4].value = 1.0;
      }
    }
    }

    // change x and y    
    if (step_right)
    {
       lights[GRID_LIGHTS + x_position + y_position*4].value  = 0.0;
      x_position += 1;
      if (x_position > 3) x_position = 0;
       lights[GRID_LIGHTS + x_position + y_position*4].value  = 1.0;
    }
    if (step_left)
    {
       lights[GRID_LIGHTS + x_position + y_position*4].value  = 0.0;
      x_position -= 1;
      if (x_position < 0) x_position = 3;      
       lights[GRID_LIGHTS + x_position + y_position*4].value  = 1.0;
    }
    if (step_down)
    {
       lights[GRID_LIGHTS + x_position + y_position*4].value  = 0.0;
      y_position += 1;
      if (y_position > 3) y_position = 0;
       lights[GRID_LIGHTS + x_position + y_position*4].value  = 1.0;
    }
    if (step_up)
    {
       lights[GRID_LIGHTS + x_position + y_position*4].value  = 0.0;
      y_position -= 1;
      if (y_position < 0) y_position = 3;      
       lights[GRID_LIGHTS + x_position + y_position*4].value  = 1.0;
    }
    
    /// set outputs
    int which_knob = y_position * 4 + x_position;

    for (int i = 0 ; i < 4 ; i++)
    {
    float main_out = params[KNOB_PARAM + which_knob].value;
    float quant_out = closestVoltageInScale(params[KNOB_PARAM + which_knob].value);
    row_outs[i] = closestVoltageInScale(params[KNOB_PARAM + y_position * 4 + i].value);
    column_outs[i] = closestVoltageInScale(params[KNOB_PARAM + x_position + i * 4].value);

    outputs[ROW_OUT + i].value = row_outs[i];
    outputs[COLUMN_OUT + i].value = column_outs[i];

    outputs[UNQUANT_OUT].value = main_out;
    outputs[QUANT_OUT].value = quant_out;
   }
  }
};

//////////////////////////////////// Display --- Based on DTROY by Bidoo  

struct BeneDisplay : TransparentWidget{
  Bene *module;
  int frame = 0;
  std::shared_ptr<Font> font;

  std::string note, scale;

  BeneDisplay()
  {

    font = (APP->window->loadFont(asset::plugin(pluginInstance, "res/Rounded_Elegance.ttf")));
  }

  void drawMessage(NVGcontext *vg, Vec pos, std::string note, std::string scale)
  {
    nvgFontSize(vg, 18);
    nvgFontFaceId(vg, font->handle);
    nvgTextLetterSpacing(vg, -2);
    nvgFillColor(vg, nvgRGBA(0xff, 0xff, 0xff, 0xff));
    nvgText(vg, pos.x + 8, pos.y + 23, note.c_str(), NULL);
    nvgText(vg, pos.x + 30, pos.y + 23, scale.c_str(), NULL);
  
  }

  std::string displayRootNote(int value)
  {
    switch (value)
    {
    case Bene::NOTE_C:
      return "C";
    case Bene::NOTE_C_SHARP:
      return "C#";
    case Bene::NOTE_D:
      return "D";
    case Bene::NOTE_D_SHARP:
      return "D#";
    case Bene::NOTE_E:
      return "E";
    case Bene::NOTE_F:
      return "F";
    case Bene::NOTE_F_SHARP:
      return "F#";
    case Bene::NOTE_G:
      return "G";
    case Bene::NOTE_G_SHARP:
      return "G#";
    case Bene::NOTE_A:
      return "A";
    case Bene::NOTE_A_SHARP:
      return "A#";
    case Bene::NOTE_B:
      return "B";
    default:
      return "";
    }
  }

  std::string displayScale(int value)
  {
    switch (value)
    {
    case Bene::AEOLIAN:
      return "Aeolian";
    case Bene::BLUES:
      return "Blues";
    case Bene::CHROMATIC:
      return "Chromatic";
    case Bene::DIATONIC_MINOR:
      return "Diat. Min.";
    case Bene::DORIAN:
      return "Dorian";
    case Bene::HARMONIC_MINOR:
      return "Harm. Min.";
    case Bene::INDIAN:
      return "Indian";
    case Bene::LOCRIAN:
      return "Locrian";
    case Bene::LYDIAN:
      return "Lydian";
    case Bene::MAJOR:
      return "Major";
    case Bene::MELODIC_MINOR:
      return "Melo. Min.";
    case Bene::MINOR:
      return "Minor";
    case Bene::MIXOLYDIAN:
      return "Mixolydian";
    case Bene::NATURAL_MINOR:
      return "Nat. Min.";
    case Bene::PENTATONIC:
      return "Pentatonic";
    case Bene::PHRYGIAN:
      return "Phrygian";
    case Bene::TURKISH:
      return "Turkish";
    case Bene::NONE:
      return "None";
    default:
      return "";
    }
  }

  void draw(NVGcontext *vg) override
  {
    if (++frame >= 4)
    {
      frame = 0;
      note = displayRootNote(module->rootNote);
      scale = displayScale(module->curScaleVal);
    }
    drawMessage(vg, Vec(0, 20), note, scale);
  }
};

/////////////////////////////////

struct BeneWidget : ModuleWidget{
BeneWidget(Bene *module){
  setModule(module);
  setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Bene.svg")));

  int top = 20;
  int top2 = 35;
  int left = 8;
  int column_spacing = 35;
  int row_spacing = 35;

  if (module != NULL)
  {
    BeneDisplay *display = createWidget<BeneDisplay>(Vec(10, 95));
    display->module = module;
    display->box.pos = Vec(left, top + 105);
    display->box.size = Vec(250, 60);
    addChild(display);
  }    


  addInput(createInput<PJ301MCPort>(Vec(left, top), module, Bene::LEFT));
  addInput(createInput<PJ301MCPort>(Vec(left+column_spacing, top), module, Bene::RIGHT));

  addInput(createInput<PJ301MCPort>(Vec(left, top + 40), module, Bene::UP));
  addInput(createInput<PJ301MCPort>(Vec(left + column_spacing, top + 40), module, Bene::DOWN));
  
  addInput(createInput<PJ301MCPort>(Vec(left+column_spacing * 2, top), module, Bene::X_RESET));
  addInput(createInput<PJ301MCPort>(Vec(left + column_spacing * 2, top + 40), module, Bene::Y_RESET));

  addInput(createInput<PJ301MOrPort>(Vec(left , top+85), module, Bene::X_PAD));
  addInput(createInput<PJ301MOrPort>(Vec(left + column_spacing , top + 85), module, Bene::Y_PAD));
  addInput(createInput<PJ301MOrPort>(Vec(left + column_spacing * 2, top + 85), module, Bene::G_PAD));

  addInput(createInput<PJ301MCPort>(Vec(left + column_spacing * 3, top ), module, Bene::RESET));

  addOutput(createOutput<PJ301MOPort>(Vec(left + column_spacing * 5-20, top), module, Bene::UNQUANT_OUT));
  addOutput(createOutput<PJ301MOPort>(Vec(left + column_spacing * 5-20, top+30), module, Bene::QUANT_OUT));
 
  for ( int i = 0 ; i < 4 ; i++)
  {
    for ( int j = 0 ; j < 4 ; j++)
    {
      addParam(createParam<Rogan2PWhite>(Vec(left+column_spacing * i, top2 + row_spacing * j + 150 ), module, Bene::KNOB_PARAM + i + j * 4));
      addChild(createLight<BigLight<OrangeLight>>(Vec(left + column_spacing * i + 8, top2 + row_spacing * j + 150 + 8), module, Bene::GRID_LIGHTS + i + j * 4));
    }
    addOutput(createOutput<PJ301MOPort>(Vec(left+column_spacing * i+5, top2 + row_spacing * 4 + 155 ), module, Bene::ROW_OUT + i));
    addOutput(createOutput<PJ301MOPort>(Vec(left+column_spacing * 4+5, top2 + row_spacing * i + 155 ), module, Bene::COLUMN_OUT + i));
	}

  addParam(createParam<Rogan2PWhite>(Vec(left + column_spacing*3-5, top + 85 + row_spacing), module, Bene::ROOT_NOTE_PARAM));
  addParam(createParam<Rogan2PWhite>(Vec(left + column_spacing*4 , top + 85 + row_spacing), module, Bene::SCALE_PARAM));

  addInput(createInput<PJ301MCPort>(Vec(column_spacing * 4-25, top + 85), module, Bene::ROOT_NOTE_INPUT));
  addInput(createInput<PJ301MCPort>(Vec(column_spacing * 4 +15, top + 85), module, Bene::SCALE_INPUT));

  addChild(createWidget<ScrewBlack>(Vec(15, 0)));
  addChild(createWidget<ScrewBlack>(Vec(box.size.x-30, 0)));
  addChild(createWidget<ScrewBlack>(Vec(15, 365)));
  addChild(createWidget<ScrewBlack>(Vec(box.size.x-30, 365)));
}
};
Model *modelBene = createModel<Bene, BeneWidget>("Bene");
