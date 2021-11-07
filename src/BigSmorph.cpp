////////////////////////////////////////////////////////////////////////////
// <3 Voice quantizer>
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
 
static float DeltaT(float delta, float tau)
{
    float lin = sgn(delta) * 10.f / tau;
    float exp = M_E * delta / tau;
    return crossfade(lin, exp, 0.90f);
}

struct BigSmorph : Module {
    enum ParamIds
    {
        ROOT_NOTE_PARAM,
        SCALE_PARAM,
        RANGE_PARAM,
        ENUMS(SEQA_PARAM, 8),
        ENUMS(SEQB_PARAM, 8),
        ENUMS(SEQC_PARAM, 8),
        ENUMS(GLIDE_PARAM, 3),
        ENUMS(GBUTTON_PARAM, 8),
        NUM_PARAMS
    };
    enum InputIds {
	//	TRIGS_INPUT,
		REV_INPUT,
        CLK_INPUT,
        RESET_INPUT,
        CV_INPUT,
     // LINK_INPUT,
		ENUMS(GATE_INPUT, 8),
		NUM_INPUTS
	};
	enum OutputIds {
      //LINK_OUTPUT,
		ENUMS(SEQ_OUTPUT, 3),
		NUM_OUTPUTS
	};

	enum LightIds
	{
		ENUMS(STEP_LIGHT, 8),
		NUM_LIGHTS
	};
////////////////////////////////////////////////////////

    //copied & fixed these scales http://www.grantmuller.com/MidiReference/doc/midiReference/ScaleReference.html
    int SCALE_AEOLIAN[7] = {0, 2, 3, 5, 7, 8, 10};
    int SCALE_BLUES[6] = {0, 3, 5, 6, 7, 10}; //FIXED!
    int SCALE_CHROMATIC[12] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    int SCALE_DIATONIC_MINOR[7] = {0, 2, 3, 5, 7, 8, 10};
    int SCALE_DORIAN[7] = {0, 2, 3, 5, 7, 9, 10};
    int SCALE_HARMONIC_MINOR[7] = {0, 2, 3, 5, 7, 8, 11};
    int SCALE_INDIAN[7] = {0, 1, 1, 4, 5, 8, 10};
    int SCALE_LOCRIAN[7] = {0, 1, 3, 5, 6, 8, 10};
    int SCALE_LYDIAN[7] = {0, 2, 4, 6, 7, 9, 10};
    int SCALE_MAJOR[7] = {0, 2, 4, 5, 7, 9, 11};
    int SCALE_MELODIC_MINOR[9] = {0, 2, 3, 5, 7, 8, 9, 10, 11};
    int SCALE_MINOR[7] = {0, 2, 3, 5, 7, 8, 10};
    int SCALE_MIXOLYDIAN[7] = {0, 2, 4, 5, 7, 9, 10};
    int SCALE_NATURAL_MINOR[7] = {0, 2, 3, 5, 7, 8, 10};
    int SCALE_PENTATONIC[5] = {0, 2, 4, 7, 9};
    int SCALE_PHRYGIAN[7] = {0, 1, 3, 5, 7, 8, 10};
    int SCALE_TURKISH[7] = {0, 1, 3, 5, 7, 10, 11};

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
    ////////////////////////////////////////////////////////


    float ins[3]{};
    float outs[3] {};

    int index = 0;
    int rootNote = 0;
    int curScaleVal = 0;

    int panelTheme;

    dsp::SchmittTrigger trigger[8];
    dsp::SchmittTrigger clk;
    dsp::SchmittTrigger resetTrigger;

    BigSmorph()
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        configParam(ROOT_NOTE_PARAM, 0.0, BigSmorph::NUM_NOTES - 1 + 0.1, 0, "Root");
        configParam(SCALE_PARAM, 0.0, BigSmorph::NUM_SCALES - 1 + 0.1, 0, "Scale");
        configParam(RANGE_PARAM, 0.0, 1.0, 0.0, "Volt Range");

