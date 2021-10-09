////////////////////////////////////////////////////////////////////////////
// <Sual Filter - dual multimilter ;) >
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
	float hp = 0.0f,bp = 0.0f,lp = 0.0f,mem1 = 0.0f,mem2 = 0.0f;

	void setParams(float freq, float q, float smpRate) {
		this->freq = freq;
		this->q=q;
		this->smpRate=smpRate;
	}

	void calcOutput(float sample)
	{
		float g = tan(pi*freq/smpRate);
		float R = 1.0f/(2.0f*q);
		hp = (sample - (2.0f*R + g)*mem1 - mem2)/(1.0f + 2.0f*R*g + g*g);
		bp = g*hp + mem1;
		lp = g*bp +  mem2;
		mem1 = g*hp + bp;
		mem2 = g*bp + lp;
	}

};

struct DualFilter : Module{
	enum ParamIds
	{
		CUTOFF_PARAM,
		Q_PARAM,
		CMOD_PARAM,
		CMOD_PARAM2,
		DRIVE_PARAM,
		CUTOFF2_PARAM,
		Q2_PARAM,
		CMOD2_PARAM,
		CMOD2_PARAM2,
		DRIVE2_PARAM,

		FADE_PARAM,

		VOLA_PARAM,
		VOLB_PARAM,

		FILTERSEL_PARAM,
		FILTER2SEL_PARAM,

		NUM_PARAMS
	};
	enum InputIds
	{
		CUTOFF_INPUT,
		CUTOFF_INPUT2,
		Q_INPUT,
		DRIVE_INPUT,
		IN,
		IN2,
		CUTOFF2_INPUT,
		CUTOFF2_INPUT2,
		Q2_INPUT,
		DRIVE2_INPUT,
		FADE_CV,

		NUM_INPUTS
	};
	enum OutputIds
	{
		OUT1,
		OUT2,
		MIXOUT,
		NUM_OUTPUTS
	};

