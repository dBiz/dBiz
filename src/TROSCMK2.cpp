////////////////////////////////////////////////////////////////////////////
// <Triple Oscillator>
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

#include <iostream>
#include <array>

#include "dsp/math.hpp"
#include "dsp/Waveshaping.hpp"

using std::array;

struct TROSCMK2 : Module
{
	enum ParamIds
	{
		ENUMS(WAVE_SEL_PARAM, 3),
		ENUMS(OCTAVE_PARAM, 3),
		ENUMS(COARSE_PARAM, 3),
		ENUMS(FINE_PARAM, 3),
		ENUMS(FM_PARAM, 3),
		ENUMS(EXP_FM_PARAM, 3),
		ENUMS(FOLD_PARAM, 3),
		ENUMS(FOLD_ATT_PARAM, 3),
		ENUMS(SYMM_PARAM, 3),
		ENUMS(SYMM_ATT_PARAM, 3),
		ENUMS(LEVEL_PARAM, 3),
		ENUMS(WAVE1_PARAM, 3),
		ENUMS(WAVE2_PARAM, 3),
		ENUMS(OSC_LEVEL_PARAM, 3),
		MASTER_PARAM,
		LINK_A_PARAM,
		LINK_B_PARAM,
		NUM_PARAMS

	};
	enum InputIds
	{
		ENUMS(PITCH_INPUT, 3),
		ENUMS(FM_INPUT, 3),
		ENUMS(EXP_FM_INPUT, 3),
		ENUMS(FOLD_CV_INPUT, 3),
		ENUMS(SYMM_CV_INPUT, 3),
		ENUMS(WAVE_MIX_INPUT, 3),
		ENUMS(VOL_INPUT, 3),
		NUM_INPUTS

	};
	enum OutputIds
	{
		ENUMS(OSC_OUTPUT, 3),
		MASTER_OUTPUT,
		NUM_OUTPUTS

	};
	enum LightIds
	{
		NUM_LIGHTS

	};

	float phase[3]={};
	float oldPhase[3]={};
	float square[3]={1,1,1};
	int discont[3]={};
	int oldDiscont[3]={};
	float input [3]={};
	float foldLevel [3]={};
	float symmLevel[3]={};
	float foldedOutput [3]={};
	float output [3]={};

	float freq[3];
	float att_a[3];
	float incr[3];
	float sintri[3];
	float sawsqr[3];
	float out_a[3];
	float saw[3];
	float sqr[3];
	float tri[3];
	float sin[3];
	float osc_a,osc_b,osc_c;
	float linka, linkb, linkc;

	std::array<float, 4> sawBuffer[3];
	std::array<float, 4> sqrBuffer[3];
	std::array<float, 4> triBuffer[3];
	std::array<Wavefolder, 4> folder[3];

	HardClipper clipper[3];
	float clip_out[3];
	HardClipper masterclip;

	float log2sampleFreq = 15.4284f;
	float sampleRate = APP->engine->getSampleRate();

	dsp::SchmittTrigger resetTrigger[3];

	int panelTheme;