        for (int i = 0; i < 8; i++)
		{
            configParam(SEQA_PARAM + i, -5.0, 5.0, 0.0, "Seq A Range");
            configParam(SEQB_PARAM + i, -5.0, 5.0, 0.0, "Seq B Range");
            configParam(SEQC_PARAM + i, -5.0, 5.0, 0.0, "Seq C Range");
            configParam(GBUTTON_PARAM + i, 0.0, 1.0, 0.0, "Seq Button");


        }
        for (int i = 0; i < 8; i++)
        {
            configParam(GLIDE_PARAM + i, 0.0, 1.0, 0.0, "Glide");
        }


        onReset();

    	panelTheme = (loadDarkAsDefault() ? 1 : 0);
    }

    json_t *dataToJson() override 
    {
        json_t *rootJ = json_object();

        // panelTheme
        json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));
        return rootJ;
    }

    void dataFromJson(json_t *rootJ) override 
    {
        // panelTheme
        json_t *panelThemeJ = json_object_get(rootJ, "panelTheme");
        if (panelThemeJ)
        panelTheme = json_integer_value(panelThemeJ);
    
    }


////////////////////////////////////////////////////////////////////////////////////
 float closestVoltageInScale(float voltsIn)
   {
    rootNote = params[ROOT_NOTE_PARAM].getValue();
    curScaleVal =params[SCALE_PARAM].getValue();
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

////////////////////////////////////////////////////////////////


	void process(const ProcessArgs &args) override
	{
////////////////////////////////////////
        float seq_a[8]{};
        float seq_b[8]{};
        float seq_c[8]{};

            if (inputs[CLK_INPUT].isConnected())
            {
            if (clk.process(inputs[CLK_INPUT].getVoltage()))
            {
                if (inputs[REV_INPUT].getVoltage() > 0)
                {
                    index--;
                }
                else
                    index++;

                if (index > 7)
                    index = 0;
                if (index < 0)
                    index = 7;
            }
            }

            if (inputs[RESET_INPUT].isConnected())
            {
             if(resetTrigger.process(inputs[RESET_INPUT].getVoltage()))
             {
                  index=0;
             }
            }


            if (inputs[CV_INPUT].isConnected())
            {
            index = round(math::rescale(inputs[CV_INPUT].getVoltage(),-5.0,5.0, 0.0, 7.0f));
            }

        for (int i = 0; i < 8; i++)
        {
            if(i<8)
            {
                if(params[RANGE_PARAM].getValue()==0)
                {
                    seq_a[i] = params[SEQA_PARAM + i].getValue();
                    seq_b[i] = params[SEQB_PARAM + i].getValue();
                    seq_c[i] = params[SEQC_PARAM + i].getValue();
                }
                else
                {
                    seq_a[i] = params[SEQA_PARAM + i].getValue()/2.0;
                    seq_b[i] = params[SEQB_PARAM + i].getValue()/2.0;
                    seq_c[i] = params[SEQC_PARAM + i].getValue()/2.0;
                }
            }


            if (trigger[i].process(params[GBUTTON_PARAM + i].getValue() * 10 + inputs[GATE_INPUT + i].getVoltage()))
            {
                index = i;
            }

            if(i==index) lights[STEP_LIGHT + i].setBrightness(1.0);
            else lights[STEP_LIGHT + i].setBrightness(0.0);

        }

        ins[0] = seq_a[index];
        ins[1] = seq_b[index];
        ins[2] = seq_c[index];




///////////////////////////////////////////////////////////////////////////////////////////
        for (int g=0;g<3;g++)
        {
            float in = closestVoltageInScale(ins[g]);
            float delta = in - outs[g];

            bool rising = false;
            bool falling = false;

            if (delta > 0)
            {
                // Rise
                float riseCv = params[GLIDE_PARAM + g].getValue();
                float rise = 1e-1 * std::pow(2.0, riseCv * 10.0);
                outs[g] += DeltaT(delta, rise) * args.sampleTime;
                rising = (in - outs[g] > 1e-3);
            }
            else if (delta < 0)
            {
                // Fall
                float fallCv = params[GLIDE_PARAM + g].getValue();
                float fall = 1e-1 * std::pow(2.0, fallCv * 10.0);
                outs[g] += DeltaT(delta, fall) * args.sampleTime;
                falling = (in - outs[g] < -1e-3);
            }

            if (!rising && !falling)
            {
                outs[g] = in;
            }

            outputs[SEQ_OUTPUT + g].setVoltage(outs[g]);
        
        }

    }
};

