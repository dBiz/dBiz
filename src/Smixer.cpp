////////////////////////////////////////////////////////////////////////////
// <Channel sequencer, mixer>
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

struct Smixer : Module {
	enum ParamId
	{
		MODE_PARAM,
		RESET_PARAM,
		RUN_PARAM,
		SI_PARAM,
		NS_PARAM,
		CLOCK_PARAM,
		ENUMS(BUS_VOL, 3),
		ENUMS(GATE_PARAM, 8),
		ENUMS(OUT_SEL, 8),
		ENUMS(BUTTONS, 8),
		ENUMS(GAIN, 8),
		SLOPE_PARAM,
		NUM_PARAMS
	};
	enum InputId {

		CLK_IN,
		RESET_IN,
		ENUMS(CH_IN, 8),
		ENUMS(CH_CV_IN, 8),
		NUM_INPUTS
	};
	enum OutputId
	{

		ENUMS(CH_OUT, 3),
		NUM_OUTPUTS
	};
	enum LightId
	{
		ENUMS(MODE_LIGHT, 4),
		RESET_LIGHT,
		RUNNING_LIGHT,
		ENUMS(BUTTON_LIGHT, 8),
		ENUMS(SLIGHT, 8),
		ENUMS(LVL_LIGHTS, 8),
		NUM_LIGHTS
	};

	float phase = 0.f;
	bool state[8];
	int index = 0;
	bool running = true;
	bool verse = true;
	int mode=0;


	dsp::SchmittTrigger muteTrigger[8];
	dsp::SchmittTrigger clkTrigger;
	dsp::SchmittTrigger runningTrigger;
	dsp::SchmittTrigger resetTrigger;
	dsp::SchmittTrigger modeTrigger;
	dsp::VuMeter2 chMeters[8];
	dsp::ClockDivider lightDivider;

	int panelTheme;

	Smixer() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		for(int i=0;i<8;i++)
		{
			configParam(GAIN+i,  0.f,1.f,0.f,"Gain");
			configParam(BUTTONS+i,  0.0,1.0,0.0,"Buttons");
			configParam(OUT_SEL+i,  0.0,2.0,0.0,"Output Selector");

		}
		for (int i = 0; i < 3; i++)
		{
		  configParam(BUS_VOL + i,  0.0, 1.0, 0.0, "Output Vol");
		}

		configParam(CLOCK_PARAM,  -2.0f, 6.0f, 2.0f,"Clock");
		configParam(SI_PARAM,  0.0,7.0,0.0,"Start Index");
		configParam(NS_PARAM,  0.0,7.0,7.0,"Num Step");
		configParam(RUN_PARAM,  0.0f, 1.0f, 0.0f,"Running");
		configParam(RESET_PARAM,  0.0f, 1.0f, 0.0f,"Reset");
		configParam(MODE_PARAM,  0.0f, 1.0f, 0.0f,"Mode");
		configParam(SLOPE_PARAM,  0.0f, 1.0f, 0.0f,"Decay time");
		onReset();
		panelTheme = (loadDarkAsDefault() ? 1 : 0);
	}

	void onReset() override
	{
		for (int i = 0; i < 8; i++)
		{
			state[i] = true;
		}
	}
	void onRandomize() override
	{
		for (int i = 0; i < 8; i++)
		{
			state[i] = (random::uniform() < 0.5f);
		}
	}

	json_t *dataToJson() override
	{
		json_t *rootJ = json_object();

		// states
		json_t *statesJ = json_array();
		for (int i = 0; i < 8; i++)
		{
			json_array_insert_new(statesJ,i,json_boolean((bool)state[i]));
		}
		json_object_set_new(rootJ, "states", statesJ);

		// running
		json_object_set_new(rootJ, "running", json_boolean(running));

		json_t *modeJ = json_integer((int)mode);
		json_object_set_new(rootJ, "mode", modeJ);

		json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));

		return rootJ;
	}

	void dataFromJson(json_t *rootJ) override
	{

		// states
		json_t *statesJ = json_object_get(rootJ, "states");
		if (statesJ)
		{
			for (int i = 0; i < 8; i++)
			{
				json_t *stateJ = json_array_get(statesJ, i);
				if (stateJ)
					state[i] = !!json_boolean_value(stateJ);
			}
		}

		// running
		json_t *runningJ = json_object_get(rootJ, "running");
		if (runningJ)
			running = json_is_true(runningJ);

		json_t *modeJ = json_object_get(rootJ, "mode");
		if (modeJ)
		{
			mode = json_integer_value(modeJ);
		}
		json_t *panelThemeJ = json_object_get(rootJ, "panelTheme");
		if (panelThemeJ)
			panelTheme = json_integer_value(panelThemeJ);

	}

	void setIndex(int index)
	{
		int max = 8;

		int start= std::round(params[SI_PARAM].getValue());
		int step = std::round(params[NS_PARAM].getValue())+1;

		phase = 0.f;

		this->index = index;

		if (this->index >= step)
			this->index = start;
		if (this->index >= max)
			this->index = start;

		if (this->index < start)
			this->index = step-1;
		//if (this->index <= start)
		//	this->index = step;

	}


