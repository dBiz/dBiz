///////////////////////////////////////////////////
//  dBiz Divider
// 
///////////////////////////////////////////////////

#include "plugin.hpp"

using namespace std;

/////added fine out /////////////////////////////////////////////////
struct Divider : Module {
  enum ParamIds
  {
		ENUMS(MODE_PARAM, 2),
    ENUMS(DIVISION_PARAM, 4),
    ENUMS(DIVISIONB_PARAM, 4),
    ENUMS(ON_SWITCH, 4),
    ENUMS(ON_SWITCHB, 4),
    NUM_PARAMS 
  };
  enum InputIds {
    CLOCK_INPUT,
    CLOCKB_INPUT,
		ENUMS(SUB1_INPUT, 4),
		ENUMS(SUB2_INPUT, 4),
    NUM_INPUTS
	};
	enum OutputIds
	{
		TRIG_OUTPUT,
		AB_OUTPUT,
		CD_OUTPUT,
		TRIGB_OUTPUT,
		AB2_OUTPUT,
		CD2_OUTPUT,
		NUM_OUTPUTS
	};

	enum LighIds
	{
		ENUMS(LIGHT_S1, 4),
		ENUMS(LIGHT_S2, 4),
		NUM_LIGHTS
	};


	int clock1Count = 0;
	int clock2Count = 0;
	int clock3Count = 0;
	int clock4Count = 0;

	int clock1bCount = 0;
	int clock2bCount = 0;
	int clock3bCount = 0;
	int clock4bCount = 0;

	int divider1 = 0;
	int divider2 = 0;
	int divider3 = 0;
	int divider4 = 0;

	int divider1b = 0;
	int divider2b = 0;
	int divider3b = 0;
	int divider4b = 0;

		dsp::PulseGenerator clk1;
  	dsp::PulseGenerator clk2;
  	dsp::PulseGenerator clk3;
  	dsp::PulseGenerator clk4;

  	dsp::PulseGenerator clk1b;
  	dsp::PulseGenerator clk2b;
  	dsp::PulseGenerator clk3b;
  	dsp::PulseGenerator clk4b;


	bool pulse1 = false;
	bool pulse2 = false;
	bool pulse3 = false;
	bool pulse4 = false;

	bool pulse1b = false;
	bool pulse2b = false;
	bool pulse3b = false;
	bool pulse4b = false;

	dsp::SchmittTrigger clk;
	dsp::SchmittTrigger clkb;

  Divider() {
	  config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

	  for (int i = 0; i < 4; i++)
	  {
		  configParam(DIVISION_PARAM + i,  1, 15, 1.0, "Division");
		  configParam(ON_SWITCH + i,  0.0, 1.0, 0.0, "On/Off");

		  configParam(DIVISIONB_PARAM + i,  1, 15, 1.0, "Division B");
		  configParam(ON_SWITCHB + i,  0.0, 1.0, 0.0 ,"On/Off B");
	  }

	  configParam(MODE_PARAM,  0.0, 1.0, 0.0,"MODE A");
	  configParam(MODE_PARAM + 1,  0.0, 1.0, 0.0, "MODE B");

	}


