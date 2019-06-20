#include "plugin.hpp"

////////////////////////////////////////////////////////

template <int OVERSAMPLE, int QUALITY>

struct sineOsc {

	float lastSyncValue = 0.0;
	float phase = 0.0;
	float freq;
	float pitch;

	dsp::Decimator<OVERSAMPLE, QUALITY> sinDecimator;

	// For analog detuning effect
	float pitchSlew = 0.0;
	int pitchSlewIndex = 0;

	float sinBuffer[OVERSAMPLE] = {};

	void setPitch(float pitchKnob, float pitchCv) {
		// Compute frequency
		pitch = pitchKnob;
		// Apply pitch slew
		const float pitchSlewAmount = 3.0;
		pitch += pitchSlew * pitchSlewAmount;
		pitch += pitchCv;
		// Note C3
		freq = 261.626 * std::pow(2.0, pitch / 12.0);
	}


	void process(float deltaTime, float syncValue) {
		
			// Adjust pitch slew
			if (++pitchSlewIndex > 32) {
				const float pitchSlewTau = 100.0; // Time constant for leaky integrator in seconds
				pitchSlew += (random::normal() - pitchSlew / pitchSlewTau) * APP->engine->getSampleTime();
				pitchSlewIndex = 0;
			}
		
		// Advance phase
		float deltaPhase = clamp(freq * deltaTime, 1e-6, 0.5f);

		// Detect sync
		int syncIndex = -1; // Index in the oversample loop where sync occurs [0, OVERSAMPLE)

		for (int i = 0; i < OVERSAMPLE; i++) {
			if (syncIndex == i) {
					// phase = syncCrossing * deltaPhase / OVERSAMPLE;
					phase = 0.0;
				}
			

				// Quadratic approximation of sine, slightly richer harmonics
				if (phase < 0.5f)
					sinBuffer[i] = 1.f - 16.f * std::pow(phase - 0.25f, 2);
				else
					sinBuffer[i] = -1.f + 16.f * std::pow(phase - 0.75f, 2);

				sinBuffer[i] *= 1.08f;

			// Advance phase
			phase += deltaPhase / OVERSAMPLE;
			phase = eucMod(phase, 1.0);
		}
	}

	float sin() {
		return sinDecimator.process(sinBuffer);
	}

	float light() {
		return std::sin(2*M_PI * phase);
	}
};