//////////////////////////////////// Display --- Based on DTROY by Bidoo

struct BigSmorphDisplay : TransparentWidget
{
    BigSmorph *module;
    int frame = 0;
    std::shared_ptr<Font> font;

    std::string note, scale;

    BigSmorphDisplay()
    {

        font = (APP->window->loadFont(asset::plugin(pluginInstance, "res/DOTMATRI.ttf")));
    }

    void drawMessage(NVGcontext *vg, Vec pos, std::string note, std::string scale)
    {
        nvgFontSize(vg, 13);
        nvgFontFaceId(vg, font->handle);
        nvgTextLetterSpacing(vg, -2);
        nvgFillColor(vg, nvgRGBA(0xff, 0xd3, 0x2a, 0xff));
        nvgText(vg, pos.x + 25, pos.y + 13, note.c_str(), NULL);
        nvgText(vg, pos.x + 35, pos.y + 13, scale.c_str(), NULL);
    }

    std::string displayRootNote(int value)
    {
        switch (value)
        {
        case BigSmorph::NOTE_C:
            return "C";
        case BigSmorph::NOTE_C_SHARP:
            return "C#";
        case BigSmorph::NOTE_D:
            return "D";
        case BigSmorph::NOTE_D_SHARP:
            return "D#";
        case BigSmorph::NOTE_E:
            return "E";
        case BigSmorph::NOTE_F:
            return "F";
        case BigSmorph::NOTE_F_SHARP:
            return "F#";
        case BigSmorph::NOTE_G:
            return "G";
        case BigSmorph::NOTE_G_SHARP:
            return "G#";
        case BigSmorph::NOTE_A:
            return "A";
        case BigSmorph::NOTE_A_SHARP:
            return "A#";
        case BigSmorph::NOTE_B:
            return "B";
        default:
            return "";
        }
    }

    std::string displayScale(int value)
    {
        switch (value)
        {
        case BigSmorph::AEOLIAN:
            return "Aeolian";
        case BigSmorph::BLUES:
            return "Blues";
        case BigSmorph::CHROMATIC:
            return "Chromatic";
        case BigSmorph::DIATONIC_MINOR:
            return "Diat. Min.";
        case BigSmorph::DORIAN:
            return "Dorian";
        case BigSmorph::HARMONIC_MINOR:
            return "Harm. Min.";
        case BigSmorph::INDIAN:
            return "Indian";
        case BigSmorph::LOCRIAN:
            return "Locrian";
        case BigSmorph::LYDIAN:
            return "Lydian";
        case BigSmorph::MAJOR:
            return "Major";
        case BigSmorph::MELODIC_MINOR:
            return "Melo. Min.";
        case BigSmorph::MINOR:
            return "Minor";
        case BigSmorph::MIXOLYDIAN:
            return "Mixolydian";
        case BigSmorph::NATURAL_MINOR:
            return "Nat. Min.";
        case BigSmorph::PENTATONIC:
            return "Pentatonic";
        case BigSmorph::PHRYGIAN:
            return "Phrygian";
        case BigSmorph::TURKISH:
            return "Turkish";
        case BigSmorph::NONE:
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
        drawMessage(vg, Vec(0, 15), note, scale);
    }
};

/////////////////////////////////

struct BigSmorphWidget : ModuleWidget
{


  SvgPanel* darkPanel;
  struct PanelThemeItem : MenuItem {
    BigSmorph *module;
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

