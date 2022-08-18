////////////////////////////////////////////////////////////////////////////
// <Quantizer>
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

using namespace std;

/////added fine out /////////////////////////////////////////////////
struct Utility : Module {
  enum ParamIds
  {
    LINK_A_PARAM,
    LINK_B_PARAM,
    ROOT_NOTE_PARAM,
    SCALE_PARAM,
    ENUMS(OCTAVE_SHIFT, 3),
    ENUMS(SEMITONE_SHIFT, 3),
    ENUMS(FINE_SHIFT, 3),
    ENUMS(AMOUNT_PARAM, 3),
    NUM_PARAMS
  };
  enum InputIds {
    ROOT_NOTE_INPUT,
    SCALE_INPUT,
	  ENUMS(OCTAVE_INPUT, 3),
    ENUMS(OCTAVE_CVINPUT, 3),
    ENUMS(SEMITONE_CVINPUT, 3),
    ENUMS(FINE_CVINPUT, 3),
    ENUMS(AMOUNT_CVINPUT, 3),
    NUM_INPUTS
	};
	enum OutputIds {
	A_OUTPUT,
    B_OUTPUT,
    C_OUTPUT,
    NUM_OUTPUTS
};

    enum LighIds {
        ENUMS(AMOUNT_LIGHT, 3),
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

  int rootNote = 0;
  int curScaleVal = 0;
  float octave_out[3] {};
  float semitone_out[3] {};
  float fine_out[3] {};

  int panelTheme;


  Utility()
  {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

     configParam(LINK_A_PARAM,  0.0, 1.0, 0.0,"Link A");
     configParam(LINK_B_PARAM,  0.0, 1.0, 0.0,"Link B");
     configParam(ROOT_NOTE_PARAM,  0.0, Utility::NUM_NOTES - 1 + 0.1, 0,"Root Note");
     configParam(SCALE_PARAM,  0.0, Utility::NUM_SCALES - 1 + 0.1, 0,"Scale");
    // params[AMOUNT_PARAM,  "");

    for(int i=0;i<3;i++){
     configParam(OCTAVE_SHIFT+i,  -4.5, 4.5, 0.0,"Octave shift");
     configParam(SEMITONE_SHIFT+i,  -5.0 ,5.0, 0.0,"Semitone shift");
     configParam(FINE_SHIFT+i,  -1.0, 1.0, 0.0,"Fine tune");
    }
    onReset();

		panelTheme = (loadDarkAsDefault() ? 1 : 0);
  }

    json_t *dataToJson() override {
      json_t *rootJ = json_object();

      // panelTheme
      json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));
      return rootJ;
      }
      void dataFromJson(json_t *rootJ) override {
        // panelTheme
        json_t *panelThemeJ = json_object_get(rootJ, "panelTheme");
        if (panelThemeJ)
          panelTheme = json_integer_value(panelThemeJ);
      }


  float closestVoltageInScale(float voltsIn)
  {
    rootNote = params[ROOT_NOTE_PARAM].getValue() + rescale(inputs[ROOT_NOTE_INPUT].getVoltage(), 0,10,0, Utility::NUM_NOTES - 1);
    curScaleVal = params[SCALE_PARAM].getValue() + rescale(inputs[SCALE_INPUT].getVoltage(), 0,10,0, Utility::NUM_SCALES - 1);
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


  if(params[LINK_A_PARAM].getValue() ==1.0 )
    inputs[OCTAVE_INPUT + 1].setVoltage(inputs[OCTAVE_INPUT + 0].getVoltage());

  if (params[LINK_B_PARAM].getValue() == 1.0)
    inputs[OCTAVE_INPUT + 2].setVoltage(inputs[OCTAVE_INPUT + 1].getVoltage());

  for (int i = 0; i < 3; i++)
  {
    octave_out[i] = inputs[OCTAVE_INPUT + i].getVoltage() + round(params[OCTAVE_SHIFT + i].getValue()) + round(inputs[OCTAVE_CVINPUT + i].getVoltage() / 2);
    semitone_out[i] = octave_out[i] + round(params[SEMITONE_SHIFT + i].getValue()) * (1.0 / 12.0) + round(inputs[SEMITONE_CVINPUT + i].getVoltage() / 2) * (1.0 / 12.0);
    fine_out[i] = (params[FINE_SHIFT + i].getValue()) * (1.0 / 12.0) + (inputs[FINE_CVINPUT + i].getVoltage() / 2) * (1.0 / 2.0);
  }

    float out_a = closestVoltageInScale(semitone_out[0])+fine_out[0];
    float out_b = closestVoltageInScale(semitone_out[1])+fine_out[1];
    float out_c = closestVoltageInScale(semitone_out[2])+fine_out[2];

    outputs[A_OUTPUT].setVoltage(out_a);
    outputs[B_OUTPUT].setVoltage(out_b);
    outputs[C_OUTPUT].setVoltage(out_c);


  }
};


