////////////////////////////////////////////////////////////////////////////
// <Strange and experimental quad filter>
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
#include "dsp/resampler.hpp"

using namespace std;

#define pi 3.14159265359


struct MultiFilter
{
  float q;
  float freq;
  float smpRate;
  float hp = 0.0f, bp = 0.0f, lp = 0.0f,sp = 0.0f, mem1 = 0.0f, mem2 = 0.0f;

  void setParams(float freq, float q, float smpRate)
  {
    this->freq = freq;
    this->q = q;
    this->smpRate = smpRate;
  }

  void calcOutput(float sample)
  {
    float g = tan(pi * freq / smpRate);
    float R = 1.0f / (2.0f * q);
    hp = (sample - (2.0f * R + g) * mem1 - mem2) / (1.0f + 2.0f * R * g + g * g);
    bp = g * hp + mem1;
    lp = g * bp + mem2;
    //sp = g*lp + g*hp ;
    mem1 = g * hp + bp;
    mem2 = g * bp + lp;
  }
};

struct VarOut
{

  float value = 0.f;

  float getVar()
  {
    return value;
  }

  /** Emulates light decay with slow fall but immediate rise. */
  void setSmoothVar(float var, float deltaTime)
  {
    if (var < value)
    {
      // Fade out light
      const float lambda = 8.f;
      value += (var - value) * lambda * deltaTime;
    }
    else
    {
      // Immediately illuminate light
      value = var;
    }
  }

};

/////added fine out /////////////////////////////////////////////////

struct QuePasa : Module {
    enum ParamIds
    {
        FREQ_PARAM,
        VCA_PARAM,
        FREQ_CV_PARAM,
        RES_PARAM,
        RES_CV_PARAM,
        RAD_L_PARAM,
        RAD_R_PARAM,
        RAD_L_CV_PARAM,
        RAD_R_CV_PARAM,

        NUM_PARAMS
    };
    enum InputIds
    {
        L_INPUT,
        R_INPUT,
        VCA_INPUT,
        RAD_L_INPUT,
        RAD_R_INPUT,
        VAR_L_INPUT,
        VAR_R_INPUT,
        FREQ1_INPUT,
        FREQ2_INPUT,
        RES_INPUT,
        NUM_INPUTS
    };
    enum OutputIds
    {

        LP_L_OUTPUT,
        LP_R_OUTPUT,
        HP_L_OUTPUT,
        HP_R_OUTPUT,
        BP_L_OUTPUT,
        BP_R_OUTPUT,
     
        NUM_OUTPUTS
    };

    enum LighIds {

        NUM_LIGHTS
    };

    MultiFilter filterL[3]={};
    MultiFilter filterR[3] = {};
    dsp::SchmittTrigger VarTrig[2];
    VarOut var[2]={};
    bool gateVar[2]={};

    int panelTheme;