void process(const ProcessArgs &args) override {

		float outA = 0.f;
		float outB = 0.f;
		float outC = 0.f;
		float step[8] = {};
		float ChIn[8] = {};

		if (index==std::round(params[NS_PARAM].getValue())) verse=false;
		if (index==std::round(params[SI_PARAM].getValue())) verse=true;


		if (runningTrigger.process(params[RUN_PARAM].getValue()))
		{
			running = !running;
		}

		if (running)
		{
			if (inputs[CLK_IN].isConnected())
			{
				// External clock
				if (clkTrigger.process(inputs[CLK_IN].getVoltage()))
				{
					// clkP.trigger(1e-3);
					if(mode==0)
					setIndex(index + 1);
					if(mode==1)
					setIndex(index - 1);
					if(mode==2)
					{
						if(verse) setIndex(index + 1);
						else setIndex(index - 1);
					}
					if(mode==3)
					setIndex(std::round(rand() % 7));
				}
		}
			else
			{
				// Internal clock
				float clockTime = std::pow(2.0f, params[CLOCK_PARAM].getValue() + inputs[CLK_IN].getVoltage());
				phase += clockTime * APP->engine->getSampleTime();
				if (phase >= 1.0f)
				{
					// clkP.trigger(1e-3);
					if(mode==0)
					setIndex(index + 1);
					if(mode==1)
					setIndex(index - 1);
					if(mode==2)
					{
						if(verse) setIndex(index + 1);
						else setIndex(index - 1);
					}
					if(mode==3)
					setIndex(std::round(rand() % 7));
				}
			}
		}

		if (modeTrigger.process(params[MODE_PARAM].getValue()))
		 {
			 mode++;
			 if (mode > 3)
				 mode = 0;
			 for (int i = 0; i < 4; i++)
			 {
				 lights[MODE_LIGHT + i].setBrightness(0.0);
			 }
		 }

		 lights[MODE_LIGHT + mode].setBrightness(1.0);


		for (int i = 0; i < 8; i++)
		{
			lights[BUTTON_LIGHT + i].setBrightness(!state[i] ? 0.9f : 0.f );
			ChIn[i]=inputs[CH_IN+i].getVoltage()*std::pow(params[GAIN+i].getValue(),2.f);
			if(inputs[CH_CV_IN+i].isConnected())
			ChIn[i]*=(inputs[CH_CV_IN+i].getVoltage()/10.f);
			if (muteTrigger[i].process(params[BUTTONS + i].getValue()))
			{
				state[i] ^= true;
			}
			lights[SLIGHT + i].setSmoothBrightness(i == index ? 1.0 : 0.0 , args.sampleTime);
			step[i]=lights[SLIGHT + i].getBrightness();
			if (!state[i]) ChIn[i] = 0.f;
			//////////////////////////////////////////////////
			if (running)
			{
			  ///////////////////////////////////

			  if (i < std::round(params[SI_PARAM].getValue()) || i > std::round(params[NS_PARAM].getValue()))
			  {
			  	if (params[OUT_SEL + i].getValue() == 0)
			  		{
							outA += ChIn[i];
						}
			  	if (params[OUT_SEL + i].getValue() == 1)
			  		{
							outB += ChIn[i];
						}
			  	if (params[OUT_SEL + i].getValue() == 2)
			  		{
							outC += ChIn[i];
						}

						chMeters[i].process(args.sampleTime, ChIn[i] / 5.f);
			  }
			  //////////////////////////////////////
			  if (params[OUT_SEL + i].getValue() == 0)
			  {
			  	outA += ChIn[i] * step[i];
			  }
			  if (params[OUT_SEL + i].getValue() == 1)
			  {
			  	outB += ChIn[i] * step[i];
			  }
			  if (params[OUT_SEL + i].getValue() == 2)
			  {
			  	outC += ChIn[i] * step[i];
			  }
				  chMeters[i].process(args.sampleTime, step[i]*ChIn[i] / 5.f);
			 }
			else
			{
				if (params[OUT_SEL + i].getValue() == 0)
				{
					 outA += ChIn[i];
				}
				if (params[OUT_SEL + i].getValue() == 1)
				{
					 	outB += ChIn[i];
				}
				if (params[OUT_SEL + i].getValue() == 2)
				{
					 	outC += ChIn[i];
				}
				chMeters[i].process(args.sampleTime, ChIn[i] / 5.f);
			}
		}

		if (resetTrigger.process(params[RESET_PARAM].getValue() + inputs[RESET_IN].getVoltage()))
		{
			if(mode==0)
			setIndex(std::round(params[SI_PARAM].getValue()));
			if(mode ==1)
			setIndex(std::round(params[NS_PARAM].getValue()));
			if(mode==2)
			setIndex(std::round(rand() % 7));

		}

		lights[RUNNING_LIGHT].setBrightness(running);
		lights[RESET_LIGHT].setSmoothBrightness(resetTrigger.isHigh(),args.sampleTime);

		//outputs[GATE_OUTPUT].setVoltage(pulse ? 10.f : 0.f );

		outputs[CH_OUT].setVoltage(outA * params[BUS_VOL].getValue());
		outputs[CH_OUT + 1].setVoltage(outB * params[BUS_VOL + 1].getValue());
		outputs[CH_OUT + 2].setVoltage(outC * params[BUS_VOL + 2].getValue());

		if (lightDivider.process()) {
			for (int i = 0; i < 8; i++) {
				float b = chMeters[i].getBrightness(-24.f, 0.f);
				lights[LVL_LIGHTS + i].setBrightness(b);
			}
		}

	}


};