struct UtilityDisplay : TransparentWidget
{
  Utility *module;
  int frame = 0;

  std::string note, scale;

  void drawMessage(NVGcontext *vg, Vec pos, std::string note, std::string scale)
  {
    std::shared_ptr<Font> font = (APP->window->loadFont(asset::plugin(pluginInstance, "res/ShareTechMono-Regular.ttf")));
    if (font)
    {
      nvgFontSize(vg, 16);
      nvgFontFaceId(vg, font->handle);
      nvgTextLetterSpacing(vg, -2);
      nvgFillColor(vg, nvgRGBA(0xff, 0xd4, 0x2a, 0xff));
      nvgText(vg, pos.x + 8, pos.y + 25, note.c_str(), NULL);
      nvgText(vg, pos.x + 30, pos.y + 25, scale.c_str(), NULL);
    }
  }

  std::string displayRootNote(int value)
  {
    switch (value)
    {
    case Utility::NOTE_C:
      return "C";
    case Utility::NOTE_C_SHARP:
      return "C#";
    case Utility::NOTE_D:
      return "D";
    case Utility::NOTE_D_SHARP:
      return "D#";
    case Utility::NOTE_E:
      return "E";
    case Utility::NOTE_F:
      return "F";
    case Utility::NOTE_F_SHARP:
      return "F#";
    case Utility::NOTE_G:
      return "G";
    case Utility::NOTE_G_SHARP:
      return "G#";
    case Utility::NOTE_A:
      return "A";
    case Utility::NOTE_A_SHARP:
      return "A#";
    case Utility::NOTE_B:
      return "B";
    default:
      return "";
    }
  }