	enum LightIds
	{
	  FADEA_LIGHTS,
	  FADEB_LIGHTS,
	  NUM_LIGHTS
	};


MultiFilter filterA;	// create a lpFilter;
MultiFilter filterB;	// create a lpFilter;

int panelTheme;

DualFilter()
{
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
    configParam(CUTOFF_PARAM, 0.f, 1.f, 1.f, "Center Freq.", " Hz", params[CUTOFF_PARAM].getValue(), 20.f * 410.58f);
    configParam(Q_PARAM, .1f, 1.f, .1f, "Q factor", "", params[Q_PARAM].getValue(), 20.f);
    configParam(CMOD_PARAM, 0.f, 1.f, 0.f, "Freq. Mod", "%", 0.f, 100.f);
    configParam(CMOD_PARAM2, -1.f, 1.f, 0.f, "Freq. Mod2", "%", 0.f, 100.f);
    configParam(DRIVE_PARAM, -5.f, 5.f, 0.f, "Drive Level", "%", 0.f, 100.f);

    configParam(CUTOFF2_PARAM, 0.f, 1.f, 1.f, "Center Freq.", " Hz", params[CUTOFF_PARAM].getValue(), 20.f * 410.58f);
    configParam(Q2_PARAM, .1f, 1.f, .1f, "Q factor", "", params[Q2_PARAM].getValue(), 20.f);
    configParam(CMOD2_PARAM, 0.f, 1.f, 0.f, "Freq. Mod", "%", 0.f, 100.f);
    configParam(CMOD2_PARAM2, -1.f, 1.f, 0.f, "Freq. Mod2", "%", 0.f, 100.f);
    configParam(DRIVE2_PARAM, -5.f, 5.f, 0.f, "Drive Level", "%", 0.f, 100.f);

    configParam(VOLA_PARAM, 0.f, 5.f, 0.f, "Amp A Level", "%", 0.f, 100.f);
    configParam(VOLB_PARAM, 0.f, 5.f, 0.f, "Amp B Level", "%", 0.f, 100.f);

    configParam(FILTERSEL_PARAM, 0.f, 2.f, 0.f, "FilterA Type");
    configParam(FILTER2SEL_PARAM, 0.f, 2.f, 0.f, "FilterB Type");

		configParam(FADE_PARAM, 0.f, 1.f, 0.f, "Fade Filter");

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

void process(const ProcessArgs &args) override {

 //float outLP;
 //float outHP;
 //float outBP;
 //float out2LP;
 //float out2HP;
 //float out2BP;


	float cutoff = std::pow(2.0f,rescale(clamp(params[CUTOFF_PARAM].getValue() + ((params[CMOD_PARAM2].getValue())*0.1f*inputs[CUTOFF_INPUT2].getVoltage()+(params[CMOD_PARAM].getValue())*0.1f*inputs[CUTOFF_INPUT].getVoltage()) *0.2f ,0.0f,1.0f),0.0f,1.0f,4.5f,14.0f));
	float cutoff2 = std::pow(2.0f,rescale(clamp(params[CUTOFF2_PARAM].getValue() + ((params[CMOD2_PARAM2].getValue())*0.1f*inputs[CUTOFF2_INPUT2].getVoltage() +(params[CMOD2_PARAM].getValue())*0.1f*inputs[CUTOFF2_INPUT].getVoltage()) *0.2f ,0.0f,1.0f),0.0f,1.0f,4.5f,14.0f));

	float q = 10.0f * clamp(params[Q_PARAM].getValue() + inputs[Q_INPUT].getVoltage() *0.2f, 0.1f, 1.0f);
	float q2 = 10.0f * clamp(params[Q2_PARAM].getValue() + inputs[Q2_INPUT].getVoltage() *0.2f, 0.1f, 1.0f);

	filterA.setParams(cutoff,q,args.sampleRate);
	filterB.setParams(cutoff2,q2,args.sampleRate);

	float in = inputs[IN].getVoltage() * params[VOLA_PARAM].getValue() *0.2f;
	float in2 = inputs[IN2].getVoltage() * params[VOLB_PARAM].getValue()*0.2f;

////////////////////////////////////////////////////////////////


	in = clamp(in, -5.0f, 5.0f) * 0.2f;
	in2 = clamp(in2, -5.0f, 5.0f) * 0.2f;

	float a_shape = params[DRIVE_PARAM].getValue() + clamp(inputs[DRIVE_INPUT].getVoltage(), -5.0f, 5.0f);
	a_shape = clamp(a_shape, -5.0f, 5.0f) * 0.2f;
	a_shape *= 0.99f;

	float b_shape = params[DRIVE2_PARAM].getValue() + clamp(inputs[DRIVE2_INPUT].getVoltage(), -5.0f, 5.0f);
	b_shape = clamp(b_shape, -5.0f, 5.0f) * 0.2f;
	b_shape *= 0.99f;

	const float a_shapeB = (1.0 - a_shape) / (1.0 + a_shape);
	const float a_shapeA = (4.0 * a_shape) / ((1.0 - a_shape) * (1.0 + a_shape));

	const float b_shapeB = (1.0 - b_shape) / (1.0 + b_shape);
	const float b_shapeA = (4.0 * b_shape) / ((1.0 - b_shape) * (1.0 + b_shape));

	float a_outputd = in * (a_shapeA + a_shapeB);
	float b_outputd = in2 * (b_shapeA + b_shapeB);

	a_outputd = a_outputd / ((std::abs(in) * a_shapeA) + a_shapeB);
	b_outputd = b_outputd / ((std::abs(in2) * b_shapeA) + b_shapeB);

///////////////////////////////////////////////////////////////////


	filterA.calcOutput(a_outputd);
	filterB.calcOutput(b_outputd);

	float xfade = params[FADE_PARAM].getValue()+inputs[FADE_CV].getVoltage() / 10.0;
	lights[FADEA_LIGHTS].value=(1-xfade);
	lights[FADEB_LIGHTS].value=xfade;


	int sel1 = round(params[FILTERSEL_PARAM].getValue());
	int sel2 = round(params[FILTER2SEL_PARAM].getValue());

	for (int i=0;i<4;i++)
	{
	if (sel1 == 0)
		outputs[OUT1].setVoltage(filterA.lp * 3.0f);
	if (sel1 == 1)
		outputs[OUT1].setVoltage(filterA.bp * 3.0f);
	if (sel1 == 2)
		outputs[OUT1].setVoltage(filterA.hp * 3.0f);
    }


	for (int i = 0; i < 4; i++)
	{
		if (sel2 == 0)
			outputs[OUT2].setVoltage(filterB.lp * 3.0f);
		if (sel2 == 1)
			outputs[OUT2].setVoltage(filterB.bp * 3.0f);
		if (sel2 == 2)
			outputs[OUT2].setVoltage(filterB.hp * 3.0f);
    }

	float filter1 = outputs[OUT1].getVoltage();
	float filter2 = outputs[OUT2].getVoltage();


	outputs[MIXOUT].setVoltage((filter1 * ( 1-xfade ))+(filter2 * xfade));

  }
};


struct DualFilterWidget:ModuleWidget {

    SvgPanel* darkPanel;
    struct PanelThemeItem : MenuItem {
      DualFilter *module;
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

      DualFilter *module = dynamic_cast<DualFilter*>(this->module);
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