struct SmixerWidget : ModuleWidget {


	SvgPanel* darkPanel;
	struct PanelThemeItem : MenuItem {
	  Smixer *module;
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

	  Smixer *module = dynamic_cast<Smixer*>(this->module);
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
	SmixerWidget(Smixer *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Light/Smixer.svg")));
		if (module) {
	    darkPanel = new SvgPanel();
	    darkPanel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Dark/Smixer.svg")));
	    darkPanel->visible = false;
	    addChild(darkPanel);
	  }

		int down =30;
		/////////////////////////////////////
		SegmentDisplay* segmentDisplay = createWidget<SegmentDisplay>(Vec(192, 252));
		segmentDisplay->box.size = Vec(50, 20);
		segmentDisplay->setLights<BlueLight>(module, Smixer::MODE_LIGHT,4);
		addChild(segmentDisplay);



		//////////////////

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));



		for (int i = 0; i < 3; i++)
		{
		addOutput(createOutput<PJ301MBPort>(Vec(10.5 + i * 105, 310), module, Smixer::CH_OUT + i));
		addParam(createParam<Trim>(Vec(15 + i * 105, 340), module, Smixer::BUS_VOL + i));
		}

		addInput(createInput<PJ301MLPort>(Vec(10.5 + 0 * 30, 120), module, Smixer::CH_IN + 0));
		addInput(createInput<PJ301MLPort>(Vec(10.5 + 1 * 30, 120), module, Smixer::CH_IN + 1));
		addInput(createInput<PJ301MLPort>(Vec(10.5 + 2 * 30, 120), module, Smixer::CH_IN + 2));
		addInput(createInput<PJ301MLPort>(Vec(10.5 + 3 * 30, 120), module, Smixer::CH_IN + 3));
		addInput(createInput<PJ301MLPort>(Vec(10.5 + 4 * 30, 120), module, Smixer::CH_IN + 4));
		addInput(createInput<PJ301MLPort>(Vec(10.5 + 5 * 30, 120), module, Smixer::CH_IN + 5));
		addInput(createInput<PJ301MLPort>(Vec(10.5 + 6 * 30, 120), module, Smixer::CH_IN + 6));
		addInput(createInput<PJ301MLPort>(Vec(10.5 + 7 * 30, 120), module, Smixer::CH_IN + 7));

