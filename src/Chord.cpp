////////////////////////////////////////////////////////////////////////////
// <Chord - 4 voice chord generator>
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


/////////////////////////////////////////////////
struct Chord : Module {
	enum ParamIds {
      OFFSET_PARAM,
      INVERSION_PARAM,
      VOICING_PARAM,
      OFFSET_AMT_PARAM,
      INVERSION_AMT_PARAM,
      VOICING_AMT_PARAM,
      FLAT_3RD_PARAM,
      FLAT_5TH_PARAM,
      FLAT_7TH_PARAM,
      SUS_2_PARAM,
      SUS_4_PARAM,
      SIX_FOR_5_PARAM,
      ONE_FOR_7_PARAM,
      FLAT_9_PARAM,
      SHARP_9_PARAM,
      SIX_FOR_7_PARAM,
      SHARP_5_PARAM,
      NUM_PARAMS
	};

	enum InputIds {
      INPUT,
      OFFSET_CV_INPUT,
      INVERSION_CV_INPUT,
      VOICING_CV_INPUT,
      FLAT_3RD_INPUT,
      FLAT_5TH_INPUT,
      FLAT_7TH_INPUT,
      SUS_2_INPUT,
      SUS_4_INPUT,
      SIX_FOR_5_INPUT,
      ONE_FOR_7_INPUT,
      FLAT_9_INPUT,
      SHARP_9_INPUT,
      SIX_FOR_7_INPUT,
      SHARP_5_INPUT,
      NUM_INPUTS
	};
	enum OutputIds {
      OUTPUT_1,
      OUTPUT_2,
      OUTPUT_3,
      OUTPUT_4,
      OUTPUT_ROOT,
      OUTPUT_THIRD,
      OUTPUT_FIFTH,
      OUTPUT_SEVENTH,
      NUM_OUTPUTS
	};

  enum LighIds {
      FLAT_3RD_LIGHT,
      FLAT_5TH_LIGHT,
      FLAT_7TH_LIGHT,
      SUS_2_LIGHT,
      SUS_4_LIGHT,
      SIX_FOR_5_LIGHT,
      ONE_FOR_7_LIGHT,
      FLAT_9_LIGHT,
      SHARP_9_LIGHT,
      SIX_FOR_7_LIGHT,
      SHARP_5_LIGHT,
      OUT_1_LIGHT,
      OUT_2_LIGHT,
      OUT_3_LIGHT,
      OUT_4_LIGHT,
      NUM_LIGHTS

  };

dsp::SchmittTrigger FLAT_3RD;
dsp::SchmittTrigger FLAT_5TH;
dsp::SchmittTrigger FLAT_7TH;
dsp::SchmittTrigger SUS_2;
dsp::SchmittTrigger SUS_4;
dsp::SchmittTrigger SIX_FOR_5;
dsp::SchmittTrigger ONE_FOR_7;
dsp::SchmittTrigger FLAT_9;
dsp::SchmittTrigger SHARP_9;
dsp::SchmittTrigger SIX_FOR_7;
dsp::SchmittTrigger SHARP_5;

bool f3 = false;
bool f5 = false;
bool f7 = false;
bool s2 = false;
bool s4 = false;
bool sff = false;
bool ofs = false;
bool f9 = false;
bool s9 = false;
bool sfs = false;
bool s5 = false;


int panelTheme;

