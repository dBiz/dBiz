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
		ENUMS(OUT_SEL, 8),
		ENUMS(BUTTONS, 8),
		ENUMS(GAIN, 8),
		NUM_PARAMS 
	};
	enum InputId {

		CLK_IN,
		RESET_IN,
		ENUMS(CH_IN, 8),
		NUM_INPUTS
	};
	enum OutputId
	{

		ENUMS(CH_OUT, 3),
		NUM_OUTPUTS
	};
	enum LightId
	{
		ENUMS(MODE_LIGHT, 3),
		RESET_LIGHT,
		RUNNING_LIGHT,
		ENUMS(BUTTON_LIGHT, 8),
		ENUMS(SLIGHT, 8),
		NUM_LIGHTS
	};

	float phase = 0.f;
	bool state[8];
	int index = 0;
	bool running = true;
	int mode=0;

	dsp::SchmittTrigger muteTrigger[8];
	dsp::SchmittTrigger clkTrigger;
	dsp::SchmittTrigger runningTrigger;
	dsp::SchmittTrigger resetTrigger;
	dsp::SchmittTrigger modeTrigger;

	Smixer() {
		// Configure the module
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

		// Configure parameters
		// See engine/Param.hpp for config() arguments
		onReset();
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
			json_t *stateJ = json_boolean(state[i]);
			json_array_append_new(statesJ, stateJ);
		}
		json_object_set_new(rootJ, "states", statesJ);

		// running
		json_object_set_new(rootJ, "running", json_boolean(running));

		json_t *modeJ = json_integer((int)mode);
		json_object_set_new(rootJ, "mode", modeJ);

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
					state[i] = json_boolean_value(stateJ);
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
		float ChIn[8]={};
		



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
					if(mode==0)
					setIndex(index + 1);
					if(mode==1)
					setIndex(index - 1);
					if(mode==2)
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
					if(mode==0)
					setIndex(index + 1);
					if(mode==1)
					setIndex(index - 1);
					if(mode==2)
					setIndex(std::round(rand() % 7));
				}
			}
		}


		if (modeTrigger.process(params[MODE_PARAM].getValue()))
		 {
			 mode++;
			 if (mode > 2)
				 mode = 0;
			 for (int i = 0; i < 3; i++)
			 {
				 lights[MODE_LIGHT + i].setBrightness(0.0);
			 }
		 }

		 lights[MODE_LIGHT + mode].setBrightness(1.0);
	 
		
	

		for (int i = 0; i < 8; i++)
		{
			ChIn[i]=inputs[CH_IN+i].getVoltage()*params[GAIN+i].getValue();

			if (muteTrigger[i].process(params[BUTTONS + i].getValue()))
			{
			state[i] ^= true;
			lights[BUTTON_LIGHT + i].setBrightness(!state[i] ? 0.9f : 0.f );
			}

			lights[SLIGHT + i].setSmoothBrightness(i == index ? 1.0 : 0.0 ,APP->engine->getSampleTime()); 

			if (!state[i]) ChIn[i] = 0.f;

			if (running)
			{

			  ///////////////////////////////////



			  if (i < std::round(params[SI_PARAM].getValue()) || i > std::round(params[NS_PARAM].getValue()))
			  {
			  	if (params[OUT_SEL + i].getValue() == 0)
			  		outA += ChIn[i];
			  	if (params[OUT_SEL + i].getValue() == 1)
			  		outB += ChIn[i];
			  	if (params[OUT_SEL + i].getValue() == 2)
			  		outC += ChIn[i];
			  	lights[SLIGHT + i].setSmoothBrightness(ChIn[i],APP->engine->getSampleTime());
			  }


			  //////////////////////////////////////
			  if (params[OUT_SEL + i].getValue() == 0)
			  {
			  	outA += ChIn[i] * clamp(lights[SLIGHT + i].value * 10, 0.0, 1.0);
			  }
			  if (params[OUT_SEL + i].getValue() == 1)
			  {
			  	outB += ChIn[i] * clamp(lights[SLIGHT + i].value * 10, 0.0, 1.0);
			  }
			  if (params[OUT_SEL + i].getValue() == 2)
			  {
			  	outC += ChIn[i] * clamp(lights[SLIGHT + i].value * 10, 0.0, 1.0);
			  }
			}

			else
			{
				if (params[OUT_SEL + i].getValue() == 0)
					outA += ChIn[i];
				if (params[OUT_SEL + i].getValue() == 1)
					outB += ChIn[i];
				if (params[OUT_SEL + i].getValue() == 2)
					outC += ChIn[i];
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
		lights[RESET_LIGHT].setSmoothBrightness(resetTrigger.isHigh(),APP->engine->getSampleTime());

		    outputs[CH_OUT].setVoltage(outA * params[BUS_VOL].getValue());
		outputs[CH_OUT + 1].setVoltage(outB * params[BUS_VOL + 1].getValue());
		outputs[CH_OUT + 2].setVoltage(outC * params[BUS_VOL + 2].getValue());
	}


};

