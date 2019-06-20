
#include "plugin.hpp"


struct VCA4 : Module {
    enum ParamIds
    {
        ENUMS(CV_PARAM, 16),
        ENUMS(MUTE_PARAM, 16),
        NUM_PARAMS
    };
    enum InputIds
    {
        ENUMS(CH_INPUT, 4),
        ENUMS(CV_INPUT, 16),
        NUM_INPUTS
    };
	enum OutputIds 
    {
	    ENUMS(CH_OUTPUT, 4),  
	    NUM_OUTPUTS
    };
    enum LightIds
    {
      ENUMS(MUTE_LIGHT, 16),
      NUM_LIGHTS
    };

    dsp::SchmittTrigger mute_triggers[16];
    bool mute_states[16] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

    float ch_in[4];
    float ch_out[4];
    float cv_val[16];
    

    VCA4() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        
        for(int i=0;i<16;i++)
        {
            configParam(CV_PARAM,  0.0, 1.0, 0.0,"Ch Cv");
            configParam(MUTE_PARAM,  0.0, 1.0, 0.0,"Mute Ch");
        }
    
    }
    

    json_t *dataToJson() override
    {
        json_t *rootJ = json_object();

        // mute states
        json_t *mute_statesJ = json_array();
        for (int i = 0; i < 16; i++)
        {
            json_t *mute_stateJ = json_integer((int)mute_states[i]);
            json_array_append_new(mute_statesJ, mute_stateJ);
        }
        json_object_set_new(rootJ, "mutes", mute_statesJ);
        return rootJ;
    }

    void dataFromJson(json_t *rootJ) override
    {
        // mute states
        json_t *mute_statesJ = json_object_get(rootJ, "mutes");
        if (mute_statesJ)
        {
            for (int i = 0; i < 16; i++)
            {
                json_t *mute_stateJ = json_array_get(mute_statesJ, i);
                if (mute_stateJ)
                    mute_states[i] = !!json_integer_value(mute_stateJ);
            }
        }
    }

    void process(const ProcessArgs &args) override  {

    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            if (mute_triggers[i + j * 4].process(params[MUTE_PARAM + i + j * 4].value))
            {
                mute_states[i + j * 4] = !mute_states[+i + j * 4];
            }
            lights[MUTE_LIGHT + i + j * 4].value = mute_states[i + j * 4] ? 1.0 : 0.0;
        }
    }

    for (int i = 0; i < 4; i++)
    {
        ch_in[i] = inputs[CH_INPUT + i].value;
    }

    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            if (!mute_states[i + j * 4])
            {
                cv_val[i + j * 4] = 0.0;
            }
            else
            {
                cv_val[i + j * 4] = params[CV_PARAM + i + j * 4].value;
            }
                
                if(inputs[CV_INPUT+i+j*4].isConnected())
                 cv_val[i+j*4] *= (inputs[CV_INPUT + i+j*4].value / 10.0f);
        }
    }

    for (int i = 0; i < 4; i++)
    {
        outputs[CH_OUTPUT + i ].value =0.4* (ch_in[0]*cv_val[i] + ch_in[0]*cv_val[i] + ch_in[0]*cv_val[i] + ch_in[0]*cv_val[i]);
    }
    for (int i = 0; i < 4; i++)
    {
        outputs[CH_OUTPUT + i ].value +=0.4* (ch_in[1] * cv_val[i+4] + ch_in[1] * cv_val[i+4] + ch_in[1] * cv_val[i+4] + ch_in[1] * cv_val[i+4]);
    }
     for (int i = 0; i < 4; i++)
     {
         outputs[CH_OUTPUT + i].value +=0.4* (ch_in[2] * cv_val[i + 8] + ch_in[2] * cv_val[i + 8] + ch_in[2] * cv_val[i + 8] + ch_in[2] * cv_val[i + 8]);
     }
     for (int i = 0; i < 4; i++)
     {
         outputs[CH_OUTPUT + i].value +=0.4* (ch_in[3] * cv_val[i+12] + ch_in[3] * cv_val[i+12] + ch_in[3] * cv_val[i+12] + ch_in[3] * cv_val[i+12]);
     }
}

};

////////////////////////////////

struct VCA4Widget : ModuleWidget {
VCA4Widget(VCA4 *module) {
    setModule(module);
    setPanel(APP->window->loadSvg(asset::plugin(pluginInstance,  "res/VCA4.svg")));

  int top = 20;
  int left = 2;
  int column_spacing = 35; 
  int row_spacing = 30;
  int button_offset = 20;


for (int i = 0; i < 4; i++)
  {
    for ( int j = 0 ; j < 4 ; j++)
    {

        addParam(createParam<LEDButton>(Vec(button_offset + left + column_spacing * i+140, top + row_spacing * j + 170), module, VCA4::MUTE_PARAM + i + j * 4));
        addChild(createLight<BigLight<OrangeLight>>(Vec(button_offset + column_spacing * i+140, top + row_spacing * j + 170 ), module, VCA4::MUTE_LIGHT + i + j * 4));
        addParam(createParam<Trimpot>(Vec(10+column_spacing * i, top + row_spacing * j + 170), module, VCA4::CV_PARAM + i + j * 4));
    }
  }

    for (int i = 0; i < 4; i++)
     {
        addInput(createInput<PJ301MIPort>(Vec(30,24+40*i),module, VCA4::CH_INPUT + i));
     }
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            if (j == 0 || j==2 )
                addInput(createInput<PJ301MCPort>(Vec(column_spacing * 1.5 * i + 100, 60 + row_spacing * j),module, VCA4::CV_INPUT + i + j * 4));
            else
                addInput(createInput<PJ301MCPort>(Vec(column_spacing * 1.5 * i + 70, 60 + row_spacing * j),module, VCA4::CV_INPUT + i + j * 4));
        }
    }

    for (int i = 0; i < 4; i++)
    {
        addOutput(createOutput<PJ301MRPort>(Vec(70 + row_spacing *1.9* i,24), module, VCA4::CH_OUTPUT + i));
    }

    addChild(createWidget<ScrewBlack>(Vec(15, 0)));
    addChild(createWidget<ScrewBlack>(Vec(box.size.x - 30, 0)));
    addChild(createWidget<ScrewBlack>(Vec(15, 365)));
    addChild(createWidget<ScrewBlack>(Vec(box.size.x - 30, 365)));
    }
};


 Model *modelVCA4 = createModel<VCA4, VCA4Widget>("VCA4");