	Chord() {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

    configParam(OFFSET_PARAM, 0.0, 1.0, 0.5,"Offset");
    configParam(INVERSION_PARAM, 0.0, 1.0, 0.0,"Voicing");
    configParam(VOICING_PARAM, 0.0, 1.0, 0.0,"Inversion");
    configParam(OFFSET_AMT_PARAM, 0.0, 1.0, 0.5,"Offset Amt");
    configParam(INVERSION_AMT_PARAM, 0.0, 1.0, 0.0,"Inversion Amt");
    configParam(VOICING_AMT_PARAM, 0.0, 1.0, 0.0,"Voicing Amt");
    configParam(FLAT_3RD_PARAM, 0.0,1.0,0.0,"b3");
    configParam(FLAT_5TH_PARAM, 0.0,1.0,0.0,"b5");
    configParam(FLAT_7TH_PARAM, 0.0,1.0,0.0,"b7");
    configParam(SUS_2_PARAM, 0.0,1.0,0.0,"sus2");
    configParam(SUS_4_PARAM, 0.0,1.0,0.0,"sus4");
    configParam(SIX_FOR_5_PARAM, 0.0,1.0,0.0,"add6");
    configParam(ONE_FOR_7_PARAM, 0.0,1.0,0.0,"1for7");
    configParam(FLAT_9_PARAM, 0.0,1.0,0.0,"b9");
    configParam(SHARP_9_PARAM, 0.0,1.0,0.0,"#9");
    configParam(SIX_FOR_7_PARAM, 0.0,1.0,0.0,"bb7");
    configParam(SHARP_5_PARAM, 0.0,1.0,0.0,"#5");
	
	  configInput(INPUT,"In");
    configInput(OFFSET_CV_INPUT,"Offset Cv");
    configInput(INVERSION_CV_INPUT,"Inversion Cv");
    configInput(VOICING_CV_INPUT,"Voicing Cv");
    configInput(FLAT_3RD_INPUT,"b3");
    configInput(FLAT_5TH_INPUT,"b5");
    configInput(FLAT_7TH_INPUT,"b7");
    configInput(SUS_2_INPUT,"sus2");
    configInput(SUS_4_INPUT,"sus4");
    configInput(SIX_FOR_5_INPUT,"add6");
    configInput(ONE_FOR_7_INPUT,"for7");
    configInput(FLAT_9_INPUT,"b9");
    configInput(SHARP_9_INPUT,"#9");
    configInput(SIX_FOR_7_INPUT,"bb7");
    configInput(SHARP_5_INPUT,"#5");
	
	  configOutput(OUTPUT_1,"Voice_1");
    configOutput(OUTPUT_2,"Voice_2");
    configOutput(OUTPUT_3,"Voice_3");
    configOutput(OUTPUT_4,"Voice_4");
    configOutput(OUTPUT_ROOT,"Out Root");
    configOutput(OUTPUT_THIRD,"Out III");
    configOutput(OUTPUT_FIFTH,"Out V");
    configOutput(OUTPUT_SEVENTH,"Out VII");	
	
	
	  // onReset();

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

	void process(const ProcessArgs &args) override
  {

  float in = inputs[INPUT].getVoltage();
  int octave = round(in);

  float offset_raw = (params[OFFSET_PARAM].getValue()) * 12 - 6 + (inputs[OFFSET_CV_INPUT].getVoltage()*params[OFFSET_AMT_PARAM].getValue()) / 1.5;
  float pitch_offset = round(offset_raw) / 12;

  float root = in - 1.0*octave + pitch_offset;
  float root_or_2nd = root;

  float inversion_raw = (params[INVERSION_PARAM].getValue()) * 4 - 1 + ((inputs[INVERSION_CV_INPUT].getVoltage()*params[INVERSION_AMT_PARAM].getValue()) / 3);
  int inversion = round(inversion_raw);
  if (inversion > 2) inversion = 2;
  if (inversion < -1) inversion = -1;

  float voicing_raw = (params[VOICING_PARAM].getValue()) * 5 - 2 + ((inputs[VOICING_CV_INPUT].getVoltage()*params[VOICING_AMT_PARAM].getValue()) / 3);
  int voicing = round(voicing_raw);
  if (voicing > 2) voicing = 2;
  if (voicing < -2) voicing = -2;


  float voice_1 = 0.0;
  float voice_2 = 0.0;
  float voice_3 = 0.0;
  float voice_4 = 0.0;

  int third = 4;
  int fifth = 7;
  int seventh = 11;

  if (FLAT_3RD.process(params[FLAT_3RD_PARAM].getValue()+inputs[FLAT_3RD_INPUT].getVoltage())) f3=!f3;
  lights[FLAT_3RD_LIGHT].setBrightness(f3);
  if (f3) third = 3;

  if (FLAT_5TH.process(params[FLAT_5TH_PARAM].getValue()+inputs[FLAT_5TH_INPUT].getVoltage())) f5=!f5;
  lights[FLAT_5TH_LIGHT].setBrightness(f5);
  if(f5) fifth = 6;
  
  

  if (SHARP_5.process(params[SHARP_5_PARAM].getValue()+inputs[SHARP_5_INPUT].getVoltage())) s5=!s5;
  lights[SHARP_5_LIGHT].setBrightness(s5);
  if(s5) fifth = 8;
  
  

  if (FLAT_7TH.process(inputs[FLAT_7TH_INPUT].getVoltage()+params[FLAT_7TH_PARAM].getValue())) f7=!f7;
  lights[FLAT_7TH_LIGHT].setBrightness(f7);
  if(f7) seventh = 10;
  
  

  if (SUS_2.process(inputs[SUS_2_INPUT].getVoltage()+params[SUS_2_PARAM].getValue())) s2=!s2;
  lights[SUS_2_LIGHT].setBrightness(s2);
  if(s2) root_or_2nd = root + (2 * (1.0/12.0));
  
  

  if (SUS_4.process(inputs[SUS_4_INPUT].getVoltage()+params[SUS_4_PARAM].getValue())) s4=!s4;
  lights[SUS_4_LIGHT].setBrightness(s4);
  if(s4) third = 5;
  
  

  if (SIX_FOR_5.process(inputs[SIX_FOR_5_INPUT].getVoltage()+params[SIX_FOR_5_PARAM].getValue())) sff=!sff;
  lights[SIX_FOR_5_LIGHT].setBrightness(sff);
  if(sff) fifth = 9;
  
  

  if (SIX_FOR_7.process(inputs[SIX_FOR_7_INPUT].getVoltage()+params[SIX_FOR_7_PARAM].getValue())) sfs=!sfs;
  lights[SIX_FOR_7_LIGHT].setBrightness(sfs);
  if(sfs) seventh = 9;
  
  


  if (FLAT_9.process(inputs[FLAT_9_INPUT].getVoltage()+params[FLAT_9_PARAM].getValue())) f9=!f9;
  lights[FLAT_9_LIGHT].setBrightness(f9);
  if(f9) root_or_2nd = root + 1.0/12.0;
  
  

  if (SHARP_9.process(inputs[SHARP_9_INPUT].getVoltage()+params[SHARP_9_PARAM].getValue())) s9=!s9;
  lights[SHARP_9_LIGHT].setBrightness(s9);
  if(s9) root_or_2nd = root + (3 * (1.0/12.0));
  
  

  if (ONE_FOR_7.process(inputs[ONE_FOR_7_INPUT].getVoltage()+params[ONE_FOR_7_PARAM].getValue())) ofs=!ofs;
  lights[ONE_FOR_7_LIGHT].setBrightness(ofs);
  if(ofs) seventh = 12;
  
  

  outputs[OUTPUT_ROOT].setVoltage(root);
  outputs[OUTPUT_THIRD].setVoltage(root + third * (1.0/12.0));
  outputs[OUTPUT_FIFTH].setVoltage(root + fifth * (1.0/12.0));
  outputs[OUTPUT_SEVENTH].setVoltage(root + seventh * (1.0/12.0));



  if (inversion == -1 )
  {
    voice_1 = root_or_2nd;
    voice_2 = root + third * (1.0/12.0);
    voice_3 = root + fifth * (1.0/12.0);
    voice_4 = root + seventh * (1.0/12.0);
  }
  if (inversion == 0 )
  {
    voice_1 = root + third * (1.0/12.0);
    voice_2 = root + fifth * (1.0/12.0);
    voice_3 = root + seventh * (1.0/12.0);
    voice_4 = root_or_2nd + 1.0;
  }
  if (inversion == 1)
  {
    voice_1 = root + fifth * (1.0/12.0);
    voice_2 = root + seventh * (1.0/12.0);
    voice_3 = root_or_2nd + 1.0;
    voice_4 = root + 1.0 + third * (1.0/12.0);
  }
  if (inversion == 2 )
  {
    voice_1 = root + seventh * (1.0/12.0);
    voice_2 = root_or_2nd + 1.0;
    voice_3 = root + 1.0 + third * (1.0/12.0);
    voice_4 = root + 1.0 + fifth * (1.0/12.0);
  }

  if (voicing == -1) voice_2 -= 1.0;
  if (voicing == -0) voice_3 -= 1.0;
  if (voicing == 1)
  {
    voice_2 -= 1.0;
    voice_4 -= 1.0;
  }
  if (voicing == 2)
  {
    voice_2 += 1.0;
    voice_4 += 1.0;
  }


  outputs[OUTPUT_1].setVoltage(voice_1);
  outputs[OUTPUT_2].setVoltage(voice_2);
  outputs[OUTPUT_3].setVoltage(voice_3);
  outputs[OUTPUT_4].setVoltage(voice_4);
}
};

struct ChordWidget : ModuleWidget
{

