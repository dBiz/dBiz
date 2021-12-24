#include "plugin.hpp"
 

using simd::float_4;

// Accurate only on [0, 1]
template <typename T>
T sin2pi_pade_05_7_6(T x) {
	x -= 0.5f;
	return (T(-6.28319) * x + T(35.353) * simd::pow(x, 3) - T(44.9043) * simd::pow(x, 5) + T(16.0951) * simd::pow(x, 7))
		/ (1 + T(0.953136) * simd::pow(x, 2) + T(0.430238) * simd::pow(x, 4) + T(0.0981408) * simd::pow(x, 6));
}

template <typename T>
T sin2pi_pade_05_5_4(T x) {
	x -= 0.5f;
	return (T(-6.283185307) * x + T(33.19863968) * simd::pow(x, 3) - T(32.44191367) * simd::pow(x, 5))
		/ (1 + T(1.296008659) * simd::pow(x, 2) + T(0.7028072946) * simd::pow(x, 4));
}

template <typename T>
T expCurve(T x) {
	return (3 + x * (-13 + 5 * x)) / (3 + 2 * x);
}


template <int OVERSAMPLE, int QUALITY, typename T>
struct sineOsc {
	bool analog = false;
	bool soft = false;
	bool syncEnabled = false;
	// For optimizing in serial code
	int channels = 0;

	T lastSyncValue = 0.f;
	T phase = 0.f;
	T freq;
	T pulseWidth = 0.5f;
	T syncDirection = 1.f;

	dsp::TRCFilter<T> sqrFilter;

	dsp::MinBlepGenerator<QUALITY, OVERSAMPLE, T> sinMinBlep;

	T sinValue = 0.f;

	void process(float deltaTime, T syncValue) {
		// Advance phase
		T deltaPhase = simd::clamp(freq * deltaTime, 1e-6f, 0.35f);
		if (soft) {
			// Reverse direction
			deltaPhase *= syncDirection;
		}
		else {
			// Reset back to forward
			syncDirection = 1.f;
		}
		phase += deltaPhase;
		// Wrap phase
		phase -= simd::floor(phase);

		// Sin
		sinValue = sin(phase);
		sinValue += sinMinBlep.process();
	}

	T sin(T phase) {
		T v;
		if (analog) {
			// Quadratic approximation of sine, slightly richer harmonics
			T halfPhase = (phase < 0.5f);
			T x = phase - simd::ifelse(halfPhase, 0.25f, 0.75f);
			v = 1.f - 16.f * simd::pow(x, 2);
			v *= simd::ifelse(halfPhase, 1.f, -1.f);
		}
		else {
			v = sin2pi_pade_05_5_4(phase);
			// v = sin2pi_pade_05_7_6(phase);
			// v = simd::sin(2 * T(M_PI) * phase);
		}
		return v;
	}
	T sin() {
		return sinValue;
	}

	T light() {
		return simd::sin(2 * T(M_PI) * phase);
	}
};

///////// inspired to Tutorial Module!!

struct DAOSC : Module {
	enum ParamIds
	{
		A_PITCH_PARAM,
		A_FINE_PARAM,
		A_FOLD_PARAM,
		A_DRIVE_PARAM,
		A_SAW_PARAM,
		A_SQUARE_PARAM,
		A_FM_PARAM,
		A_FM2_PARAM,

		B_PITCH_PARAM,
		B_FINE_PARAM,
		B_FOLD_PARAM,
		B_DRIVE_PARAM,
		B_SAW_PARAM,
		B_SQUARE_PARAM,
		B_FM_PARAM,
		B_FM2_PARAM,

		NUM_PARAMS
	};
	enum InputIds
	{

		A_FM_INPUT,
		A_FM2_INPUT,
		A_SAW_INPUT,
		A_SQUARE_INPUT,
		A_PITCH_INPUT,
		A_FOLD_INPUT,
		A_DRIVE_INPUT,

		B_FM_INPUT,
		B_FM2_INPUT,
		B_SAW_INPUT,
		B_SQUARE_INPUT,
		B_PITCH_INPUT,
		B_DRIVE_INPUT,
		B_FOLD_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
		A_OUTPUT,
        B_OUTPUT,
        SUM_OUTPUT,
		NUM_OUTPUTS
	};
	enum sinIds {
		NUM_sinS
	};

	float phase = 0.0;
	float blinkPhase = 0.0;