/////////////////////////////////////////////////////////

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

        B_PITCH_PARAM,
        B_FINE_PARAM,
        B_FOLD_PARAM,
        B_DRIVE_PARAM,
        B_SAW_PARAM,
        B_SQUARE_PARAM,
        B_FM_PARAM,

        
        NUM_PARAMS
    };
	enum InputIds
	{

		A_FM_INPUT,
		A_SAW_INPUT,
		A_SQUARE_INPUT,
		A_PITCH_INPUT,
		A_FOLD_INPUT,
		A_DRIVE_INPUT,

		B_FM_INPUT,
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

	sineOsc <8, 8> osc_a;
	sineOsc <8, 8> a_harmonic[5]={};
	sineOsc <8, 8> a_harmonicq[5] = {};
	sineOsc <8, 8> osc_b;
	sineOsc <8, 8> b_harmonic[5] = {};
	sineOsc <8, 8> b_harmonicq[5] = {};

	DAOSC(){
	config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_sinS);
	
	 configParam(A_PITCH_PARAM,  -54.f, 54.f, 0.f, "Osc1 Frequency", "Hz", std::pow(2, 1 / 12.f), dsp::FREQ_C4);
     configParam(A_FINE_PARAM,  -1.f, 1.f, 0.f, "Osc1 Fine frequency");
     configParam(A_FOLD_PARAM,  0.0, 5.0, 0.0,"Fold");
     configParam(A_DRIVE_PARAM,  -5.0, 5.0, 0.,"Drive");
     configParam(A_SAW_PARAM,  0.0, 1.0, 0.0,"Saw Harmonic");
     configParam(A_SQUARE_PARAM,  0.0, 1.0, 0.0,"Square Harmonic");
     configParam(A_FM_PARAM,  0.0, 1.0, 0.0,"Fm amount");

     configParam(B_PITCH_PARAM,  -54.f, 54.f, 0.f, "Osc2 Frequency", "Hz", std::pow(2, 1 / 12.f), dsp::FREQ_C4);
     configParam(B_FINE_PARAM,  -1.f, 1.f, 0.f, "Osc2 Fine frequency");
     configParam(B_FOLD_PARAM,  0.0, 5.0, 0.0,"Fold");
     configParam(B_DRIVE_PARAM,  -5.0, 5.0, 0.,"Drive");
     configParam(B_SAW_PARAM,  0.0, 1.0, 0.0,"Saw Harmonic");
     configParam(B_SQUARE_PARAM,  0.0, 1.0, 0.0,"Square Harmonic");
     configParam(B_FM_PARAM,  0.0, 1.0, 0.0,"Fm amount");
	
	}

	void process(const ProcessArgs &args) override 
	{

	float a_harmsum = 0.0;
	float a_harmsumq = 0.0;
	float b_harmsum = 0.0;
	float b_harmsumq = 0.0;

	float a_pitchCv = 12.0 * inputs[A_PITCH_INPUT].value;
	float b_pitchCv = 12.0 * inputs[B_PITCH_INPUT].value;

	float a_pitchFine = 3.0 * dsp::quadraticBipolar(params[A_FINE_PARAM].value);
	float b_pitchFine = 3.0 * dsp::quadraticBipolar(params[B_FINE_PARAM].value);

	if (inputs[A_FM_INPUT].isConnected())
	{
		a_pitchCv += dsp::quadraticBipolar(params[A_FM_PARAM].value) * 12.0 * inputs[A_FM_INPUT].value;
	}

	if (inputs[B_FM_INPUT].isConnected())
	{
		b_pitchCv += dsp::quadraticBipolar(params[B_FM_PARAM].value) * 12.0 * inputs[B_FM_INPUT].value;
	}

	osc_a.setPitch(params[A_PITCH_PARAM].value, a_pitchFine + a_pitchCv);
	osc_a.process(APP->engine->getSampleTime(), 8.0);
	osc_b.setPitch(params[B_PITCH_PARAM].value, b_pitchFine + b_pitchCv);
	osc_b.process(APP->engine->getSampleTime(), 8.0);

	for (int i =0; i < 5; i++)
	{ 
		a_harmonic[i].freq=(((i+1)*2)*osc_a.freq);
		a_harmonic[i].process(APP->engine->getSampleTime(), 8.0);

		a_harmsum += (a_harmonic[i].sin()/(i+2))*params[A_SAW_PARAM].value; 
		if(inputs[A_SAW_INPUT].isConnected())
		a_harmsum *=clamp(inputs[A_SAW_INPUT].value/10.f,-1.f,1.f);


		a_harmonicq[i].freq=((((i+1)*2)+1) * osc_a.freq);
		a_harmonicq[i].process(APP->engine->getSampleTime(), 8.0);

		a_harmsumq += (a_harmonicq[i].sin()/(i+2))*params[A_SQUARE_PARAM].value; 
		if(inputs[A_SQUARE_INPUT].isConnected())
		a_harmsumq*=clamp(inputs[A_SQUARE_INPUT].value/10.f,-1.f,1.f);


		b_harmonic[i].freq=(((i+1)*2) * osc_b.freq);
		b_harmonic[i].process(APP->engine->getSampleTime(), 8.0);

		b_harmsum += (b_harmonic[i].sin()/(i+2))*params[B_SAW_PARAM].value; 
		if(inputs[B_SAW_INPUT].isConnected())
		b_harmsum *=clamp(inputs[B_SAW_INPUT].value/10.f,-1.f,1.f);


		b_harmonicq[i].freq=((((i+1)*2)+1) * osc_b.freq);
		b_harmonicq[i].process(APP->engine->getSampleTime(), 8.0);

		b_harmsumq += (b_harmonicq[i].sin()/(i+2))*params[B_SQUARE_PARAM].value; 
		if(inputs[B_SQUARE_INPUT].isConnected())
		b_harmsumq*=clamp(inputs[B_SQUARE_INPUT].value/10.f,-1.f,1.f);

			}
    

	//////////////// Contrast - Thx to  Michael Hetrick!!!

	////////////////A

	float a_inputf = 2*(osc_a.sin() + a_harmsum + a_harmsumq);
	float b_inputf = 2*(osc_b.sin() + b_harmsum + b_harmsumq);

	a_inputf = clamp(a_inputf, -6.0f, 6.0f) * 0.2f;
	b_inputf = clamp(b_inputf, -6.0f, 6.0f) * 0.2f;

	float a_contrast = params[A_FOLD_PARAM].value + clamp(inputs[A_FOLD_INPUT].value, 0.0f, 6.0f);
	float b_contrast = params[B_FOLD_PARAM].value + clamp(inputs[B_FOLD_INPUT].value, 0.0f, 6.0f);

	a_contrast = clamp(a_contrast, 0.0f, 6.0f) * 0.2f;
	b_contrast = clamp(b_contrast, 0.0f, 6.0f) * 0.2f;

	const float a_factor1 = a_inputf * 1.57143;
	const float a_factor2 = sinf(a_inputf * 6.28571) * a_contrast;

	const float b_factor1 = b_inputf * 1.57143;
	const float b_factor2 = sinf(b_inputf * 6.28571) * b_contrast;

	float a_outputf = sinf(a_factor1 + a_factor2);
	a_outputf *= 6.0f;

	float b_outputf = sinf(b_factor1 + b_factor2);
	b_outputf *= 6.0f;

	//////////////////////////Wave shape - Thx to  Michael Hetrick!!!

	float a_inputd = a_outputf;
	float b_inputd = b_outputf;

	a_inputd = clamp(a_inputd, -5.0f, 5.0f) * 0.2f;
	b_inputd = clamp(b_inputd, -5.0f, 5.0f) * 0.2f;

	float a_shape = params[A_DRIVE_PARAM].value + clamp(inputs[A_DRIVE_INPUT].value, -5.0f, 5.0f);
	a_shape = clamp(a_shape, -5.0f, 5.0f) * 0.2f;
	a_shape *= 0.99f;

	float b_shape = params[B_DRIVE_PARAM].value + clamp(inputs[B_DRIVE_INPUT].value, -5.0f, 5.0f);
	b_shape = clamp(b_shape, -5.0f, 5.0f) * 0.2f;
	b_shape *= 0.99f;

	const float a_shapeB = (1.0 - a_shape) / (1.0 + a_shape);
	const float a_shapeA = (4.0 * a_shape) / ((1.0 - a_shape) * (1.0 + a_shape));

	const float b_shapeB = (1.0 - b_shape) / (1.0 + b_shape);
	const float b_shapeA = (4.0 * b_shape) / ((1.0 - b_shape) * (1.0 + b_shape));

	float a_outputd = a_inputd * (a_shapeA + a_shapeB);
	float b_outputd = b_inputd * (b_shapeA + b_shapeB);

	a_outputd = a_outputd / ((std::abs(a_inputd) * a_shapeA) + a_shapeB);
	b_outputd = b_outputd / ((std::abs(b_inputd) * b_shapeA) + b_shapeB);


	////////////////////////////////////////////////////////
	outputs[A_OUTPUT].value = 5.0f * a_outputd;
	outputs[B_OUTPUT].value = 5.0f * b_outputd;

	outputs[SUM_OUTPUT].value = 5.0f * (a_outputd + b_outputd) / 2;
}
};