    int lastPanelTheme = -1;
	std::shared_ptr<window::Svg> light_svg;
	std::shared_ptr<window::Svg> dark_svg;
	
	struct PanelThemeItem : MenuItem {
	  Chord *module;
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

	  Chord *module = dynamic_cast<Chord*>(this->module);
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

ChordWidget(Chord *module){
  setModule(module);
  // Main panels from Inkscape
 		light_svg = APP->window->loadSvg(asset::plugin(pluginInstance, "res/Light/Chord.svg"));
		dark_svg = APP->window->loadSvg(asset::plugin(pluginInstance, "res/Dark/Chord.svg"));
		int panelTheme = isDark(module ? (&(((Chord*)module)->panelTheme)) : NULL) ? 1 : 0;// need this here since step() not called for module browser
		setPanel(panelTheme == 0 ? light_svg : dark_svg);	


  //
  addChild(createWidget<ScrewBlack>(Vec(15, 0)));
  addChild(createWidget<ScrewBlack>(Vec(box.size.x-30, 0)));
  addChild(createWidget<ScrewBlack>(Vec(15, 365)));
  addChild(createWidget<ScrewBlack>(Vec(box.size.x-30, 365)));
//
int jacks = 27;
int pot=22;
int off =2.5;
int space = 40;

  addInput(createInput<PJ301MSPort>(Vec(off,60+jacks*1), module, Chord::OFFSET_CV_INPUT));
  addInput(createInput<PJ301MSPort>(Vec(off,60+jacks*2), module, Chord::INVERSION_CV_INPUT));
  addInput(createInput<PJ301MSPort>(Vec(off,60+jacks*3), module, Chord::VOICING_CV_INPUT));

  addParam(createParam<Trimpot>(Vec(off*2,pot*1), module, Chord::OFFSET_AMT_PARAM));
  addParam(createParam<Trimpot>(Vec(off*2,pot*2), module, Chord::INVERSION_AMT_PARAM));
  addParam(createParam<Trimpot>(Vec(off*2,pot*3), module, Chord::VOICING_AMT_PARAM));



  addParam(createParam<FlatA>(Vec(off + 30 ,space*1-15), module, Chord::OFFSET_PARAM));
  addParam(createParam<FlatA>(Vec(off + 30 ,space*2-15), module, Chord::INVERSION_PARAM));
  addParam(createParam<FlatA>(Vec(off + 30 ,space*3-15), module, Chord::VOICING_PARAM));

//

int right = 95 ;
int left = 30;

  addInput(createInput<PJ301MSPort>(Vec(left, 180), module, Chord::FLAT_3RD_INPUT));
  addInput(createInput<PJ301MSPort>(Vec(left, 180+jacks*1), module, Chord::FLAT_5TH_INPUT));
  addInput(createInput<PJ301MSPort>(Vec(left, 180+jacks*2), module, Chord::FLAT_7TH_INPUT));
  addInput(createInput<PJ301MSPort>(Vec(left, 180+jacks*3), module, Chord::SIX_FOR_7_INPUT));
  addInput(createInput<PJ301MSPort>(Vec(left, 180+jacks*4), module, Chord::SHARP_5_INPUT));

  //
  addInput(createInput<PJ301MSPort>(Vec(right, 180), module, Chord::SUS_2_INPUT));
  addInput(createInput<PJ301MSPort>(Vec(right, 180+jacks*1), module, Chord::SUS_4_INPUT));
  addInput(createInput<PJ301MSPort>(Vec(right, 180+jacks*2), module, Chord::SIX_FOR_5_INPUT));
  addInput(createInput<PJ301MSPort>(Vec(right, 180+jacks*3), module, Chord::ONE_FOR_7_INPUT));
  addInput(createInput<PJ301MSPort>(Vec(right, 180+jacks*4), module, Chord::FLAT_9_INPUT));
  addInput(createInput<PJ301MSPort>(Vec(right, 180+jacks*5), module, Chord::SHARP_9_INPUT));
//


  addParam(createLightParam<LEDLightBezel<OrangeLight>>(Vec(-22 + left, 3+ 180), module, Chord::FLAT_3RD_PARAM, Chord::FLAT_3RD_LIGHT));
  addParam(createLightParam<LEDLightBezel<OrangeLight>>(Vec(-22 + left, 3+ 180+jacks*1), module, Chord::FLAT_5TH_PARAM, Chord::FLAT_5TH_LIGHT));
  addParam(createLightParam<LEDLightBezel<OrangeLight>>(Vec(-22 + left, 3+ 180+jacks*2), module, Chord::FLAT_7TH_PARAM, Chord::FLAT_7TH_LIGHT));
  addParam(createLightParam<LEDLightBezel<OrangeLight>>(Vec(-22 + left, 3+ 180+jacks*3), module, Chord::SIX_FOR_7_PARAM, Chord::SIX_FOR_7_LIGHT));
  addParam(createLightParam<LEDLightBezel<OrangeLight>>(Vec(-22 + left, 3+ 180+jacks*4), module, Chord::SHARP_5_PARAM, Chord::SHARP_5_LIGHT));
  addParam(createLightParam<LEDLightBezel<OrangeLight>>(Vec(-22 + right,3+  180), module, Chord::SUS_2_PARAM, Chord::SUS_2_LIGHT));
  addParam(createLightParam<LEDLightBezel<OrangeLight>>(Vec(-22 + right,3+  180+jacks*1), module, Chord::SUS_4_PARAM, Chord::SUS_4_LIGHT));
  addParam(createLightParam<LEDLightBezel<OrangeLight>>(Vec(-22 + right,3+  180+jacks*2), module, Chord::SIX_FOR_5_PARAM, Chord::SIX_FOR_5_LIGHT));
  addParam(createLightParam<LEDLightBezel<OrangeLight>>(Vec(-22 + right,3+  180+jacks*3), module, Chord::ONE_FOR_7_PARAM, Chord::ONE_FOR_7_LIGHT));
  addParam(createLightParam<LEDLightBezel<OrangeLight>>(Vec(-22 + right,3+  180+jacks*4), module, Chord::FLAT_9_PARAM, Chord::FLAT_9_LIGHT));
  addParam(createLightParam<LEDLightBezel<OrangeLight>>(Vec(-22 + right,3+  180+jacks*5), module, Chord::SHARP_9_PARAM, Chord::SHARP_9_LIGHT));

  //

  //

  addOutput(createOutput<PJ301MSPort>(Vec(70,jacks*1), module, Chord::OUTPUT_ROOT));
  addOutput(createOutput<PJ301MSPort>(Vec(70,jacks*2), module, Chord::OUTPUT_THIRD));
  addOutput(createOutput<PJ301MSPort>(Vec(70,jacks*3), module, Chord::OUTPUT_FIFTH));
  addOutput(createOutput<PJ301MSPort>(Vec(70,jacks*4), module, Chord::OUTPUT_SEVENTH));

  addOutput(createOutput<PJ301MSPort>(Vec(97,jacks*1 ), module, Chord::OUTPUT_1));
  addOutput(createOutput<PJ301MSPort>(Vec(97,jacks*2 ), module, Chord::OUTPUT_2));
  addOutput(createOutput<PJ301MSPort>(Vec(97,jacks*3 ), module, Chord::OUTPUT_3));
  addOutput(createOutput<PJ301MSPort>(Vec(97,jacks*4 ), module, Chord::OUTPUT_4));

  addInput(createInput<PJ301MSPort>(Vec(97,57+jacks*3), module, Chord::INPUT));

}
void step() override {
		int panelTheme = isDark(module ? (&(((Chord*)module)->panelTheme)) : NULL) ? 1 : 0;
		if (lastPanelTheme != panelTheme) {
			lastPanelTheme = panelTheme;
			SvgPanel* panel = (SvgPanel*)getPanel();
			panel->setBackground(panelTheme == 0 ? light_svg : dark_svg);
		}
		Widget::step();
	}
};
Model *modelChord = createModel<Chord, ChordWidget>("Chord");