	sineOsc <8, 8, float_4> osc_a[4]={};
	sineOsc <8, 8, float_4> a_harmonic[10]={};
	sineOsc <8, 8, float_4> a_harmonicq[10] = {};
	sineOsc <8, 8, float_4> osc_b[4]={};
	sineOsc <8, 8, float_4> b_harmonic[10] = {};
	sineOsc <8, 8, float_4> b_harmonicq[10] = {};

	int panelTheme;

	DAOSC(){
	config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_sinS);

	 configParam(A_PITCH_PARAM,  -54.f, 54.f, 0.f, "Osc1 Frequency", " Hz", dsp::FREQ_SEMITONE, dsp::FREQ_C4);
     configParam(A_FINE_PARAM,  -1.f, 1.f, 0.f, "Osc1 Fine frequency");
     configParam(A_FOLD_PARAM,  0.0, 5.0, 0.0,"Fold");
     configParam(A_DRIVE_PARAM,  0.0, 5.0, 0.,"Drive");
     configParam(A_SAW_PARAM,  0.0, 1.0, 0.0,"Saw Harmonic");
     configParam(A_SQUARE_PARAM,  0.0, 1.0, 0.0,"Square Harmonic");
     configParam(A_FM_PARAM,  -1.0, 1.0, 0.0,"Fm amount");
	 getParamQuantity(A_FM_PARAM)->randomizeEnabled = false;
	 configParam(A_FM2_PARAM,  -1.0, 1.0, 0.0,"Fm2 amount");
	 getParamQuantity(A_FM2_PARAM)->randomizeEnabled = false;

     configParam(B_PITCH_PARAM,  -54.f, 54.f, 0.f, "Osc2 Frequency", " Hz", dsp::FREQ_SEMITONE, dsp::FREQ_C4);
     configParam(B_FINE_PARAM,  -1.f, 1.f, 0.f, "Osc2 Fine frequency");
     configParam(B_FOLD_PARAM,  0.0, 5.0, 0.0,"Fold");
     configParam(B_DRIVE_PARAM,  0.0, 5.0, 0.,"Drive");
     configParam(B_SAW_PARAM,  0.0, 1.0, 0.0,"Saw Harmonic");
     configParam(B_SQUARE_PARAM,  0.0, 1.0, 0.0,"Square Harmonic");
     configParam(B_FM_PARAM,  -1.0, 1.0, 0.0,"Fm amount");
	 getParamQuantity(B_FM_PARAM)->randomizeEnabled = false;
	 configParam(B_FM2_PARAM,  -1.0, 1.0, 0.0,"Fm2 amount");
	 getParamQuantity(B_FM2_PARAM)->randomizeEnabled = false;
	

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

	float_4 va=0.f;
	float_4 vb=0.f;
	float_4 vaO=0.f;
	float_4 vaE=0.f;
	float_4 vbO=0.f;
	float_4 vbE=0.f;
	float_4 sumA=0.f;
	float_4 sumB=0.f;

	float_4 pitchA;
	float_4 pitchB;

	float freqParamA = params[A_PITCH_PARAM].getValue() / 12.f;
	float fmParamA =  params[A_FM_PARAM].getValue();
	float fmParamA2 = params[A_FM2_PARAM].getValue();

	float freqParamB = params[B_PITCH_PARAM].getValue() / 12.f;
	float fmParamB =  params[B_FM_PARAM].getValue();
	float fmParamB2 = params[B_FM2_PARAM].getValue();

	int channelsA = std::max(inputs[A_PITCH_INPUT].getChannels(), 1);
	int channelsB = std::max(inputs[B_PITCH_INPUT].getChannels(), 1);

	for (int c = 0; c < channelsA; c += 4)
	{
			auto& oscA = osc_a[c / 4];
			oscA.channels = std::min(channelsA - c, 4);
			oscA.analog = true;
			oscA.soft = 0;


			pitchA = freqParamA + inputs[A_PITCH_INPUT].getVoltageSimd<float_4>(c);
			float_4 freqA;

			pitchA += fmParamA * inputs[A_FM_INPUT].getPolyVoltageSimd<float_4>(c);		
			freqA += dsp::FREQ_C4 * inputs[A_FM2_INPUT].getPolyVoltageSimd<float_4>(c) * fmParamA2;


			freqA += dsp::FREQ_C4 * dsp::approxExp2_taylor5(pitchA + 30.f) / std::pow(2.f, 30.f);
			freqA = clamp(freqA, 0.f, args.sampleRate / 2.f);

			oscA.freq=freqA;
			oscA.process(args.sampleTime, 0.f);
			va+=oscA.sin();
	}

