////////////////////////////////////////////////////////////////////////////
// <Bene - a kind of grid sequencer>
// Copyright (C) <2019>  <Giovanni Ghisleni>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
/////////////////////////////////////////////////////////////////////////////

#include "plugin.hpp"
#define SEQUENCER_LEN 16
#define MAX_PATTERN_LEN 16


using namespace std;
 
struct Bene : Module {
  enum ParamIds
  {
    ROOT_NOTE_PARAM,
    SCALE_PARAM,
    X_LOCK_PARAM,
    Y_LOCK_PARAM,
    X_DIR_PARAM,
    Y_DIR_PARAM,
    ENUMS(KNOB_PARAM, 16),
    ENUMS(GRID_PARAM, 16),
    NUM_PARAMS
  };
  enum InputIds
  {
    ROOT_NOTE_INPUT,
    SCALE_INPUT,
    Y_CLK,
    X_CLK,
    X_DIR_INPUT,
    X_LOCK_INPUT,
    Y_DIR_INPUT,
    Y_LOCK_INPUT,
    X_RESET,
    Y_RESET,
    X_PAD,
    Y_PAD,
    G_PAD,
    RESET,
    NUM_INPUTS
  };
  enum OutputIds {
		GATE_OUT,
    QUANT_OUT,
    TRIG_OUT,
    ENUMS(ROW_OUT, 4),
    ENUMS(COLUMN_OUT, 4),
		NUM_OUTPUTS
  };

  enum LightIds
  {
    ENUMS(GRID_LIGHTS, 16),
    X_DIR_LIGHT,
    Y_DIR_LIGHT,
    X_LOCK_LIGHT,    
    Y_LOCK_LIGHT,
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
    dsp::SchmittTrigger xdirTrigger;
    dsp::SchmittTrigger ydirTrigger;
    dsp::SchmittTrigger xlockTrigger;
    dsp::SchmittTrigger ylockTrigger;
    dsp::SchmittTrigger resetTrigger;
    dsp::SchmittTrigger x_resetTrigger;
    dsp::SchmittTrigger y_resetTrigger;

    dsp::SchmittTrigger gateTrig[4][4];
    dsp::PulseGenerator trig_out[4][4];

    
    bool trig[4][4];


    bool gates[4][4];

    float row_outs[4] = {0.0,0.0,0.0,0.0};
    float column_outs[4] = {0.0,0.0,0.0,0.0};
    float quant_out = 0;
    float trig_gen;

    int x_position = 0;
    int y_position = 0;
    int which_knob =0;

    int rootNote = 0;
    int curScaleVal = 0;
    float pitch = 0;
    float previousPitch = 0;

    bool xdir=false;
    bool ydir=false;
    bool xlock=false;
    bool ylock=false;

	float consumerMessage[3] = {};// this module must read from here
	float producerMessage[3] = {};// mother will write into here


    int panelTheme;

    Bene()
    {
      config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
          configParam(ROOT_NOTE_PARAM, 0.0, Bene::NUM_NOTES - 1 + 0.1, 0,"Root");
          configParam(SCALE_PARAM, 0.0, Bene::NUM_SCALES - 1 + 0.1, 0,"Scale");
          for (int i = 0; i < 16; i++)
          {
            configParam(KNOB_PARAM + i, -2.0, 2.0, 0.0, "Note Range");
            configButton(GRID_PARAM + i,"Gate Step");
          }
          configButton(X_DIR_PARAM ,"Direction X");
          configButton(Y_DIR_PARAM ,"Direction Y");
          configButton(X_LOCK_PARAM,"Lock X");
          configButton(Y_LOCK_PARAM,"Lock Y");
	    
	  configOutput(GATE_OUT, "Gate");
          configOutput(QUANT_OUT, "V/Oct");
          configOutput(TRIG_OUT, "Trigger");

        rightExpander.producerMessage = producerMessage;
		    rightExpander.consumerMessage = consumerMessage;

          onReset();

      		panelTheme = (loadDarkAsDefault() ? 1 : 0);
    }