		//addOutput(createOutput<PJ301MBPort>(Vec(30, 215), module, Smixer::GATE_OUTPUT));



			addParam(createLightParam<LEDLightSliderFixed<BlueLight>>(Vec(13 + 0 * 30, 20), module, Smixer::GAIN + 0, Smixer::LVL_LIGHTS + 0));
			addParam(createLightParam<LEDLightSliderFixed<BlueLight>>(Vec(13 + 1 * 30, 20), module, Smixer::GAIN + 1, Smixer::LVL_LIGHTS + 1));
			addParam(createLightParam<LEDLightSliderFixed<BlueLight>>(Vec(13 + 2 * 30, 20), module, Smixer::GAIN + 2, Smixer::LVL_LIGHTS + 2));
			addParam(createLightParam<LEDLightSliderFixed<BlueLight>>(Vec(13 + 3 * 30, 20), module, Smixer::GAIN + 3, Smixer::LVL_LIGHTS + 3));
			addParam(createLightParam<LEDLightSliderFixed<BlueLight>>(Vec(13 + 4 * 30, 20), module, Smixer::GAIN + 4, Smixer::LVL_LIGHTS + 4));
			addParam(createLightParam<LEDLightSliderFixed<BlueLight>>(Vec(13 + 5 * 30, 20), module, Smixer::GAIN + 5, Smixer::LVL_LIGHTS + 5));
			addParam(createLightParam<LEDLightSliderFixed<BlueLight>>(Vec(13 + 6 * 30, 20), module, Smixer::GAIN + 6, Smixer::LVL_LIGHTS + 6));
			addParam(createLightParam<LEDLightSliderFixed<BlueLight>>(Vec(13 + 7 * 30, 20), module, Smixer::GAIN + 7, Smixer::LVL_LIGHTS + 7));

			for (int i = 0; i < 8; i++)
			{
				addInput(createInput<PJ301MCPort>(Vec(10.5 + i * 30, 217), module, Smixer::CH_CV_IN+i));

      	addChild(createLight<MediumLight<BlueLight>>(Vec(18 + i * 30, 105), module, Smixer::SLIGHT + i));

		addParam(createLightParam<LEDLightBezel<BlueLight>>(Vec(14.5 + i * 30, 154), module, Smixer::BUTTONS + i, Smixer::BUTTON_LIGHT + i));

      	addParam(createParam<MCKSSS>(Vec(20 + i * 30, 188), module, Smixer::OUT_SEL + i));
    	}

			addInput(createInput<PJ301MCPort>(Vec(10.5 , 220+down), module, Smixer::CLK_IN));
			addInput(createInput<PJ301MCPort>(Vec(10.5 + 90, 220+down), module, Smixer::RESET_IN));
			addParam(createParam<Trim>(Vec(40, 222.5+down), module, Smixer::CLOCK_PARAM));

			addParam(createLightParam<LEDLightBezel<BlueLight>>(Vec(65, 224 + down), module, Smixer::RUN_PARAM, Smixer::RUNNING_LIGHT));

			addParam(createLightParam<LEDLightBezel<BlueLight>>(Vec(130, 224 + down), module, Smixer::RESET_PARAM, Smixer::RESET_LIGHT));

			addParam(createParam<LEDBezel>(Vec(165, 224 + down), module, Smixer::MODE_PARAM));
			for(int i=0;i<4;i++){
		//	addChild(createLight<MediumLight<BlueLight>>(Vec(192 + i*13, 230+down), module, Smixer::MODE_LIGHT + i));
			}

			addParam(createParam<RoundWhySnapKnob>(Vec(60,  310), module, Smixer::SI_PARAM));
			addParam(createParam<RoundWhySnapKnob>(Vec(160, 310), module, Smixer::NS_PARAM));



    }
		void step() override {
		  if (module) {
			Widget* panel = getPanel();
		    panel->visible = ((((Smixer*)module)->panelTheme) == 0);
		    darkPanel->visible  = ((((Smixer*)module)->panelTheme) == 1);
		  }
		  Widget::step();
		}
};


// Define the Model with the Module type, ModuleWidget type, and module slug
Model *modelSmixer = createModel<Smixer, SmixerWidget>("Smixer");