	for (int c = 0; c < channelsB; c += 4)
	{
			auto& oscB = osc_b[c / 4];
			oscB.channels = std::min(channelsB - c, 4);
			oscB.analog = true;
			oscB.soft = 0;


			pitchB = freqParamB + inputs[B_PITCH_INPUT].getVoltageSimd<float_4>(c);
			float_4 freqB;

			pitchB += fmParamB * inputs[B_FM_INPUT].getPolyVoltageSimd<float_4>(c);
			freqB += dsp::FREQ_C4 * inputs[B_FM2_INPUT].getPolyVoltageSimd<float_4>(c) * fmParamB2;
			
			freqB += dsp::FREQ_C4 * dsp::approxExp2_taylor5(pitchB + 30.f) / std::pow(2.f, 30.f);
			freqB = clamp(freqB, 0.f, args.sampleRate / 2.f);

			oscB.freq=freqB;
			oscB.process(args.sampleTime, 0.f);
			vb+=oscB.sin();
	}


	for (int i =0; i < 10; i++)
	{

		a_harmonic[i].freq=(((i+1)*2) * osc_a->freq);
		a_harmonic[i].process(args.sampleTime, 0.f);

		vaE+=(a_harmonic[i].sin()/(i+2))*params[A_SAW_PARAM].getValue();
		if(inputs[A_SAW_INPUT].isConnected())
			vaE *=clamp(inputs[A_SAW_INPUT].getVoltage()/10.f,-1.f,1.f);

		a_harmonicq[i].freq=((((i+1)*2)+1) * osc_a->freq);
		a_harmonicq[i].process(args.sampleTime, 0.f);

		vaO+=(a_harmonicq[i].sin()/(i+2))*params[A_SQUARE_PARAM].getValue();
		if(inputs[A_SQUARE_INPUT].isConnected())
			vaO *= clamp(inputs[A_SQUARE_INPUT].getVoltage() / 10.f, -1.f, 1.f);

		b_harmonic[i].freq=(((i+1)*2) * osc_b->freq);
		b_harmonic[i].process(args.sampleTime, 0.f);

		vbE+=(b_harmonic[i].sin()/(i+2))*params[B_SAW_PARAM].getValue();
		if(inputs[B_SAW_INPUT].isConnected())
			vbE *= clamp(inputs[B_SAW_INPUT].getVoltage() / 10.f, -1.f, 1.f);

		b_harmonicq[i].freq=((((i+1)*2)+1) * osc_b->freq);
		b_harmonicq[i].process(args.sampleTime, 0.f);

		vbO+=(b_harmonicq[i].sin()/(i+2))*params[B_SQUARE_PARAM].getValue();
		if(inputs[B_SQUARE_INPUT].isConnected())
			vbO *= clamp(inputs[B_SQUARE_INPUT].getVoltage() / 10.f, -1.f, 1.f);
	}
		sumA=va+vaO+vaE;
		sumB=vb+vbE+vbO;

/////////////////////////////////////////////////////////////////////////////

		float_4 sumDB;
		float_4 sumDA;
		float_4 a_outputf;
		float_4 b_outputf;

	/////////////////////////////////////////////////////////////////////////////

		for (int c = 0; c < channelsA; c += 4)
		{
			float_4 a_inputf = sumA*3.f;
			float_4 b_inputf = sumB*3.f;

			a_inputf = clamp(a_inputf, -5.0f, 5.0f) * 0.2f;
			b_inputf = clamp(b_inputf, -5.0f, 5.0f) * 0.2f;

			float a_contrast = params[A_FOLD_PARAM].getValue() + clamp(inputs[A_FOLD_INPUT].getVoltage(), 0.0f, 6.0f);
			float b_contrast = params[B_FOLD_PARAM].getValue() + clamp(inputs[B_FOLD_INPUT].getVoltage(), 0.0f, 6.0f);

			a_contrast = clamp(a_contrast, 0.0f, 5.0f) * 0.2f;
			b_contrast = clamp(b_contrast, 0.0f, 5.0f) * 0.2f;

			const float_4 a_factor1 = a_inputf * 1.57143;
			const float_4 a_factor2 = sin(a_inputf * 6.28571) * a_contrast;

			const float_4 b_factor1 = b_inputf * 1.57143;
			const float_4 b_factor2 = sin(b_inputf * 6.28571) * b_contrast;

			a_outputf = sin(a_factor1 + a_factor2);
			a_outputf *= 5.0f;

			b_outputf = sin(b_factor1 + b_factor2);
			b_outputf *= 5.0f;
		}
	///////////////////////////////////////////////////////////////////////////////