struct DAOSCWidget : ModuleWidget{
DAOSCWidget(DAOSC *module) {
setModule(module);
setPanel(APP->window->loadSvg(asset::plugin(pluginInstance,  "res/DAOSC.svg")));

int knob=42;
int jack=30;
float mid = 97.5;
int top = 20;
int down = 50;

addChild(createWidget<ScrewBlack>(Vec(15, 0)));
addChild(createWidget<ScrewBlack>(Vec(box.size.x - 30, 0)));
addChild(createWidget<ScrewBlack>(Vec(15, 365)));
addChild(createWidget<ScrewBlack>(Vec(box.size.x - 30, 365)));

addParam(createParam<LRoundWhy>(Vec(box.size.x-mid-50, top), module, DAOSC::A_PITCH_PARAM));
addParam(createParam<RoundWhy>(Vec(box.size.x-mid-knob*2 - 10, top), module, DAOSC::A_FINE_PARAM));
addParam(createParam<RoundWhy>(Vec(box.size.x - mid - knob * 1 , top + knob + 45), module, DAOSC::A_FM_PARAM));
addParam(createParam<RoundAzz>(Vec(box.size.x - mid - knob * 2 - 5, top + knob + 5), module, DAOSC::A_FOLD_PARAM));
addParam(createParam<RoundRed>(Vec(box.size.x - mid - knob * 2 - 5, 125), module, DAOSC::A_DRIVE_PARAM));
addParam(createParam<RoundWhy>(Vec(box.size.x-mid-knob, 157), module, DAOSC::A_SQUARE_PARAM));
addParam(createParam<RoundWhy>(Vec(box.size.x-mid-knob*2, 177), module, DAOSC::A_SAW_PARAM));

addInput(createInput<PJ301MCPort>(Vec(box.size.x-mid-jack-5, 160+down), module, DAOSC::A_FM_INPUT));
addInput(createInput<PJ301MCPort>(Vec(box.size.x-mid-jack-5, 190+down), module, DAOSC::A_PITCH_INPUT));
addInput(createInput<PJ301MCPort>(Vec(box.size.x-mid-jack*2-5, 190+down), module, DAOSC::A_FOLD_INPUT));
addInput(createInput<PJ301MCPort>(Vec(box.size.x-mid-jack*3-5, 190+down), module, DAOSC::A_DRIVE_INPUT));
addInput(createInput<PJ301MCPort>(Vec(box.size.x-mid-jack*2-5, 230+down), module, DAOSC::A_SQUARE_INPUT));
addInput(createInput<PJ301MCPort>(Vec(box.size.x-mid-jack*3-5, 230+down), module, DAOSC::A_SAW_INPUT));

addOutput(createOutput<PJ301MOPort>(Vec(box.size.x - mid-jack-5, 230+down), module, DAOSC::A_OUTPUT));

addParam(createParam<LRoundWhy>(Vec(box.size.x-mid+5, top), module, DAOSC::B_PITCH_PARAM));
addParam(createParam<RoundWhy>(Vec(box.size.x-mid+5+knob+10, top), module, DAOSC::B_FINE_PARAM));
addParam(createParam<RoundWhy>(Vec(box.size.x - mid + 5, top + knob+45), module, DAOSC::B_FM_PARAM));
addParam(createParam<RoundAzz>(Vec(box.size.x - mid + 10 + knob, top + knob + 5), module, DAOSC::B_FOLD_PARAM));
addParam(createParam<RoundRed>(Vec(box.size.x - mid + 10 + knob, 125), module, DAOSC::B_DRIVE_PARAM));
addParam(createParam<RoundWhy>(Vec(box.size.x-mid+5, 157), module, DAOSC::B_SQUARE_PARAM));
addParam(createParam<RoundWhy>(Vec(box.size.x-mid+5+knob, 177), module, DAOSC::B_SAW_PARAM));

addInput(createInput<PJ301MCPort>(Vec(box.size.x-mid+10, 160+down), module, DAOSC::B_FM_INPUT));
addInput(createInput<PJ301MCPort>(Vec(box.size.x-mid+10, 190+down), module, DAOSC::B_PITCH_INPUT));
addInput(createInput<PJ301MCPort>(Vec(box.size.x-mid+10+jack, 190+down), module, DAOSC::B_FOLD_INPUT));
addInput(createInput<PJ301MCPort>(Vec(box.size.x-mid+10+jack*2, 190+down), module, DAOSC::B_DRIVE_INPUT));
addInput(createInput<PJ301MCPort>(Vec(box.size.x-mid+10+jack, 230+down), module, DAOSC::B_SQUARE_INPUT));
addInput(createInput<PJ301MCPort>(Vec(box.size.x-mid+10+jack*2, 230+down), module, DAOSC::B_SAW_INPUT));

addOutput(createOutput<PJ301MOPort>(Vec(box.size.x - mid+10, 230+down), module, DAOSC::B_OUTPUT));

addOutput(createOutput<PJ301MOPort>(Vec(box.size.x - mid-12.5, 265+down), module, DAOSC::SUM_OUTPUT));
}
};
Model *modelDAOSC = createModel<DAOSC, DAOSCWidget>("DAOSC");