  std::string displayScale(int value)
  {
    switch (value)
    {
    case Utility::AEOLIAN:
      return "Aeolian";
    case Utility::BLUES:
      return "Blues";
    case Utility::CHROMATIC:
      return "Chromatic";
    case Utility::DIATONIC_MINOR:
      return "Diat. Min.";
    case Utility::DORIAN:
      return "Dorian";
    case Utility::HARMONIC_MINOR:
      return "Harm. Min.";
    case Utility::INDIAN:
      return "Indian";
    case Utility::LOCRIAN:
      return "Locrian";
    case Utility::LYDIAN:
      return "Lydian";
    case Utility::MAJOR:
      return "Major";
    case Utility::MELODIC_MINOR:
      return "Melo. Min.";
    case Utility::MINOR:
      return "Minor";
    case Utility::MIXOLYDIAN:
      return "Mixolydian";
    case Utility::NATURAL_MINOR:
      return "Nat. Min.";
    case Utility::PENTATONIC:
      return "Pentatonic";
    case Utility::PHRYGIAN:
      return "Phrygian";
    case Utility::TURKISH:
      return "Turkish";
    case Utility::NONE:
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


//////////////////////////////////////////////////////////////////
struct UtilityWidget : ModuleWidget
{


  SvgPanel* darkPanel;
  struct PanelThemeItem : MenuItem {
    Utility *module;
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

    Utility *module = dynamic_cast<Utility*>(this->module);
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
UtilityWidget(Utility *module){
  setModule(module);
  setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Light/Utility.svg")));
  if (module) {
    darkPanel = new SvgPanel();
    darkPanel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Dark/Utility.svg")));
    darkPanel->visible = false;
    addChild(darkPanel);
  }

  if (module != NULL)
    {
      UtilityDisplay *display = createWidget<UtilityDisplay>(Vec(10, 95));
      display->module = module;
      display->box.pos = Vec(10, +240);
      display->box.size = Vec(250, 60);
      addChild(display);
    }

    //Screw
    addChild(createWidget<ScrewBlack>(Vec(15, 0)));
    addChild(createWidget<ScrewBlack>(Vec(box.size.x - 30, 0)));
    addChild(createWidget<ScrewBlack>(Vec(15, 365)));
    addChild(createWidget<ScrewBlack>(Vec(box.size.x - 30, 365)));

    int knob = 35;

    //
    for (int i = 0; i < 3; i++)
    {
      addParam(createParam<FlatASnap>(Vec(10 + knob * i, 20), module, Utility::OCTAVE_SHIFT + i));
      addParam(createParam<FlatASnap>(Vec(10 + knob * i, 60), module, Utility::SEMITONE_SHIFT + i));
      addParam(createParam<FlatA>(Vec(10 + knob * i, 100), module, Utility::FINE_SHIFT + i));

    }

      addInput(createInput<PJ301MIPort>(Vec(12.5 + knob * 0, 100 + knob * 1.3), module, Utility::OCTAVE_INPUT + 0));
      addInput(createInput<PJ301MIPort>(Vec(12.5 + knob * 1, 100 + knob * 1.3), module, Utility::OCTAVE_INPUT + 1));
      addInput(createInput<PJ301MIPort>(Vec(12.5 + knob * 2, 100 + knob * 1.3), module, Utility::OCTAVE_INPUT + 2));

      addInput(createInput<PJ301MCPort>(Vec(12.5 + knob * 0, 130 + knob * 1.3), module, Utility::OCTAVE_CVINPUT + 0));
      addInput(createInput<PJ301MCPort>(Vec(12.5 + knob * 1, 130 + knob * 1.3), module, Utility::OCTAVE_CVINPUT + 1));
      addInput(createInput<PJ301MCPort>(Vec(12.5 + knob * 2, 130 + knob * 1.3), module, Utility::OCTAVE_CVINPUT + 2));

      addInput(createInput<PJ301MCPort>(Vec(12.5 + knob * 0, 160 + knob * 1.3), module, Utility::SEMITONE_CVINPUT + 0));
      addInput(createInput<PJ301MCPort>(Vec(12.5 + knob * 1, 160 + knob * 1.3), module, Utility::SEMITONE_CVINPUT + 1));
      addInput(createInput<PJ301MCPort>(Vec(12.5 + knob * 2, 160 + knob * 1.3), module, Utility::SEMITONE_CVINPUT + 2));

      addInput(createInput<PJ301MCPort>(Vec(12.5 + knob * 0, 190 + knob * 1.3), module, Utility::FINE_CVINPUT + 0));
      addInput(createInput<PJ301MCPort>(Vec(12.5 + knob * 1, 190 + knob * 1.3), module, Utility::FINE_CVINPUT + 1));
      addInput(createInput<PJ301MCPort>(Vec(12.5 + knob * 2, 190 + knob * 1.3), module, Utility::FINE_CVINPUT + 2));


  addParam(createParam<Trimpot>(Vec(65,304), module, Utility::ROOT_NOTE_PARAM));
  addParam(createParam<Trimpot>(Vec(90,304), module, Utility::SCALE_PARAM));

  addInput(createInput<PJ301MIPort>(Vec(10,300), module, Utility::ROOT_NOTE_INPUT));
  addInput(createInput<PJ301MIPort>(Vec(37,300), module, Utility::SCALE_INPUT));

  addOutput(createOutput<PJ301MOPort>(Vec(12.5,335), module, Utility::A_OUTPUT));
  addOutput(createOutput<PJ301MOPort>(Vec(12.5+knob*1,335), module, Utility::B_OUTPUT));
  addOutput(createOutput<PJ301MOPort>(Vec(12.5+knob*2,335), module, Utility::C_OUTPUT));

  addParam(createParam<CKSSS>(Vec(39,150), module, Utility::LINK_A_PARAM));
  addParam(createParam<CKSSS>(Vec(74.5, 150), module, Utility::LINK_B_PARAM));
}
void step() override {
  if (module) {
    Widget* panel = getPanel();
    panel->visible = ((((Utility*)module)->panelTheme) == 0);
    darkPanel->visible  = ((((Utility*)module)->panelTheme) == 1);
  }
  Widget::step();
}
};
Model *modelUtility = createModel<Utility, UtilityWidget>("Utility");