			for (int c = 0; c < channelsA; c += 4)
		{

			float_4 a_inputd = clamp(a_outputf, -5.0f, 5.0f) * 0.2f;
			float_4 b_inputd = clamp(b_outputf, -5.0f, 5.0f) * 0.2f;

			float a_shape = params[A_DRIVE_PARAM].getValue() + clamp(inputs[A_DRIVE_INPUT].getVoltage(), -5.0f, 5.0f);
			a_shape = clamp(a_shape, -5.0f, 5.0f) * 0.2f;
			a_shape *= 0.99f;

			float b_shape = params[B_DRIVE_PARAM].getValue() + clamp(inputs[B_DRIVE_INPUT].getVoltage(), -5.0f, 5.0f);
			b_shape = clamp(b_shape, -5.0f, 5.0f) * 0.2f;
			b_shape *= 0.99f;

			const float a_shapeB = (1.0 - a_shape) / (1.0 + a_shape);
			const float a_shapeA = (4.0 * a_shape) / ((1.0 - a_shape) * (1.0 + a_shape));

			const float b_shapeB = (1.0 - b_shape) / (1.0 + b_shape);
			const float b_shapeA = (4.0 * b_shape) / ((1.0 - b_shape) * (1.0 + b_shape));

			 float_4 a_outputd = a_inputd * (a_shapeA + a_shapeB);
			 float_4 b_outputd = b_inputd * (b_shapeA + b_shapeB);

			 a_outputd = a_outputd / ((fabs(a_inputd) * a_shapeA) + a_shapeB);
			 b_outputd = b_outputd / ((fabs(b_inputd) * b_shapeA) + b_shapeB);

			sumDA = a_outputd;
			sumDB = b_outputd;
		}

	//////////////////////////////////////////////////////////////////////////////////

		for (int c = 0; c < channelsB + channelsA; c += 8)
		{
			outputs[A_OUTPUT].setVoltageSimd(5 * sumDA, c);
			outputs[B_OUTPUT].setVoltageSimd(5 * sumDB, c);
			outputs[SUM_OUTPUT].setVoltageSimd(2.5f * (sumDA + sumDB), c);
	}


	outputs[A_OUTPUT].setChannels(channelsA);
	outputs[B_OUTPUT].setChannels(channelsB);
	outputs[SUM_OUTPUT].setChannels(channelsB+channelsA);

}
};

struct DAOSCWidget : ModuleWidget{


	SvgPanel* darkPanel;
	struct PanelThemeItem : MenuItem {
	  DAOSC *module;
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