struct SmixerWidget : ModuleWidget {
	SmixerWidget(Smixer *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Smixer.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		int up= 20;

		for (int i = 0; i < 3; i++)
		{
		addOutput(createOutput<PJ301MBPort>(Vec(10.5 + i * 105, 310), module, Smixer::CH_OUT + i));
		addParam(createParam<Trimpot>(Vec(12.5 + i * 105, 340), module, Smixer::BUS_VOL + i));
		}

		addInput(createInput<PJ301MLPort>(Vec(10.5 + 0 * 30, 120), module, Smixer::CH_IN + 0));
		addInput(createInput<PJ301MLPort>(Vec(10.5 + 1 * 30, 120), module, Smixer::CH_IN + 1));
		addInput(createInput<PJ301MLPort>(Vec(10.5 + 2 * 30, 120), module, Smixer::CH_IN + 2));
		addInput(createInput<PJ301MLPort>(Vec(10.5 + 3 * 30, 120), module, Smixer::CH_IN + 3));
		addInput(createInput<PJ301MLPort>(Vec(10.5 + 4 * 30, 120), module, Smixer::CH_IN + 4));
		addInput(createInput<PJ301MLPort>(Vec(10.5 + 5 * 30, 120), module, Smixer::CH_IN + 5));
		addInput(createInput<PJ301MLPort>(Vec(10.5 + 6 * 30, 120), module, Smixer::CH_IN + 6));
		addInput(createInput<PJ301MLPort>(Vec(10.5 + 7 * 30, 120), module, Smixer::CH_IN + 7));

		for (int i = 0; i < 8; i++)
		{
			addParam(createParam<LEDSliderBlue>(Vec(13 + i * 30, 20), module, Smixer::GAIN + i));
            addChild(createLight<MediumLight<BlueLight>>(Vec(18 + i * 30, 105), module, Smixer::SLIGHT + i));

            addParam(createParam<BPush>(Vec(12.5 + i * 30, 152.5), module, Smixer::BUTTONS + i));
            addChild(createLight<BigLight<BlueLight>>(Vec(15 + i * 30, 155), module, Smixer::BUTTON_LIGHT + i));
            addParam(createParam<MCKSSS>(Vec(20 + i * 30, 185), module, Smixer::OUT_SEL + i));
        }

			addInput(createInput<PJ301MCPort>(Vec(10.5 , 220), module, Smixer::CLK_IN));
			addInput(createInput<PJ301MCPort>(Vec(10.5 + 90, 220), module, Smixer::RESET_IN));
	
			addParam(createParam<Trimpot>(Vec(40, 222.5), module, Smixer::CLOCK_PARAM));

            addParam(createParam<RoundWhySnapKnob>(Vec(60, 280 - up), module, Smixer::SI_PARAM));
   

            addParam(createParam<RoundWhySnapKnob>(Vec(160, 280 - up), module, Smixer::NS_PARAM));
    

            addParam(createParam<BPush>(Vec(65, 222), module, Smixer::RUN_PARAM));
            addChild(createLight<BigLight<BlueLight>>(Vec(67.5, 224.5), module, Smixer::RUNNING_LIGHT));

            addParam(createParam<BPush>(Vec(130, 222), module, Smixer::RESET_PARAM));
            addChild(createLight<BigLight<BlueLight>>(Vec(132.5, 224.5), module, Smixer::RESET_LIGHT));

						addParam(createParam<BPush>(Vec(165, 222), module, Smixer::MODE_PARAM));

						for(int i=0;i<3;i++){
						addChild(createLight<MediumLight<BlueLight>>(Vec(192 + i*20, 230), module, Smixer::MODE_LIGHT + i));
						}

    }
};


// Define the Model with the Module type, ModuleWidget type, and module slug
Model *modelSmixer = createModel<Smixer, SmixerWidget>("Smixer");