	TROSCMK2() {

	config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

	for ( int i=0;i<3;i++)
	{
	configParam(WAVE_SEL_PARAM+i,  0.f, 1.f, 0.f, string::f("Wave %d Mix Level",i+1));
	configParam(OCTAVE_PARAM+i,  4, 12, 8,string::f("Osc%d Octave",i+1));
	configParam(COARSE_PARAM+i,  -7, 7, 0.f, string::f("Osc%d Coarse note",i+1));
	configParam(FINE_PARAM+i,  -0.083333, 0.083333, 0.0, string::f("Osc%d Fine frequency",i+1));
	configParam(FM_PARAM+i,  -11.7, 11.7, 0.0,  string::f("Osc%d Lin Frequency modulation",i+1));
	configParam(EXP_FM_PARAM+i,  -1.0, 1.0, 0.0, string::f("Osc%d Exp Frequency modulation",i+1));
	configParam(FOLD_PARAM + i, 0.9f, 10.f, 0.9f, string::f("Osc%d Folds",i+1));
	configParam(FOLD_ATT_PARAM+i, -1.0f, 1.0f, 0.0f, string::f("Osc%d Folds CV",i+1));
	configParam(SYMM_PARAM + i, -5.0f, 5.0f, 0.0f, string::f("Osc%d Symmetry",i+1));
	configParam(SYMM_ATT_PARAM+i, -1.0f, 1.0f, 0.0f, string::f("Osc%d Folds CV",i+1));
	configParam(LEVEL_PARAM + i, 0.0, 1.0, 0.0, string::f("Osc%d  Amp Level",i+1));
	configParam(WAVE1_PARAM+i,  0.f, 1.f, 0.f, string::f("Osc%d Wave A Level",i+1));
	configParam(WAVE2_PARAM+i,  0.f, 1.f, 0.f, string::f("Osc%d Wave 2A Level",i+1));
	configParam(OSC_LEVEL_PARAM+i,  0, 1.f, 0.f, string::f("Osc%d  Level Att",i+1));
	
	configInput(PITCH_INPUT + i,string::f("Osc%d V/Oct",i+1));
	configInput(FM_INPUT+ i,string::f("Osc%d FM",i+1));
	configInput(EXP_FM_INPUT+ i,string::f("Osc%d Exp FM",i+1));
	configInput(FOLD_CV_INPUT+ i,string::f("Osc%d Fold Cv",i+1));
	configInput(SYMM_CV_INPUT+ i,string::f("Osc%d Symm Cv",i+1));
	configInput(WAVE_MIX_INPUT+ i,string::f("Osc%d WaveMix",i+1));
	configInput(VOL_INPUT+ i,string::f("Osc%d Level",i+1));
	
	configOutput(OSC_OUTPUT+i,string::f("Osc%d ",i+1)); 

	}
	configParam(MASTER_PARAM, 0.0, M_SQRT2, 1.0, "Ch level", " dB", -10, 40);
	configParam(LINK_A_PARAM, 0.f, 1.f, 0.f, "Link A Param");
	configParam(LINK_B_PARAM, 0.f, 1.f, 0.f, "Link B Param");
	
	configOutput(MASTER_OUTPUT,"Master Level");

	// onReset();

	panelTheme = (loadDarkAsDefault() ? 1 : 0);
	}

	void process(const ProcessArgs &args) override;
	void onSampleRateChange() override;
	

	json_t *dataToJson() override
	{
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

};

void TROSCMK2::onSampleRateChange()
{
	log2sampleFreq = log2f(1.0f / APP->engine->getSampleTime()) - 0.00009f;
	sampleRate = APP->engine->getSampleRate();
}

	void TROSCMK2::process(const ProcessArgs &args) 
	{
		linka = inputs[PITCH_INPUT+0].getVoltage();
		linkb = inputs[PITCH_INPUT+1].getVoltage();
		linkc = inputs[PITCH_INPUT+2].getVoltage();
		if (params[LINK_A_PARAM].getValue() == 0)
			linkb = linka;
		if (params[LINK_B_PARAM].getValue() == 0)
			linkc = linkb;


		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j <= 2; ++j)
			{
    	   	 	sawBuffer[i][j] = sawBuffer[i][j + 1];
    	    	sqrBuffer[i][j] = sqrBuffer[i][j + 1];
    	    	triBuffer[i][j] = triBuffer[i][j + 1];
    		}

			 freq[0] = params[OCTAVE_PARAM + 0].getValue() + 0.031360 + 0.083333 * params[COARSE_PARAM + 0].getValue() + params[FINE_PARAM + 0].getValue() + linka;
			 freq[1] = params[OCTAVE_PARAM + 1].getValue() + 0.031360 + 0.083333 * params[COARSE_PARAM + 1].getValue() + params[FINE_PARAM + 1].getValue() + linkb;
			 freq[2] = params[OCTAVE_PARAM + 2].getValue() + 0.031360 + 0.083333 * params[COARSE_PARAM + 2].getValue() + params[FINE_PARAM + 2].getValue() + linkc;

			
			freq[i] += params[EXP_FM_PARAM+i].getValue() * inputs[EXP_FM_INPUT+i].getVoltage();
    		if (freq[i] >= log2sampleFreq)
			{
    		    freq[i] = log2sampleFreq;
    		}
    		freq[i] = powf(2.0f, freq[i]);
    
    		if (inputs[FM_INPUT+i].isConnected()) 
			{
        		freq[i] += params[FM_PARAM+i].getValue() * params[FM_PARAM+i].getValue() * params[FM_PARAM+i].getValue() * inputs[FM_INPUT+i].getVoltage();
        		incr[i] = args.sampleTime * freq[i];
        		if (incr[i] > 1.0f) 
				{
        		    incr[i] = 1.0f;
        		}
        		else if (incr[i] < -1.0f) 
				{
        		    incr[i] = -1.0f;
        		}
    		}
    		else {
    		    incr[i] = args.sampleTime * freq[i];
    		}