 void onReset() override
  {
    for (int i = 0; i < 4; i++)
    {
      for (int j = 0; j < 4; j++)
      {
        gates[i][j] = false;
        trig[i][j] = false;
      }
    }
  }
      json_t *dataToJson() override 
      {
        json_t *rootJ = json_object();

        json_t *gate_statesJ = json_array();
          for (int i = 0; i < 4; i++)
          {
            for (int j = 0; j < 4; j++)
            {
              json_t *gate_stateJ = json_boolean(gates[i][j]);
              json_array_append_new(gate_statesJ, gate_stateJ);
            }
          }
          json_object_set_new(rootJ, "gates", gate_statesJ);

        // panelTheme
        json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));
        return rootJ;
      }
      void dataFromJson(json_t *rootJ) override 
      {

        json_t *gate_statesJ = json_object_get(rootJ, "gates");
        if (gate_statesJ)
        {
          for (int i = 0; i < 4; i++)
          {
            for (int j = 0; j < 4; j++)
            {
              json_t *gate_stateJ = json_array_get(gate_statesJ, j+i*4);
              if (gate_stateJ)
                gates[i][j] = json_boolean_value(gate_stateJ);
            }
          }
        }
          // panelTheme
          json_t *panelThemeJ = json_object_get(rootJ, "panelTheme");
          if (panelThemeJ)
            panelTheme = json_integer_value(panelThemeJ);
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

    //////////////////////GATES/////////////////
    float deltaTime = 1.0f / args.sampleRate;
    which_knob = y_position * 4 + x_position;

    for (int i = 0; i < 4; i++)
          {
            for (int j = 0; j < 4; j++)
            {
              if (gateTrig[i][j].process(params[GRID_PARAM+i+j*4].getValue()))
              {
                gates[i][j]= !gates[i][j];
              }
              lights[GRID_LIGHTS +i+j*4].setSmoothBrightness(gates[i][j] ? 0.2f : 0.f,args.sampleTime);
            }
          }



    //////// Loop direction /////////////////////////////
    if (xdirTrigger.process(params[X_DIR_PARAM].getValue()+inputs[X_DIR_INPUT].getVoltage()))
    {
      xdir = !xdir;
    }
    lights[X_DIR_LIGHT].setSmoothBrightness(xdir ? 1.0 : 0.0, args.sampleTime);

    if (ydirTrigger.process(params[Y_DIR_PARAM].getValue()+inputs[Y_DIR_INPUT].getVoltage()))
    {
      ydir = !ydir;
    }
      lights[Y_DIR_LIGHT].setSmoothBrightness(ydir ? 1.0 : 0.0, args.sampleTime);

    if (xlockTrigger.process(params[X_LOCK_PARAM].getValue()+inputs[X_LOCK_INPUT].getVoltage()))
    {
      xlock = !xlock;
    }
    lights[X_LOCK_LIGHT].setSmoothBrightness(xlock ? 1.0 : 0.0, args.sampleTime);

    if (ylockTrigger.process(params[Y_LOCK_PARAM].getValue()+inputs[Y_LOCK_INPUT].getVoltage()))
    {
      ylock = !ylock;
    }
      lights[Y_LOCK_LIGHT].setSmoothBrightness(ylock ? 1.0 : 0.0, args.sampleTime);


    ///////////// CLK ////////////////////////////////////


  	bool step_right = false;
    bool step_left = false;
    bool step_up = false;
    bool step_down = false;

   //float xd = inputs[X_CV].getVoltage();
   //float yd = inputs[Y_CV].getVoltage();

    // handle clock inputs

    if (inputs[Y_CLK].isConnected())
    {
			if (rightTrigger.process(inputs[Y_CLK].value))
      {
        if(xdir) step_up = true;
        else step_down = true;
		  }
    }

    if (inputs[X_CLK].isConnected())
    {
			if (leftTrigger.process(inputs[X_CLK].value))
      {
        if(xdir) step_left = true;
        else step_right = true;
			}
		}

   /////////// resets

    if (x_resetTrigger.process(inputs[X_RESET].value))
    {
      lights[GRID_LIGHTS + x_position + y_position*4].value=0;
		  x_position = 0;
      lights[GRID_LIGHTS + x_position + y_position*4].value=1;
      step_right = false;
      step_left = false;
      step_up = false;
      step_down = false;
	  }
    if (y_resetTrigger.process(inputs[Y_RESET].value))
    {
      lights[GRID_LIGHTS + x_position + y_position*4].value=0;
		  y_position = 0;
      lights[GRID_LIGHTS + x_position + y_position*4].value=1;
      step_right = false;
      step_left = false;
      step_up = false;
      step_down = false;
	  }


    // handle button triggers
   
    if(rightExpander.module && rightExpander.module->model == modelBenePads) {
	   float *messagesFromExpander = (float*)rightExpander.consumerMessage;
     int xpad = round(messagesFromExpander[0]);
     int ypad = round(messagesFromExpander[1]);
     bool gated = messagesFromExpander[2]>0.0;

     if (gated)
     {
       for (int i = 0; i < 4; i++)
       {
         for (int j = 0; j < 4; j++)
         {
           lights[GRID_LIGHTS + x_position + y_position*4].value=0;
           x_position = xpad-1;
           y_position = ypad-1;
           lights[GRID_LIGHTS + x_position + y_position*4].value=1;
         }
       }
     }
    }
   
    // change x and y
    if (step_right)
    {
      lights[GRID_LIGHTS + x_position + y_position*4].value=0;
      x_position += 1;
      if (x_position > 3)
      {
        x_position = 0;
        if(!xlock)
        {
          y_position += 1 ;
          if(y_position>3) y_position = 0;
        }
      }
      lights[GRID_LIGHTS + x_position + y_position*4].value=1;
    }
    if (step_left)
    {
      lights[GRID_LIGHTS + x_position + y_position*4].value=0;
      x_position -= 1;
      if (x_position < 0)
      {
        x_position = 3;
        if(!xlock)
        {
          y_position -= 1 ;
          if(y_position<0) y_position = 3;
        }
      }
      lights[GRID_LIGHTS + x_position + y_position*4].value=1;
    }
    if (step_down)
    {
      lights[GRID_LIGHTS + x_position + y_position*4].value=0;
      y_position += 1;
      if (y_position > 3)
      {
        y_position = 0;
        if(!ylock)
        {
          x_position += 1 ;
          if(x_position>3) x_position = 0;
        }
      }
      lights[GRID_LIGHTS + x_position + y_position*4].value=1;
    }
    if (step_up)
    {
      lights[GRID_LIGHTS + x_position + y_position*4].value=0;
      y_position -= 1;
      if (y_position < 0)
      {
        y_position = 3;
        if(!ylock)
        {
          x_position -= 1 ;
          if(x_position<0) x_position = 3;
        }
      }
      lights[GRID_LIGHTS + x_position + y_position*4].value=1;
    }

    /// set outputs

    if (gates[x_position][y_position])
    {
      quant_out = closestVoltageInScale(params[KNOB_PARAM + which_knob].value);
      lights[GRID_LIGHTS + x_position + y_position * 4].value = 1;

  }
  for (int i = 0 ; i < 4 ; i++) 
  {
    row_outs[i] = closestVoltageInScale(params[KNOB_PARAM + y_position * 4 + i].value);
    column_outs[i] = closestVoltageInScale(params[KNOB_PARAM + x_position + i * 4].value);

    outputs[ROW_OUT + i].value = row_outs[i];
    outputs[COLUMN_OUT + i].value = column_outs[i];
  }
  
  outputs[QUANT_OUT].setVoltage(quant_out);

  outputs[GATE_OUT].setVoltage(gates[x_position][y_position] ? 10.f : 0.f);

//outputs a trigger using gates[] and step conditions.

  if(gates[x_position][y_position])

    if (step_up or step_down or step_left or step_right)

      trig_out[x_position][y_position].trigger(1e-3);


  trig[x_position][y_position] = trig_out[x_position][y_position].process(deltaTime);

  outputs[TRIG_OUT].setVoltage(trig[x_position][y_position]?10.f:0.f);


  if(rightExpander.module && rightExpander.module->model == modelBenePads) 
  {
  	float *messagesFromExpander = (float*)rightExpander.consumerMessage;
    bool trigpad = messagesFromExpander[2]>0;
    outputs[QUANT_OUT].setVoltage(closestVoltageInScale(params[KNOB_PARAM + which_knob].value));
    outputs[GATE_OUT].setVoltage(trigpad || gates[x_position][y_position] ? 10.f : 0.f);
  }
    }
};