	  DAOSC *module = dynamic_cast<DAOSC*>(this->module);
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

DAOSCWidget(DAOSC *module) {
setModule(module);
setPanel(APP->window->loadSvg(asset::plugin(pluginInstance,  "res/Light/DAOSC.svg")));
if (module) {
	darkPanel = new SvgPanel();
	darkPanel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Dark/DAOSC.svg")));
	darkPanel->visible = false;
	addChild(darkPanel);
}

int knob=42;
int jack=30;
float mid = 97.5;
int top = 20;
int down = 50;

addChild(createWidget<ScrewBlack>(Vec(15, 0)));
addChild(createWidget<ScrewBlack>(Vec(box.size.x - 30, 0)));
addChild(createWidget<ScrewBlack>(Vec(15, 365)));
addChild(createWidget<ScrewBlack>(Vec(box.size.x - 30, 365)));

addParam(createParam<LRoundWhy>(Vec(box.size.x-mid-70, top), module, DAOSC::A_PITCH_PARAM));
// addParam(createParam<RoundWhy>(Vec(box.size.x-mid-knob*2 - 10, top), module, DAOSC::A_FINE_PARAM));
addParam(createParam<RoundWhy>(Vec(box.size.x - mid - knob * 1 , top + knob + 8), module, DAOSC::A_FM_PARAM));
addParam(createParam<RoundWhy>(Vec(box.size.x - mid - knob * 1, top + knob + 48), module, DAOSC::A_FM2_PARAM));

addParam(createParam<RoundAzz>(Vec(box.size.x - mid - knob * 2 - 5, top + knob + 10), module, DAOSC::A_FOLD_PARAM));
addParam(createParam<RoundRed>(Vec(box.size.x - mid - knob * 2 - 5, 125), module, DAOSC::A_DRIVE_PARAM));
addParam(createParam<RoundWhy>(Vec(box.size.x-mid-knob, 160), module, DAOSC::A_SQUARE_PARAM));
addParam(createParam<RoundWhy>(Vec(box.size.x-mid-knob*2, 177), module, DAOSC::A_SAW_PARAM));

addInput(createInput<PJ301MCPort>(Vec(box.size.x-mid-jack-5, 160+down), module, DAOSC::A_FM_INPUT));
addInput(createInput<PJ301MCPort>(Vec(box.size.x - mid - jack - 5, 190 + down), module, DAOSC::A_FM2_INPUT));

addInput(createInput<PJ301MCPort>(Vec(box.size.x - mid - jack * 3 - 5, 270 + down), module, DAOSC::A_PITCH_INPUT));

addInput(createInput<PJ301MCPort>(Vec(box.size.x-mid-jack*2-5, 190+down), module, DAOSC::A_FOLD_INPUT));
addInput(createInput<PJ301MCPort>(Vec(box.size.x-mid-jack*3-5, 190+down), module, DAOSC::A_DRIVE_INPUT));
addInput(createInput<PJ301MCPort>(Vec(box.size.x-mid-jack*2-5, 230+down), module, DAOSC::A_SQUARE_INPUT));
addInput(createInput<PJ301MCPort>(Vec(box.size.x-mid-jack*3-5, 230+down), module, DAOSC::A_SAW_INPUT));

addOutput(createOutput<PJ301MOPort>(Vec(box.size.x - mid-jack-5, 230+down), module, DAOSC::A_OUTPUT));

addParam(createParam<LRoundWhy>(Vec(box.size.x-mid+25, top), module, DAOSC::B_PITCH_PARAM));
// addParam(createParam<RoundWhy>(Vec(box.size.x-mid+5+knob+10, top), module, DAOSC::B_FINE_PARAM));
addParam(createParam<RoundWhy>(Vec(box.size.x - mid + 5, top + knob+8), module, DAOSC::B_FM_PARAM));
addParam(createParam<RoundWhy>(Vec(box.size.x - mid + 5, top + knob + 48), module, DAOSC::B_FM2_PARAM));

addParam(createParam<RoundAzz>(Vec(box.size.x - mid + 10 + knob, top + knob + 10), module, DAOSC::B_FOLD_PARAM));
addParam(createParam<RoundRed>(Vec(box.size.x - mid + 10 + knob, 125), module, DAOSC::B_DRIVE_PARAM));
addParam(createParam<RoundWhy>(Vec(box.size.x-mid+5, 160), module, DAOSC::B_SQUARE_PARAM));
addParam(createParam<RoundWhy>(Vec(box.size.x-mid+5+knob, 177), module, DAOSC::B_SAW_PARAM));

addInput(createInput<PJ301MCPort>(Vec(box.size.x-mid+10, 160+down), module, DAOSC::B_FM_INPUT));
addInput(createInput<PJ301MCPort>(Vec(box.size.x - mid + 10, 190 + down), module, DAOSC::B_FM2_INPUT));

addInput(createInput<PJ301MCPort>(Vec(box.size.x - mid + 10 + jack * 2, 270 + down), module, DAOSC::B_PITCH_INPUT));

addInput(createInput<PJ301MCPort>(Vec(box.size.x-mid+10+jack, 190+down), module, DAOSC::B_FOLD_INPUT));
addInput(createInput<PJ301MCPort>(Vec(box.size.x-mid+10+jack*2, 190+down), module, DAOSC::B_DRIVE_INPUT));
addInput(createInput<PJ301MCPort>(Vec(box.size.x-mid+10+jack, 230+down), module, DAOSC::B_SQUARE_INPUT));
addInput(createInput<PJ301MCPort>(Vec(box.size.x-mid+10+jack*2, 230+down), module, DAOSC::B_SAW_INPUT));

addOutput(createOutput<PJ301MOPort>(Vec(box.size.x - mid+10, 230+down), module, DAOSC::B_OUTPUT));

addOutput(createOutput<PJ301MOPort>(Vec(box.size.x - mid-12.5, 265+down), module, DAOSC::SUM_OUTPUT));
}
void step() override {
  if (module) {
	Widget* panel = getPanel();
    panel->visible = ((((DAOSC*)module)->panelTheme) == 0);
    darkPanel->visible  = ((((DAOSC*)module)->panelTheme) == 1);
  }
  Widget::step();
}
};
Model *modelDAOSC = createModel<DAOSC, DAOSCWidget>("DAOSC");