    		phase[i] += incr[i];
    		if (phase[i] >= 0.0f && phase[i] < 1.0f) {
    		    discont[i] = 0;
    		}
    		else if (phase[i] >= 1.0f) {
    		    discont[i] = 1;
    		    --phase[i];
    		    square[i] *= -1.0f;
    		}
    		else {
    		    discont[i] = -1;
    		    ++phase[i];
    		    square[i] *= -1.0f;
    		}

    		sawBuffer[i][3] = phase[i];
    		sqrBuffer[i][3] = square[i];
			
    		if (square[i] >= 0.0f) {
    		    triBuffer[i][3] = phase[i];
    		}
    		else {
    		    triBuffer[i][3] = 1.0f - phase[i];
    		}

    
        	if (oldDiscont[i] == 1) {
        	    polyblep4(sawBuffer[i], 1.0f - oldPhase[i] / incr[i], 1.0f);
        	}
        	else if (oldDiscont[i] == -1) {
        	    polyblep4(sawBuffer[i], 1.0f - (oldPhase[i] - 1.0f) / incr[i], -1.0f);
        	}
        	saw[i]=(clamp(10.0f * (sawBuffer[i][0] - 0.5f), -5.0f, 5.0f));
	
	
        	if (discont[i] == 0) {
        	    if (oldDiscont[i] == 1) {
        	        polyblep4(sqrBuffer[i], 1.0f - oldPhase[i] / incr[i], -2.0f * square[i]);
        	    }
        	    else if (oldDiscont[i] == -1) {
        	        polyblep4(sqrBuffer[i], 1.0f - (oldPhase[i] - 1.0f) / incr[i], -2.0f * square[i]);
        	    }
        	}
        	else {
        	    if (oldDiscont[i] == 1) {
        	        polyblep4(sqrBuffer[i], 1.0f - oldPhase[i] / incr[i], 2.0f * square[i]);
        	    }
        	    else if (oldDiscont[i] == -1) {
        	        polyblep4(sqrBuffer[i], 1.0f - (oldPhase[i] - 1.0f) / incr[i], 2.0f * square[i]);
        	    }
        	}
        	sqr[i]=(clamp(4.9999f * sqrBuffer[i][0], -5.0f, 5.0f));
	
	
        	if (discont[i] == 0) {
        	    if (oldDiscont[i] == 1) {
        	        polyblamp4(triBuffer[i], 1.0f - oldPhase[i] / incr[i], 2.0f * square[i] * incr[i]);
        	    }
        	    else if (oldDiscont[i] == -1) {
        	        polyblamp4(triBuffer[i], 1.0f - (oldPhase[i] - 1.0f) / incr[i], 2.0f * square[i] * incr[i]);
        	    }
        	}
        	else {
        	    if (oldDiscont[i] == 1) {
        	        polyblamp4(triBuffer[i], 1.0f - oldPhase[i] / incr[i], -2.0f * square[i] * incr[i]);
        	    }
        	    else if (oldDiscont[i] == -1) {
        	        polyblamp4(triBuffer[i], 1.0f - (oldPhase[i] - 1.0f) / incr[i], -2.0f * square[i] * incr[i]);
        	    }
        	}
        	tri[i]=(clamp(10.0f * (triBuffer[i][0] - 0.5f), -5.0f, 5.0f));

			if (square[i] >= 0.0f)
			{
				sin[i]=(5.0f * sin_01(0.5f * phase[i]));
			}
			else
			{
				sin[i] = (5.0f * sin_01(0.5f * (1.0f - phase[i])));
			}


    	oldPhase[i] = phase[i];
    	oldDiscont[i] = discont[i];


		sintri[i]=crossfade(sin[i],tri[i],params[WAVE1_PARAM+i].getValue());
		sawsqr[i]=crossfade(saw[i],sqr[i],params[WAVE2_PARAM+i].getValue());
		out_a[i]=crossfade(sintri[i],sawsqr[i],params[WAVE_SEL_PARAM+i].getValue());

/////////////////////////////////////////////////////////////////////////////////////
	// Scale input to be within [-1 1]
	input [i] = 0.2*out_a[i];