template <typename BASE>
struct ULight : BASE
{
  ULight()
  {
    this->box.size = mm2px(Vec(5, 5));
  }
};

//////////////////////////////////// Display --- Based on DTROY by Bidoo

struct BeneDisplay : TransparentWidget{
  Bene *module;
  int frame = 0;

  std::string note, scale;


  void drawMessage(NVGcontext *vg, Vec pos, std::string note, std::string scale)
  {
    std::shared_ptr<Font> font = (APP->window->loadFont(asset::plugin(pluginInstance, "res/ShareTechMono-Regular.ttf")));
    if (font){
    nvgFontSize(vg, 16);
    nvgFontFaceId(vg, font->handle);
    nvgTextLetterSpacing(vg, -2);
    nvgFillColor(vg, nvgRGBA(0xff, 0xd4, 0x2a, 0xff));
    nvgText(vg, pos.x + 8, pos.y + 23, note.c_str(), NULL);
    nvgText(vg, pos.x + 25, pos.y + 23, scale.c_str(), NULL);
    }
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
	
	int lastPanelTheme = -1;
	std::shared_ptr<window::Svg> light_svg;
	std::shared_ptr<window::Svg> dark_svg;
	
  struct PanelThemeItem : MenuItem {
    Bene *module;
    int theme;
    void onAction(const event::Action &e) override {
      module->panelTheme = theme;
    }
    void step() override {
      rightText = (module->panelTheme == theme) ? "✔" : "";
    }
  };
  void appendContextMenu(Menu *menu) override {
    MenuLabel *spacerLabel = new MenuLabel();
    menu->addChild(spacerLabel);

    Bene *module = dynamic_cast<Bene*>(this->module);
    assert(module);

    MenuLabel *themeLabel = new MenuLabel();
    themeLabel->text = "Panel Theme";
    menu->addChild(themeLabel);

    PanelThemeItem *lightItem = new PanelThemeItem();
    lightItem->text = lightPanelID;
    lightItem->module = module;
    lightItem->theme = 0;
    menu->addChild(lightItem);

    PanelThemeItem *darkItem = new PanelThemeItem();
    darkItem->text = darkPanelID;
    darkItem->module = module;
    darkItem->theme = 1;
    menu->addChild(darkItem);

    menu->addChild(createMenuItem<DarkDefaultItem>("Dark as default", CHECKMARK(loadDarkAsDefault())));
  }
BeneWidget(Bene *module){
  setModule(module);
  
    // Main panels from Inkscape
 	light_svg = APP->window->loadSvg(asset::plugin(pluginInstance, "res/Light/Bene.svg"));
	dark_svg = APP->window->loadSvg(asset::plugin(pluginInstance, "res/Dark/Bene.svg"));
	int panelTheme = isDark(module ? (&(((Bene*)module)->panelTheme)) : NULL) ? 1 : 0;// need this here since step() not called for module browser
	setPanel(panelTheme == 0 ? light_svg : dark_svg);	
  
  int top = 15;
  int top2 = 35;
  int left = 8;
  int left2 = 30;
  int column_spacing = 35;
  int row_spacing = 35;

  if (module != NULL)
  {
    BeneDisplay *display = createWidget<BeneDisplay>(Vec(10, 65));
    display->module = module;
    display->box.pos = Vec(left, 110);
    display->box.size = Vec(250, 60);
    addChild(display);
  }


  addInput(createInput<PJ301MCPort>(Vec(left2, top), module, Bene::X_CLK));
  addInput(createInput<PJ301MCPort>(Vec(left2 + 30, top), module, Bene::Y_CLK));

  addInput(createInput<PJ301MCPort>(Vec(left2 + 60, top), module, Bene::X_RESET));
  addInput(createInput<PJ301MCPort>(Vec(left2 + 90, top), module, Bene::Y_RESET));

  addInput(createInput<PJ301MCPort>(Vec(left2 + 60, top + 35), module, Bene::X_DIR_INPUT));
  addInput(createInput<PJ301MCPort>(Vec(left2 + 60, top + 65), module, Bene::Y_DIR_INPUT));

  addInput(createInput<PJ301MCPort>(Vec(left2 + 90, top + 35), module, Bene::X_LOCK_INPUT));
  addInput(createInput<PJ301MCPort>(Vec(left2 + 90, top + 65), module, Bene::Y_LOCK_INPUT));


   addParam(createLightParam<LEDLightBezel<OrangeLight>>(Vec(left2 , top + 40  ), module, Bene::X_DIR_PARAM, Bene::X_DIR_LIGHT));
   addParam(createLightParam<LEDLightBezel<OrangeLight>>(Vec(left2 , top + 70 ), module, Bene::Y_DIR_PARAM, Bene::Y_DIR_LIGHT));

   addParam(createLightParam<LEDLightBezel<GreenLight>>(Vec(left2 +30, top + 40  ), module, Bene::X_LOCK_PARAM, Bene::X_LOCK_LIGHT));
   addParam(createLightParam<LEDLightBezel<GreenLight>>(Vec(left2 +30, top + 70 ), module, Bene::Y_LOCK_PARAM, Bene::Y_LOCK_LIGHT));


  addOutput(createOutput<PJ301MOPort>(Vec(160,20), module, Bene::QUANT_OUT));
  addOutput(createOutput<PJ301MOPort>(Vec(160,50), module, Bene::GATE_OUT));
  addOutput(createOutput<PJ301MOPort>(Vec(160, 80), module, Bene::TRIG_OUT));

  for ( int i = 0 ; i < 4 ; i++)
  {
    for ( int j = 0 ; j < 4 ; j++)
    {
      addParam(createParam<VerboDS>(Vec(left+column_spacing * i +1 , top2 + row_spacing * j + 150 ), module, Bene::KNOB_PARAM + i + j * 4)); 
      addParam(createLightParam<LEDLightBezel<OrangeLight>>(Vec(left + column_spacing * i + 8, top2 + row_spacing * j + 150 + 6), module, Bene::GRID_PARAM + i + j * 4,Bene::GRID_LIGHTS + i + j * 4));
     }
	}
    addOutput(createOutput<PJ301MOPort>(Vec(left+column_spacing * 0+5, top2 + row_spacing * 4 + 155 ), module, Bene::ROW_OUT + 0));
    addOutput(createOutput<PJ301MOPort>(Vec(left+column_spacing * 1+5, top2 + row_spacing * 4 + 155 ), module, Bene::ROW_OUT + 1));
    addOutput(createOutput<PJ301MOPort>(Vec(left+column_spacing * 2+5, top2 + row_spacing * 4 + 155 ), module, Bene::ROW_OUT + 2));
    addOutput(createOutput<PJ301MOPort>(Vec(left+column_spacing * 3+5, top2 + row_spacing * 4 + 155 ), module, Bene::ROW_OUT + 3));

    addOutput(createOutput<PJ301MOPort>(Vec(left+column_spacing * 4+5, top2 + row_spacing * 0 + 155 ), module, Bene::COLUMN_OUT + 0));
    addOutput(createOutput<PJ301MOPort>(Vec(left+column_spacing * 4+5, top2 + row_spacing * 1 + 155 ), module, Bene::COLUMN_OUT + 1));
    addOutput(createOutput<PJ301MOPort>(Vec(left+column_spacing * 4+5, top2 + row_spacing * 2 + 155 ), module, Bene::COLUMN_OUT + 2));
    addOutput(createOutput<PJ301MOPort>(Vec(left+column_spacing * 4+5, top2 + row_spacing * 3 + 155 ), module, Bene::COLUMN_OUT + 3));

  addParam(createParam<FlatA>(Vec(left + column_spacing*3-5, top + 95 + row_spacing), module, Bene::ROOT_NOTE_PARAM));
  addParam(createParam<FlatA>(Vec(left + column_spacing*4 , top + 95 + row_spacing), module, Bene::SCALE_PARAM));

  addInput(createInput<PJ301MCPort>(Vec(2 + left + column_spacing * 3 - 5, top + 100), module, Bene::ROOT_NOTE_INPUT));
  addInput(createInput<PJ301MCPort>(Vec(2 + left + column_spacing * 4, top + 100), module, Bene::SCALE_INPUT));

  addChild(createWidget<ScrewBlack>(Vec(15, 0)));
  addChild(createWidget<ScrewBlack>(Vec(box.size.x-30, 0)));
  addChild(createWidget<ScrewBlack>(Vec(15, 365)));
  addChild(createWidget<ScrewBlack>(Vec(box.size.x-30, 365)));
}
void step() override {
		int panelTheme = isDark(module ? (&(((Bene*)module)->panelTheme)) : NULL) ? 1 : 0;
		if (lastPanelTheme != panelTheme) {
			lastPanelTheme = panelTheme;
			SvgPanel* panel = (SvgPanel*)getPanel();
			panel->setBackground(panelTheme == 0 ? light_svg : dark_svg);
		}
		Widget::step();
	}
};
Model *modelBene = createModel<Bene, BeneWidget>("Bene");