    DualFilterWidget(DualFilter *module){
      setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Light/DualFilter.svg")));
    if (module) {
      darkPanel = new SvgPanel();
      darkPanel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Dark/DualFilter.svg")));
      darkPanel->visible = false;
      addChild(darkPanel);
    }

    addChild(createWidget<ScrewBlack>(Vec(15, 0)));
    addChild(createWidget<ScrewBlack>(Vec(box.size.x - 30, 0)));
    addChild(createWidget<ScrewBlack>(Vec(15, 365)));
    addChild(createWidget<ScrewBlack>(Vec(box.size.x - 30, 365)));


    int i=90;
    int s=27;
    int l=7;
    int of = -25;
    int cv = 314;


	 addChild(createLight<MediumLight<GreenLight>>(Vec(i-28,21),module,DualFilter::FADEA_LIGHTS));
	 addChild(createLight<MediumLight<GreenLight>>(Vec(i+20,21),module,DualFilter::FADEB_LIGHTS));
// 
	 addParam(createParam<VerboDS>(Vec(i -17.5 , 21), module, DualFilter::FADE_PARAM));
// 
// 
	 addParam(createParam<VerboDS>(Vec(i-85+16, 30), module, DualFilter::CUTOFF_PARAM));
	 addParam(createParam<VerboDS>(Vec(i+50-16, 30), module, DualFilter::CUTOFF2_PARAM));
	 //
	 addParam(createParam<VerboDS>(Vec(of + 75, 83), module, DualFilter::Q_PARAM));
	 addParam(createParam<VerboDS>(Vec(of + 33, 103), module, DualFilter::CMOD_PARAM));
	 //
	 addParam(createParam<VerboDS>(Vec(of + 32 + i, 83), module, DualFilter::Q2_PARAM));
	 addParam(createParam<VerboDS>(Vec(137, 103), module, DualFilter::CMOD2_PARAM));
	 //
	 addParam(createParam<VerboS>(Vec(of + 75, 135), module, DualFilter::DRIVE_PARAM));
	 addParam(createParam<VerboS>(Vec(of + 32 + i, 135), module, DualFilter::DRIVE2_PARAM));

	 addParam(createParam<VerboDS>(Vec(of + 33, 155), module, DualFilter::CMOD_PARAM2));
	 addParam(createParam<VerboDS>(Vec(137, 155), module, DualFilter::CMOD2_PARAM2));

	 addParam(createParam<VerboDS>(Vec(of + 75, 190), module, DualFilter::VOLA_PARAM));
	 addParam(createParam<VerboDS>(Vec(of + 32 + i, 190), module, DualFilter::VOLB_PARAM));

	 addParam(createParam<VerboDSSnapKnob>(Vec(of + 33, 210), module, DualFilter::FILTERSEL_PARAM));
	 addParam(createParam<VerboDSSnapKnob>(Vec(137, 210), module, DualFilter::FILTER2SEL_PARAM));
	 //
	 //
	 //
	 //
	  addInput(createInput<PJ301MCPort>(Vec(l, 260),module, DualFilter::CUTOFF_INPUT));
	  addInput(createInput<PJ301MCPort>(Vec(l + s , 260),module, DualFilter::CUTOFF_INPUT2));
	  addInput(createInput<PJ301MCPort>(Vec(l , 260+s),module, DualFilter::Q_INPUT));
	  addInput(createInput<PJ301MRPort>(Vec(l + s , 260+s),module, DualFilter::DRIVE_INPUT));
	 //
	  addInput(createInput<PJ301MCPort>(Vec(l + s , cv+15),module, DualFilter::FADE_CV));
	 //
	  addInput(createInput<PJ301MCPort>(Vec(i+2+ s*2 , 260),module, DualFilter::CUTOFF2_INPUT));
	  addInput(createInput<PJ301MCPort>(Vec(i+2+ s , 260),module, DualFilter::CUTOFF2_INPUT2));
	  addInput(createInput<PJ301MCPort>(Vec(i+2+ s*2 , 260+s),module, DualFilter::Q2_INPUT));
	  addInput(createInput<PJ301MRPort>(Vec(i+2+ s , 260+s),module, DualFilter::DRIVE2_INPUT));
	 //
	  addInput(createInput<PJ301MIPort>(Vec(l + s * 2, cv),module, DualFilter::IN));
	  addInput(createInput<PJ301MIPort>(Vec(i+2, cv),module, DualFilter::IN2));
	 //
	  addOutput(createOutput<PJ301MOPort>(Vec(l , cv),module, DualFilter::OUT1));
	 //
	  addOutput(createOutput<PJ301MOPort>(Vec(i+2 +  s * 2, cv),module, DualFilter::OUT2));
	 //
	  addOutput(createOutput<PJ301MOPort>(Vec(i+2+s, cv+15),module, DualFilter::MIXOUT));
}
void step() override {
  if (module) {
	Widget* panel = getPanel();
    panel->visible = ((((DualFilter*)module)->panelTheme) == 0);
    darkPanel->visible  = ((((DualFilter*)module)->panelTheme) == 1);
  }
  Widget::step();
}
};

Model *modelDualFilter = createModel<DualFilter, DualFilterWidget>("DualFilter");