	// Read fold cv control
	foldLevel [i] = params[FOLD_PARAM+i].value + params[FOLD_ATT_PARAM+i].value*std::abs(inputs[FOLD_CV_INPUT+i].value);
	foldLevel [i] = clamp(foldLevel[i], -10.0f, 10.0f);

	// Read symmetry cv control
	symmLevel[i] = params[SYMM_PARAM+i].value + 0.5f*params[SYMM_ATT_PARAM+i].value*inputs[SYMM_CV_INPUT+i].value;
	symmLevel[i] = clamp(symmLevel[i], -5.0f, 5.0f);

	// Implement wavefolders
	foldedOutput[i] = input[i] * foldLevel[i] + symmLevel[i];
	for (int j=0; j<4; j++) {
		folder[i][j].process(foldedOutput[i]);
		foldedOutput[i] = folder[i][j].getFoldedOutput();
	}

	// Send samples to output
	output[i] = 5.0f * foldedOutput[i];

	clip_out[i]=output[i] * (std::pow(params[LEVEL_PARAM+i].getValue(), 2.f) + (params[OSC_LEVEL_PARAM+i].getValue()* (inputs[VOL_INPUT+i].getVoltage()/10.f)));
	clipper[i].process(clip_out[i]);
	outputs[OSC_OUTPUT + i].setVoltage(clip_out[i]);

	}
	osc_a=outputs[OSC_OUTPUT+0].getVoltage();
	osc_b=outputs[OSC_OUTPUT+1].getVoltage();
	osc_c=outputs[OSC_OUTPUT+2].getVoltage();
	float master = osc_a+osc_b+osc_c;


	float master_out = 0.3*master* std::pow(params[MASTER_PARAM].getValue(),2.f);
	masterclip.process(master_out);
	outputs[MASTER_OUTPUT].setVoltage(master_out);
}


struct TROSCMK2Widget : ModuleWidget {
	
	int lastPanelTheme = -1;
	std::shared_ptr<window::Svg> light_svg;
	std::shared_ptr<window::Svg> dark_svg;
	
	struct PanelThemeItem : MenuItem {
	  TROSCMK2 *module;
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