    QuePasa()
    {
      config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
      configParam(FREQ_PARAM, 0.f, 1.f, 1.f, "Center Freq.", " Hz", std::pow(2, 10.f), dsp::FREQ_C4 / std::pow(2, 5.f));
      configParam(RES_PARAM, .1f, 1.f, .1f, "Q factor", "", 0.f, 100.f);
      configParam(FREQ_CV_PARAM, -1.f, 1.f, 0.f, "Freq. Mod", "%", 0.f, 100.f);
      configParam(RES_CV_PARAM, -1.f, 1.f, 0.f, "Res. Mod", "%", 0.f, 100.f);
      configParam(VCA_PARAM, 0.f, 1.f, 0.f, "Vca Level", "%", 0.f, 100.f);
      configParam(RAD_L_PARAM, 0.f, 1.f, 0.f, "Space L", "%", std::pow(2, 10.f), dsp::FREQ_C4 / std::pow(2, 5.f));
      configParam(RAD_R_PARAM, 0.f, 1.f, 0.f, "Space R", "%", std::pow(2, 10.f), dsp::FREQ_C4 / std::pow(2, 5.f));
      configParam(RAD_L_CV_PARAM, -1.f, 1.f, 0.f, "Space L Modulation", "%", 0.f, 100.f);
      configParam(RAD_R_CV_PARAM, -1.f, 1.f, 0.f, "Space R Modulation", "%", 0.f, 100.f);
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


  void process(const ProcessArgs &args) override
  {

//////////////////////////////////////////////////////////////////////////

if (VarTrig[0].process(inputs[VAR_L_INPUT].getVoltage()))
{
  gateVar[0] = true;
}
else
  gateVar[0] = false;

var[0].setSmoothVar(gateVar[0] ? 5.0 : 0.0, args.sampleTime);

if (VarTrig[1].process(inputs[VAR_R_INPUT].getVoltage()))
{
  gateVar[1] = true;
}
else
  gateVar[1] = false;

var[1].setSmoothVar(gateVar[1] ? 5.0 : 0.0, args.sampleTime);

/////////////////////////////////////////////

float fcv = params[FREQ_CV_PARAM].getValue();
fcv=dsp::quadraticBipolar(fcv);
if (inputs[FREQ2_INPUT].isConnected())
{
fcv *= inputs[FREQ2_INPUT].getVoltage();
}
float cfreq = params[FREQ_PARAM].getValue();
cfreq = cfreq * 10.f - 5.f;
float pitch = cfreq + inputs[FREQ1_INPUT].getVoltage() * fcv;
float cutoff = dsp::FREQ_C4 * std::pow(2.f, pitch);
cutoff = clamp(cutoff, 1.f, 8000.f);
float q = 10.0f * clamp(params[RES_PARAM].getValue() + params[RES_CV_PARAM].getValue() * inputs[RES_INPUT].getVoltage() * 0.2f, 0.1f, 1.0f);


float fcvL = params[RAD_L_CV_PARAM].getValue();
fcvL=dsp::quadraticBipolar(fcvL);
float RfreqL = params[RAD_L_PARAM].getValue();
RfreqL = RfreqL *10.f-5.f;
float pitchL = RfreqL +var[0].getVar()+inputs[RAD_L_INPUT].getVoltage() * fcvL;
float cutoffL = dsp::FREQ_C4 * std::pow(2.f, pitchL);
cutoffL = clamp(cutoffL, 1.f, 8000.f);

float fcvR = params[RAD_R_CV_PARAM].getValue();
fcvR=dsp::quadraticBipolar(fcvR);
float RfreqR = params[RAD_R_PARAM].getValue();
RfreqR = RfreqR *10.f-5.f;
float pitchR = RfreqR + var[1].getVar()+ inputs[RAD_R_INPUT].getVoltage() * fcvR;
float cutoffR = dsp::FREQ_C4 * std::pow(2.f, pitchR);
cutoffR = clamp(cutoffR, 1.f, 8000.f);

for (int i = 0; i < 3; i++)
{
  filterL[i].setParams(cutoff, q, args.sampleRate);
  filterR[i].setParams(cutoff, q, args.sampleRate);
  if (i == 0 || i == 2)
  {
    filterL[i].freq -= cutoffL / 3;
    filterR[i].freq -= cutoffR / 3;
  }
  if (i == 1 || i == 3)
  {
    filterL[i].freq += cutoffL;
    filterR[i].freq += cutoffR;
  }

   if (filterL[i].freq < std::pow(2.0f, 4.5f))
    filterL[i].freq = std::pow(2.0f, 4.5f);
  if (filterR[i].freq < std::pow(2.0f, 4.5f))
    filterR[i].freq = std::pow(2.0f, 4.5f);

  if (filterL[i].freq > std::pow(2.0f, 14.0f))
    filterL[i].freq = std::pow(2.0f, 14.0f);
  if (filterR[i].freq > std::pow(2.0f, 14.0f))
    filterR[i].freq = std::pow(2.0f, 14.0f);

 }

     float inl = inputs[L_INPUT].getVoltage() * std::pow(params[VCA_PARAM].getValue(), 2.f)*0.2f;
     float inr = inputs[R_INPUT].getVoltage() * std::pow(params[VCA_PARAM].getValue(), 2.f)*0.2f;


     // //filtering
     for (int i = 0; i < 3; i++)
     {
       filterL[i].calcOutput(inl);
       filterR[i].calcOutput(inr);
     }
     float LPL=0.f;
     float LPR=0.f;
     float HPL=0.f;
     float HPR=0.f;
     float BPL=0.f;
     float BPR=0.f;


     for (int i = 0; i < 3; i++)
     {
       LPL+=filterL[i].lp*3.0f;
       LPR+=filterR[i].lp*3.0f;
       HPL+=filterL[i].hp*3.0f;
       HPR+=filterR[i].hp*3.0f;
       BPL+=filterL[i].bp*3.0f;
       BPR+=filterR[i].bp*3.0f;

     }

     outputs[LP_L_OUTPUT].setVoltage(LPL);
     outputs[LP_R_OUTPUT].setVoltage(LPR);
     outputs[HP_L_OUTPUT].setVoltage(HPL);
     outputs[HP_R_OUTPUT].setVoltage(HPR);
     outputs[BP_L_OUTPUT].setVoltage(BPL);
     outputs[BP_R_OUTPUT].setVoltage(BPR);

  }
};

//////////////////////////////////////////////////////////////////
struct QuePasaWidget : ModuleWidget
{


    int lastPanelTheme = -1;
	std::shared_ptr<window::Svg> light_svg;
	std::shared_ptr<window::Svg> dark_svg;
	
  struct PanelThemeItem : MenuItem {
    QuePasa *module;
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