  void process(const ProcessArgs &args) override 
 {

		divider1 = round(params[DIVISION_PARAM].getValue()   + clamp(inputs[SUB1_INPUT+0].getVoltage(), -15.0f, 15.0f));
		if (divider1>15) divider1=15; 
		if (divider1<=1) divider1=1;
		divider2 = round(params[DIVISION_PARAM+1].getValue() + clamp(inputs[SUB1_INPUT+1].getVoltage(), -15.0f, 15.0f));
		if (divider2>15) divider2=15; 
		if (divider2<=1) divider2=1;
		divider3 = round(params[DIVISION_PARAM+2].getValue() + clamp(inputs[SUB1_INPUT+2].getVoltage(), -15.0f, 15.0f));
		if (divider3>15) divider3=15; 
		if (divider3<=1) divider3=1;
		divider4 = round(params[DIVISION_PARAM+3].getValue() + clamp(inputs[SUB1_INPUT+3].getVoltage(), -15.0f, 15.0f));
		if (divider4>15) divider4=15; 
		if (divider4<=1) divider4=1;

		divider1b = round(params[DIVISIONB_PARAM].getValue()   + clamp(inputs[SUB2_INPUT+0].getVoltage(), -15.0f, 15.0f));
		if (divider1b>15) divider1b=15; 
		if (divider1b<=1) divider1b=1;
		divider2b = round(params[DIVISIONB_PARAM+1].getValue() + clamp(inputs[SUB2_INPUT+1].getVoltage(), -15.0f, 15.0f));
		if (divider2b>15) divider2b=15; 
		if (divider2b<=1) divider2b=1;
		divider3b = round(params[DIVISIONB_PARAM+2].getValue() + clamp(inputs[SUB2_INPUT+2].getVoltage(), -15.0f, 15.0f));
		if (divider3b>15) divider3b=15; 
		if (divider3b<=1) divider3b=1;
		divider4b = round(params[DIVISIONB_PARAM+3].getValue() + clamp(inputs[SUB2_INPUT+3].getVoltage(), -15.0f, 15.0f));
		if (divider4b>15) divider4b=15; 
		if (divider4b<=1) divider4b=1;


if (clk.process(inputs[CLOCK_INPUT].getVoltage()))
 {
		clock1Count++;
		clock2Count++;		
		clock3Count++;		
		clock4Count++;					
 }

 if (clkb.process(inputs[CLOCKB_INPUT].getVoltage()))
 {
		clock1bCount++;
		clock2bCount++;		
		clock3bCount++;		
		clock4bCount++;					
 }



lights[LIGHT_S1+0].setSmoothBrightness(clock1Count == 0? 1.f : 0.0, args.sampleTime);
lights[LIGHT_S1+1].setSmoothBrightness(clock2Count == 0? 1.f : 0.0, args.sampleTime);
lights[LIGHT_S1+2].setSmoothBrightness(clock3Count == 0? 1.f : 0.0, args.sampleTime);
lights[LIGHT_S1+3].setSmoothBrightness(clock4Count == 0? 1.f : 0.0, args.sampleTime);

lights[LIGHT_S2+0].setSmoothBrightness(clock1bCount == 0? 1.f : 0.0, args.sampleTime);
lights[LIGHT_S2+1].setSmoothBrightness(clock2bCount == 0? 1.f : 0.0, args.sampleTime);
lights[LIGHT_S2+2].setSmoothBrightness(clock3bCount == 0? 1.f : 0.0, args.sampleTime);
lights[LIGHT_S2+3].setSmoothBrightness(clock4bCount == 0? 1.f : 0.0, args.sampleTime);
	
	/////////////////////////////////////////////////////////////////

if(params[ON_SWITCH+0].getValue())
{
	if (clock1Count >= divider1)
	{
		clock1Count = 0;		
	  clk1.trigger(1e-3);  
	}
}
if(params[ON_SWITCH+1].getValue())
{
	if (clock2Count >= divider2)
	{
		clock2Count = 0;		
	  clk2.trigger(1e-3);
  }
}
if(params[ON_SWITCH+2].getValue())
{
	if (clock3Count >= divider3)
	{
		clock3Count = 0;		
	  clk3.trigger(1e-3);
  }
}
if(params[ON_SWITCH+3].getValue())
{
	if (clock4Count >= divider4)
	{
		clock4Count = 0;		
	  clk4.trigger(1e-3);
  }
} 	


if(params[ON_SWITCHB+0].getValue())
{
	if (clock1bCount >= divider1b)
	{
		clock1bCount = 0;		
	  clk1b.trigger(1e-3);  
	}
}
if(params[ON_SWITCHB+1].getValue())
{
	if (clock2bCount >= divider2b)
	{
		clock2bCount = 0;		
	  clk2b.trigger(1e-3);
  }
}
if(params[ON_SWITCHB+2].getValue())
{
	if (clock3bCount >= divider3b)
	{
		clock3bCount = 0;		
	  clk3b.trigger(1e-3);
  }
}
if(params[ON_SWITCHB+3].getValue())
{
	if (clock4bCount >= divider4b)
	{
		clock4bCount = 0;		
	  clk4b.trigger(1e-3);
  }
} 	

//////////////////////////////////////////////////////////////////
pulse1 = clk1.process(1.0f / APP->engine->getSampleTime());
pulse2 = clk2.process(1.0f / APP->engine->getSampleTime());
pulse3 = clk3.process(1.0f / APP->engine->getSampleTime());
pulse4 = clk4.process(1.0f / APP->engine->getSampleTime());

pulse1b = clk1b.process(1.0f / APP->engine->getSampleTime());
pulse2b = clk2b.process(1.0f / APP->engine->getSampleTime());
pulse3b = clk3b.process(1.0f / APP->engine->getSampleTime());
pulse4b = clk4b.process(1.0f / APP->engine->getSampleTime());

//////////////////////////////////////////////////////////////////
if(params[MODE_PARAM].getValue())
{
outputs[TRIG_OUTPUT].setVoltage((((pulse1||pulse2)||pulse3)||pulse4)? 10.0f : 0.0f);
outputs[AB_OUTPUT].setVoltage((pulse1 || pulse2) ? 10.0f : 0.0f);
outputs[CD_OUTPUT].setVoltage((pulse3 || pulse4) ? 10.0f : 0.0f);
}
else
{
bool xora,xorb = false; 
xora = pulse1==pulse2;
xorb = pulse3==pulse4;

outputs[TRIG_OUTPUT].setVoltage(xora == xorb ? 0.0f : 10.0f);
  outputs[AB_OUTPUT].setVoltage(xora ? 0.0f : 10.0f);
  outputs[CD_OUTPUT].setVoltage(xorb ? 0.0f : 10.0f);
}

if(params[MODE_PARAM+1].getValue())
{
outputs[TRIGB_OUTPUT].setVoltage((((pulse1b||pulse2b)||pulse3b)||pulse4b)? 10.0f : 0.0f);
  outputs[AB2_OUTPUT].setVoltage((pulse1b || pulse2b) ? 10.0f : 0.0f);
  outputs[CD2_OUTPUT].setVoltage((pulse3b || pulse4b) ? 10.0f : 0.0f);
}
else
{
	bool xora2, xorb2 = false;
	xora2 = pulse1b == pulse2b;
	xorb2 = pulse3b == pulse4b;

	outputs[TRIGB_OUTPUT].setVoltage(xora2 == xorb2 ? 0.0f : 10.0f);
	  outputs[AB2_OUTPUT].setVoltage(xora2 ? 0.0f : 10.0f);
	  outputs[CD2_OUTPUT].setVoltage(xorb2 ? 0.0f : 10.0f);
}
}

};