    BigSmorph *module = dynamic_cast<BigSmorph*>(this->module);
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
    BigSmorphWidget(BigSmorph *module)
    {

        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Light/BigSmorph.svg")));
        if (module) {
          darkPanel = new SvgPanel();
          darkPanel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Dark/BigSmorph.svg")));
          darkPanel->visible = false;
          addChild(darkPanel);
        }

        addChild(createWidget<ScrewBlack>(Vec(15, 0)));
        addChild(createWidget<ScrewBlack>(Vec(box.size.x - 30, 0)));
        addChild(createWidget<ScrewBlack>(Vec(15, 365)));
        addChild(createWidget<ScrewBlack>(Vec(box.size.x - 30, 365)));

        if (module != NULL)
        {
            BigSmorphDisplay *display = createWidget<BigSmorphDisplay>(Vec(10,95));
            display->module = module;
            display->box.pos = Vec(65, 5);
            display->box.size = Vec(65, 15);
            addChild(display);
        }

        int seq = 33;
        int gli = 30;
        int low = 31;

        addParam(createParam<MCKSSS2>(Vec(10,20), module, BigSmorph::RANGE_PARAM));

        for(int i=0;i<8;i++)
        {

            addParam(createParam<MicroBlu>(Vec(10, 50+i*seq), module, BigSmorph::SEQA_PARAM+i));
            addParam(createParam<MicroBlu>(Vec(40, 50 + i * seq), module, BigSmorph::SEQB_PARAM + i));
            addParam(createParam<MicroBlu>(Vec(70, 50 + i * seq), module, BigSmorph::SEQC_PARAM + i));


        
              addParam(createLightParam<LEDLightBezel<OrangeLight>>(Vec(103, 51 + i * seq), module, BigSmorph::GBUTTON_PARAM + i, BigSmorph::STEP_LIGHT + i));
         

        }
            addInput(createInput<PJ301MOrPort>(Vec(130, 47 + 0 * seq), module, BigSmorph::GATE_INPUT + 0));
            addInput(createInput<PJ301MOrPort>(Vec(130, 47 + 1 * seq), module, BigSmorph::GATE_INPUT + 1));
            addInput(createInput<PJ301MOrPort>(Vec(130, 47 + 2 * seq), module, BigSmorph::GATE_INPUT + 2));
            addInput(createInput<PJ301MOrPort>(Vec(130, 47 + 3 * seq), module, BigSmorph::GATE_INPUT + 3));
            addInput(createInput<PJ301MOrPort>(Vec(130, 47 + 4 * seq), module, BigSmorph::GATE_INPUT + 4));
            addInput(createInput<PJ301MOrPort>(Vec(130, 47 + 5 * seq), module, BigSmorph::GATE_INPUT + 5));
            addInput(createInput<PJ301MOrPort>(Vec(130, 47 + 6 * seq), module, BigSmorph::GATE_INPUT + 6));
            addInput(createInput<PJ301MOrPort>(Vec(130, 47 + 7 * seq), module, BigSmorph::GATE_INPUT + 7));

           addOutput(createOutput<PJ301MOPort>(Vec(8 + low * 0, 335), module, BigSmorph::SEQ_OUTPUT + 0));
           addOutput(createOutput<PJ301MOPort>(Vec(8 + low * 1, 335), module, BigSmorph::SEQ_OUTPUT + 1));
           addOutput(createOutput<PJ301MOPort>(Vec(8 + low * 2, 335), module, BigSmorph::SEQ_OUTPUT + 2));

            for (int i = 0; i < 3; i++)
            {

                addParam(createParam<Trim>(Vec(13 + i * gli, 310), module, BigSmorph::GLIDE_PARAM + i));
        }

        addParam(createParam<MicroBlu>(Vec(25,15), module, BigSmorph::ROOT_NOTE_PARAM));
        addParam(createParam<MicroBlu>(Vec(55,15), module, BigSmorph::SCALE_PARAM));

        addInput(createInput<PJ301MCPort>(Vec(100, 306), module, BigSmorph::CV_INPUT));
        addInput(createInput<PJ301MCPort>(Vec(130, 306), module, BigSmorph::REV_INPUT));
        addInput(createInput<PJ301MCPort>(Vec(100, 335), module, BigSmorph::CLK_INPUT));
        addInput(createInput<PJ301MCPort>(Vec(130, 335), module, BigSmorph::RESET_INPUT));
}
void step() override {
  if (module) {
    Widget* panel = getPanel();
    panel->visible = ((((BigSmorph*)module)->panelTheme) == 0);
    darkPanel->visible  = ((((BigSmorph*)module)->panelTheme) == 1);
  }
  Widget::step();
}
};
Model *modelBigSmorph = createModel<BigSmorph, BigSmorphWidget>("BigSmorph");