    QuePasa *module = dynamic_cast<QuePasa*>(this->module);
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
QuePasaWidget(QuePasa *module){
  setModule(module);
  // Main panels from Inkscape
 		light_svg = APP->window->loadSvg(asset::plugin(pluginInstance, "res/Light/QuePasa.svg"));
		dark_svg = APP->window->loadSvg(asset::plugin(pluginInstance, "res/Dark/QuePasa.svg"));
		int panelTheme = isDark(module ? (&(((QuePasa*)module)->panelTheme)) : NULL) ? 1 : 0;// need this here since step() not called for module browser
		setPanel(panelTheme == 0 ? light_svg : dark_svg);	

  int kn=41;
  int jk=30;

  addParam(createParam<VerboDS>(Vec(10, 60), module, QuePasa::VCA_PARAM));

  addParam(createParam<HRoundWhy>(Vec((box.size.x / 2) - 25, 90), module, QuePasa::FREQ_PARAM));

  addParam(createParam<VerboDS>(Vec(10+2*kn, 180), module, QuePasa::RES_PARAM));
  addParam(createParam<VerboDS>(Vec(10 + 2 * kn, 180 + kn), module, QuePasa::RES_CV_PARAM));
  addParam(createParam<VerboDS>(Vec(10+kn, 180), module, QuePasa::FREQ_CV_PARAM));
  addParam(createParam<VerboRS>(Vec(10, 180), module, QuePasa::RAD_L_PARAM));
  addParam(createParam<VerboRS>(Vec(10 + 3 * kn, 180), module, QuePasa::RAD_R_PARAM));
  addParam(createParam<VerboS>(Vec(10, 180+kn), module, QuePasa::RAD_L_CV_PARAM));
  addParam(createParam<VerboS>(Vec(10+3*kn, 180+kn), module, QuePasa::RAD_R_CV_PARAM));

  ///Innies
  addInput(createInput<PJ301MIPort>(Vec(10, 20), module, QuePasa::L_INPUT));
  addInput(createInput<PJ301MIPort>(Vec(10+jk, 20), module, QuePasa::R_INPUT));

  addInput(createInput<PJ301MCPort>(Vec(15 , 110), module, QuePasa::VCA_INPUT));

  addInput(createInput<PJ301MCPort>(Vec(15+kn, 280), module, QuePasa::FREQ1_INPUT));
  addInput(createInput<PJ301MCPort>(Vec(15 + kn, 280 + jk), module, QuePasa::FREQ2_INPUT));

  addInput(createInput<PJ301MCPort>(Vec(15, 280), module, QuePasa::RAD_L_INPUT));
  addInput(createInput<PJ301MCPort>(Vec(15+3*kn, 280), module, QuePasa::RAD_R_INPUT));
  addInput(createInput<PJ301MBPort>(Vec(15, 280 + jk), module, QuePasa::VAR_L_INPUT));
  addInput(createInput<PJ301MBPort>(Vec(15 + 3 * kn, 280 + jk), module, QuePasa::VAR_R_INPUT));

  addInput(createInput<PJ301MCPort>(Vec(15+2*kn, 280), module, QuePasa::RES_INPUT));

  ///Outies
  addOutput(createOutput<PJ301MOPort>(Vec(box.size.x-60, 20), module, QuePasa::LP_L_OUTPUT));
  addOutput(createOutput<PJ301MOPort>(Vec(box.size.x-35, 20), module, QuePasa::LP_R_OUTPUT));
  addOutput(createOutput<PJ301MOPort>(Vec(box.size.x-60, 20+jk), module, QuePasa::BP_L_OUTPUT));
  addOutput(createOutput<PJ301MOPort>(Vec(box.size.x-35, 20+jk), module, QuePasa::BP_R_OUTPUT));
  addOutput(createOutput<PJ301MOPort>(Vec(box.size.x-60, 20+2*jk), module, QuePasa::HP_L_OUTPUT));
  addOutput(createOutput<PJ301MOPort>(Vec(box.size.x-35, 20+2*jk), module, QuePasa::HP_R_OUTPUT));



  //Screw
  addChild(createWidget<ScrewBlack>(Vec(15, 0)));
  addChild(createWidget<ScrewBlack>(Vec(box.size.x - 30, 0)));
  addChild(createWidget<ScrewBlack>(Vec(15, 365)));
  addChild(createWidget<ScrewBlack>(Vec(box.size.x - 30, 365)));


}

void step() override {
		int panelTheme = isDark(module ? (&(((QuePasa*)module)->panelTheme)) : NULL) ? 1 : 0;
		if (lastPanelTheme != panelTheme) {
			lastPanelTheme = panelTheme;
			SvgPanel* panel = (SvgPanel*)getPanel();
			panel->setBackground(panelTheme == 0 ? light_svg : dark_svg);
		}
		Widget::step();
	}
};
Model *modelQuePasa = createModel<QuePasa, QuePasaWidget>("QuePasa");