struct DividerWidget : ModuleWidget {
DividerWidget(Divider *module){
	setModule(module);
	setPanel(APP->window->loadSvg(asset::plugin(pluginInstance,  "res/Divider.svg")));

//Screw
  addChild(createWidget<ScrewBlack>(Vec(15, 0)));
  addChild(createWidget<ScrewBlack>(Vec(box.size.x-30, 0)));
  addChild(createWidget<ScrewBlack>(Vec(15, 365)));
  addChild(createWidget<ScrewBlack>(Vec(box.size.x-30, 365)));
    
   int knob=35;
   int jack = 27;
   int si = 15;   

   //
   for (int i = 0; i < 4; i++)
   {
	   addParam(createParam<SDKnob>(Vec(si + 70, 20 + knob * i), module, Divider::DIVISION_PARAM + i));
	   addParam(createParam<SilverSwitch>(Vec(si + 10, 20 + knob * i), module, Divider::ON_SWITCH + i));

	   addParam(createParam<SDKnob>(Vec(si + 70, 170 + knob * i), module, Divider::DIVISIONB_PARAM + i));
	   addParam(createParam<SilverSwitch>(Vec(si + 10, 170 + knob * i), module, Divider::ON_SWITCHB + i));

	   addChild(createLight<SmallLight<RedLight>>(Vec(si + 105, 30 + knob * i), module, Divider::LIGHT_S1 + i));
	   addChild(createLight<SmallLight<RedLight>>(Vec(si + 105, 180 + knob * i), module, Divider::LIGHT_S2 + i));

	}
	
	addInput(createInput<PJ301MVAPort>(Vec(si + 40, 22.5 + knob * 0), module, Divider::SUB1_INPUT + 0));
	addInput(createInput<PJ301MVAPort>(Vec(si + 40, 22.5 + knob * 1), module, Divider::SUB1_INPUT + 1));
	addInput(createInput<PJ301MVAPort>(Vec(si + 40, 22.5 + knob * 2), module, Divider::SUB1_INPUT + 2));
	addInput(createInput<PJ301MVAPort>(Vec(si + 40, 22.5 + knob * 3), module, Divider::SUB1_INPUT + 3));

	addInput(createInput<PJ301MVAPort>(Vec(si + 40, 173.5 + knob * 0), module, Divider::SUB2_INPUT + 0));
	addInput(createInput<PJ301MVAPort>(Vec(si + 40, 173.5 + knob * 1), module, Divider::SUB2_INPUT + 1));
	addInput(createInput<PJ301MVAPort>(Vec(si + 40, 173.5 + knob * 2), module, Divider::SUB2_INPUT + 2));
	addInput(createInput<PJ301MVAPort>(Vec(si + 40, 173.5 + knob * 3), module, Divider::SUB2_INPUT + 3));



addInput(createInput<PJ301MVAPort>(Vec(15, 310), module, Divider::CLOCK_INPUT));
addOutput(createOutput<PJ301MVAPort>(Vec(15 + jack * 1, 310), module, Divider::AB_OUTPUT));
addOutput(createOutput<PJ301MVAPort>(Vec(15 + jack * 2, 310), module, Divider::CD_OUTPUT));
addOutput(createOutput<PJ301MVAPort>(Vec(15 + jack * 3, 310), module, Divider::TRIG_OUTPUT));

addParam(createParam<MCKSSS2>(Vec(15 + jack * 4, 313), module, Divider::MODE_PARAM + 0));

addInput(createInput<PJ301MVAPort>(Vec(15, 310 + jack), module, Divider::CLOCKB_INPUT));
addOutput(createOutput<PJ301MVAPort>(Vec(15 + jack * 1, 310 + jack), module, Divider::AB2_OUTPUT));
addOutput(createOutput<PJ301MVAPort>(Vec(15 + jack * 2, 310 + jack), module, Divider::CD2_OUTPUT));
addOutput(createOutput<PJ301MVAPort>(Vec(15 + jack * 3, 310 + jack), module, Divider::TRIGB_OUTPUT));

addParam(createParam<MCKSSS2>(Vec(15 + jack * 4, 313 + jack), module, Divider::MODE_PARAM + 1));
}
};

Model *modelDivider = createModel<Divider, DividerWidget>("Divider");