	  TROSCMK2 *module = dynamic_cast<TROSCMK2*>(this->module);
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
	TROSCMK2Widget(TROSCMK2 *module){
	 setModule(module);
	 // Main panels from Inkscape
 		light_svg = APP->window->loadSvg(asset::plugin(pluginInstance, "res/Light/TROSCMK2.svg"));
		dark_svg = APP->window->loadSvg(asset::plugin(pluginInstance, "res/Dark/TROSCMK2.svg"));
		int panelTheme = isDark(module ? (&(((TROSCMK2*)module)->panelTheme)) : NULL) ? 1 : 0;// need this here since step() not called for module browser
		setPanel(panelTheme == 0 ? light_svg : dark_svg);	

	 addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
	 addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
	 addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
	 addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

	 int space = 170;
	 int js = 30;

	 addParam(createParam<VerboDSSnapKnob>(Vec(41,  20), module, TROSCMK2::OCTAVE_PARAM+0));
	 addParam(createParam<VerboDSSnapKnob>(Vec(41, 150), module, TROSCMK2::OCTAVE_PARAM+1));
	 addParam(createParam<VerboDSSnapKnob>(Vec(41, 280), module, TROSCMK2::OCTAVE_PARAM+2));

	 addParam(createParam<MicroBluSnapKnob>(Vec(79, 20-13), module, TROSCMK2::COARSE_PARAM+0));
	 addParam(createParam<MicroBluSnapKnob>(Vec(79,150-13), module, TROSCMK2::COARSE_PARAM+1));
	 addParam(createParam<MicroBluSnapKnob>(Vec(79,280-13), module, TROSCMK2::COARSE_PARAM+2));

	 addParam(createParam<Trim>(Vec(37, 50+ 20), module, TROSCMK2::FINE_PARAM+0));
	 addParam(createParam<Trim>(Vec(37, 50+150), module, TROSCMK2::FINE_PARAM+1));
	 addParam(createParam<Trim>(Vec(37, 50+280), module, TROSCMK2::FINE_PARAM+2));

	 addParam(createParam<VerboDS>(Vec(121,  20 - 10), module, TROSCMK2::FM_PARAM+0));
	 addParam(createParam<VerboDS>(Vec(121, 150 - 10), module, TROSCMK2::FM_PARAM+1));
	 addParam(createParam<VerboDS>(Vec(121, 280 - 10), module, TROSCMK2::FM_PARAM+2));

     addParam(createParam<VerboDS>(Vec(161,  20 - 10), module, TROSCMK2::EXP_FM_PARAM+0));
	 addParam(createParam<VerboDS>(Vec(161, 150 - 10), module, TROSCMK2::EXP_FM_PARAM+1));
	 addParam(createParam<VerboDS>(Vec(161, 280 - 10), module, TROSCMK2::EXP_FM_PARAM+2));

     addParam(createParam<VerboS>(Vec(76,26+  20), module, TROSCMK2::FOLD_PARAM+0));
	 addParam(createParam<VerboS>(Vec(76,26+ 150), module, TROSCMK2::FOLD_PARAM+1));
	 addParam(createParam<VerboS>(Vec(76,26+ 280), module, TROSCMK2::FOLD_PARAM+2));

	 addParam(createParam<VerboXS>(Vec(114,36+  20), module, TROSCMK2::FOLD_ATT_PARAM+0));
	 addParam(createParam<VerboXS>(Vec(114,36+ 150), module, TROSCMK2::FOLD_ATT_PARAM+1));
	 addParam(createParam<VerboXS>(Vec(114,36+ 280), module, TROSCMK2::FOLD_ATT_PARAM+2));

	 addParam(createParam<VerboS>(Vec(144,26+  20), module, TROSCMK2::SYMM_PARAM+0));
	 addParam(createParam<VerboS>(Vec(144,26+ 150), module, TROSCMK2::SYMM_PARAM+1));
	 addParam(createParam<VerboS>(Vec(144,26+ 280), module, TROSCMK2::SYMM_PARAM+2));

	 addParam(createParam<VerboXS>(Vec(182, 36 +  20), module, TROSCMK2::SYMM_ATT_PARAM + 0));
	 addParam(createParam<VerboXS>(Vec(182, 36 + 150), module, TROSCMK2::SYMM_ATT_PARAM + 1));
	 addParam(createParam<VerboXS>(Vec(182, 36 + 280), module, TROSCMK2::SYMM_ATT_PARAM + 2));

	 addParam(createParam<VerboDS>(Vec(213, 40 +  25), module, TROSCMK2::LEVEL_PARAM+0));
	 addParam(createParam<VerboDS>(Vec(213, 40 + 155), module, TROSCMK2::LEVEL_PARAM+1));
	 addParam(createParam<VerboDS>(Vec(213, 40 + 280), module, TROSCMK2::LEVEL_PARAM+2));

	 addParam(createParam<MicroBlu>(Vec(33 + space, 15 + 20), module, TROSCMK2::WAVE1_PARAM+0));
	 addParam(createParam<MicroBlu>(Vec(63 + space, 15 + 20), module, TROSCMK2::WAVE2_PARAM+0));
	 addParam(createParam<MicroBlu>(Vec(33 + space, 15 + 150), module, TROSCMK2::WAVE1_PARAM+1));
	 addParam(createParam<MicroBlu>(Vec(63 + space, 15 + 150), module, TROSCMK2::WAVE2_PARAM+1));
	 addParam(createParam<MicroBlu>(Vec(33 + space, 15 + 275), module, TROSCMK2::WAVE1_PARAM+2));
	 addParam(createParam<MicroBlu>(Vec(63 + space, 15 + 275), module, TROSCMK2::WAVE2_PARAM+2));
	 

	 addParam(createParam<Trim>(Vec( 50 + space, 20  - 5), module, TROSCMK2::WAVE_SEL_PARAM+0));
	 addParam(createParam<Trim>(Vec( 50 + space, 150 - 5), module, TROSCMK2::WAVE_SEL_PARAM+1));
	 addParam(createParam<Trim>(Vec( 50 + space, 275 - 5), module, TROSCMK2::WAVE_SEL_PARAM+2));

	  addInput(createInput<PJ301MSPort>(Vec(77 + space, 20 -  18), module, TROSCMK2::WAVE_MIX_INPUT+0));
	  addInput(createInput<PJ301MSPort>(Vec(77 + space, 150 - 18), module, TROSCMK2::WAVE_MIX_INPUT+1));
	  addInput(createInput<PJ301MSPort>(Vec(77 + space, 280 - 18), module, TROSCMK2::WAVE_MIX_INPUT+2));

	 addInput(createInput<PJ301MSPort>(Vec(9, 30 +  20), module, TROSCMK2::PITCH_INPUT+0));
	 addInput(createInput<PJ301MSPort>(Vec(9, 30 + 150), module, TROSCMK2::PITCH_INPUT+1));
	 addInput(createInput<PJ301MSPort>(Vec(9, 30 + 280), module, TROSCMK2::PITCH_INPUT+2));

	 addInput(createInput<PJ301MSPort>(Vec(91, 65 +  20), module, TROSCMK2::FOLD_CV_INPUT+0));
	 addInput(createInput<PJ301MSPort>(Vec(91, 65 + 150), module, TROSCMK2::FOLD_CV_INPUT+1));
	 addInput(createInput<PJ301MSPort>(Vec(91, 65 + 280), module, TROSCMK2::FOLD_CV_INPUT+2));

	 addInput(createInput<PJ301MSPort>(Vec(91+js*1, 65 +  20), module, TROSCMK2::FM_INPUT+0));
	 addInput(createInput<PJ301MSPort>(Vec(91+js*1, 65 + 150), module, TROSCMK2::FM_INPUT+1));
	 addInput(createInput<PJ301MSPort>(Vec(91+js*1, 65 + 280), module, TROSCMK2::FM_INPUT+2));

     addInput(createInput<PJ301MSPort>(Vec(91+js*2, 65 +  20), module, TROSCMK2::EXP_FM_INPUT+0));
	 addInput(createInput<PJ301MSPort>(Vec(91+js*2, 65 + 150), module, TROSCMK2::EXP_FM_INPUT+1));
	 addInput(createInput<PJ301MSPort>(Vec(91+js*2, 65 + 280), module, TROSCMK2::EXP_FM_INPUT+2));

	 addInput(createInput<PJ301MSPort>(Vec(91+js*3, 65 +  20), module, TROSCMK2::SYMM_CV_INPUT+0));
	 addInput(createInput<PJ301MSPort>(Vec(91+js*3, 65 + 150), module, TROSCMK2::SYMM_CV_INPUT+1));
	 addInput(createInput<PJ301MSPort>(Vec(91+js*3, 65 + 280), module, TROSCMK2::SYMM_CV_INPUT+2));

	 addInput(createInput<PJ301MSPort>(Vec(252, 58 +  20), module, TROSCMK2::VOL_INPUT+0));
	 addInput(createInput<PJ301MSPort>(Vec(252, 58 + 150), module, TROSCMK2::VOL_INPUT+1));
	 addInput(createInput<PJ301MSPort>(Vec(252, 50 + 280), module, TROSCMK2::VOL_INPUT+2));

	 addParam(createParam<Trim>(Vec( 280, 63 +  20), module, TROSCMK2::OSC_LEVEL_PARAM+0));
	 addParam(createParam<Trim>(Vec( 280, 63 + 150), module, TROSCMK2::OSC_LEVEL_PARAM+1));
	 addParam(createParam<Trim>(Vec( 280, 55 + 280), module, TROSCMK2::OSC_LEVEL_PARAM+2));

	 addParam(createParam<SilverSwitch>(Vec(47, 80 + 20), module, TROSCMK2::LINK_A_PARAM));
	 addParam(createParam<SilverSwitch>(Vec(47, 80 + 150), module, TROSCMK2::LINK_B_PARAM));

	 addOutput(createOutput<PJ301MSPort>(Vec(263, 22 +  20), module, TROSCMK2::OSC_OUTPUT+0));
	 addOutput(createOutput<PJ301MSPort>(Vec(263, 22 + 150), module, TROSCMK2::OSC_OUTPUT+1));
	 addOutput(createOutput<PJ301MSPort>(Vec(263, 22 + 272), module, TROSCMK2::OSC_OUTPUT+2));

	 addOutput(createOutput<PJ301MSPort>(Vec(302, 220), module, TROSCMK2::MASTER_OUTPUT));
	 addParam(createParam<VerboDS>(Vec(297, 180), module, TROSCMK2::MASTER_PARAM));
}
void step() override {
		int panelTheme = isDark(module ? (&(((TROSCMK2*)module)->panelTheme)) : NULL) ? 1 : 0;
		if (lastPanelTheme != panelTheme) {
			lastPanelTheme = panelTheme;
			SvgPanel* panel = (SvgPanel*)getPanel();
			panel->setBackground(panelTheme == 0 ? light_svg : dark_svg);
		}
		Widget::step();
	}
};


// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.

Model *modelTROSCMK2 = createModel<TROSCMK2, TROSCMK2Widget>("TROSCMK2");
