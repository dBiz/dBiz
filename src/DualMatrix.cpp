////////////////////////////////////////////////////////////////////////////
// <Dual Matrix mixer with combined audio - kind of buchla dual matrix mixer
//  with CV >
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
using namespace std;

/////added fine out /////////////////////////////////////////////////

struct DualMatrix : Module {
    enum ParamIds
    {
        ENUMS(LEFT_A_PARAM, 5),
        ENUMS(LEFT_B_PARAM, 5),
        ENUMS(LEFT_C_PARAM, 5),
        ENUMS(LEFT_D_PARAM, 5),
        ENUMS(RIGHT_A_PARAM, 5),
        ENUMS(RIGHT_B_PARAM, 5),
        ENUMS(RIGHT_C_PARAM, 5),
        ENUMS(RIGHT_D_PARAM, 5),
        STEREO_A_PARAM,
        STEREO_B_PARAM,
        STEREO_C_PARAM,
        STEREO_D_PARAM,
        MUTE_A_PARAM,
        MUTE_B_PARAM,
        MUTE_C_PARAM,
        MUTE_D_PARAM,
        NUM_PARAMS
    };
    enum InputIds
    {
        ENUMS(LINEL_INPUT, 5),
        ENUMS(LINER_INPUT, 5),
        ENUMS(LEFT_A_CV_INPUT, 5),
        ENUMS(LEFT_B_CV_INPUT, 5),
        ENUMS(LEFT_C_CV_INPUT, 5),
        ENUMS(LEFT_D_CV_INPUT, 5),
        ENUMS(RIGHT_A_CV_INPUT, 5),
        ENUMS(RIGHT_B_CV_INPUT, 5),
        ENUMS(RIGHT_C_CV_INPUT, 5),
        ENUMS(RIGHT_D_CV_INPUT, 5),
        STEREO_A_CV_INPUT,
        STEREO_B_CV_INPUT,
        STEREO_C_CV_INPUT,
        STEREO_D_CV_INPUT,
        MUTE_A_INPUT,
        MUTE_B_INPUT,
        MUTE_C_INPUT,
        MUTE_D_INPUT,
        NUM_INPUTS
    };
    enum OutputIds
    {
        ENUMS(LEFT_OUTPUT, 4),
        ENUMS(LEFTC_OUTPUT, 4),
        ENUMS(RIGHT_OUTPUT, 4),
        ENUMS(RIGHTC_OUTPUT, 4),
        ENUMS(STEREOL_OUTPUT, 4),
        ENUMS(STEREOR_OUTPUT, 4),
        NUM_OUTPUTS
    };

    enum LighIds
    {
        MUTE_A_LIGHT,
        MUTE_B_LIGHT,
        MUTE_C_LIGHT,
        MUTE_D_LIGHT,
        NUM_LIGHTS
    };

    dsp::SchmittTrigger muteAT, muteBT, muteCT, muteDT;

    bool muteA=false, muteB=false, muteC=false, muteD=false;

    int panelTheme;
    float lineL[5]={};
    float lineR[5]={};

    float AL[5] = {}, AR[5] = {};
    float BL[5] = {}, BR[5] = {};
    float CL[5] = {}, CR[5] = {};
    float DL[5] = {}, DR[5] = {};


    float outAL=0, outBL=0, outCL=0, outDL=0;
    float outAR=0, outBR=0, outCR=0, outDR=0;

    float sumAL=0, sumAR=0;
    float sumBL=0, sumBR=0;
    float sumCL=0, sumCR=0;
    float sumDL=0, sumDR=0;

    DualMatrix()
    {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
        for (int i = 0; i < 5; i++)
        {
            configParam(LEFT_A_PARAM + i, 0.0, 1.0, 0.0, string::f("Left A%d",i+1));
            configParam(LEFT_B_PARAM + i, 0.0, 1.0, 0.0, string::f("Left B%d",i+1));
            configParam(LEFT_C_PARAM + i, 0.0, 1.0, 0.0, string::f("Left C%d",i+1));
            configParam(LEFT_D_PARAM + i, 0.0, 1.0, 0.0, string::f("Left D%d",i+1));

            configParam(RIGHT_A_PARAM + i, 0.0, 1.0, 0.0, string::f("Right A%d",i+1));
            configParam(RIGHT_B_PARAM + i, 0.0, 1.0, 0.0, string::f("Right B%d",i+1));
            configParam(RIGHT_C_PARAM + i, 0.0, 1.0, 0.0, string::f("Right C%d",i+1));
            configParam(RIGHT_D_PARAM + i, 0.0, 1.0, 0.0, string::f("Right D%d",i+1));
			
			
			configInput(LINEL_INPUT +i,string::f("Ch%d L",i+1));
			configInput(LINER_INPUT +i,string::f("Ch%d R",i+1));
			configInput(LEFT_A_CV_INPUT +i,string::f("A%d Left Cv",i+1));
			configInput(LEFT_B_CV_INPUT +i,string::f("B%d Left Cv",i+1));
			configInput(LEFT_C_CV_INPUT +i,string::f("C%d Left Cv",i+1));
			configInput(LEFT_D_CV_INPUT +i,string::f("D%d Left Cv",i+1));
			configInput(RIGHT_A_CV_INPUT +i,string::f("A%d Right Cv",i+1));
			configInput(RIGHT_B_CV_INPUT +i,string::f("B%d Right Cv",i+1));
			configInput(RIGHT_C_CV_INPUT +i,string::f("C%d Right Cv",i+1));
			configInput(RIGHT_D_CV_INPUT +i,string::f("D%d Right Cv",i+1));


        }
		for (int i = 0; i < 4; i++)
        {
		
		configOutput(LEFT_OUTPUT+i,string::f("Ch%d L",i+1));
		configOutput(LEFTC_OUTPUT+i,string::f("Ch%d L2",i+1));
		configOutput(RIGHT_OUTPUT+i,string::f("Ch%d R",i+1));
		configOutput(RIGHTC_OUTPUT+i,string::f("Ch%d R2",i+1));
		configOutput(STEREOL_OUTPUT+i,string::f("Ch%d Master L",i+1));
		configOutput(STEREOR_OUTPUT+i,string::f("Ch%d Master R",i+1));
		
		}
		
        configParam(STEREO_A_PARAM , 0.0, 1.0, 0.0, "Stereo A Level");
        configParam(STEREO_B_PARAM , 0.0, 1.0, 0.0, "Stereo B Level");
        configParam(STEREO_C_PARAM , 0.0, 1.0, 0.0, "Stereo C Level");
        configParam(STEREO_D_PARAM , 0.0, 1.0, 0.0, "Stereo D Level");

        configParam(MUTE_A_PARAM, 0.0, 1.0, 0.0, "Mute A");
        configParam(MUTE_B_PARAM, 0.0, 1.0, 0.0, "Mute B");
        configParam(MUTE_C_PARAM, 0.0, 1.0, 0.0, "Mute C");
        configParam(MUTE_D_PARAM, 0.0, 1.0, 0.0, "Mute D");
		
		configInput(STEREO_A_CV_INPUT,"A Stereo Cv");
        configInput(STEREO_B_CV_INPUT,"B Stereo Cv");
        configInput(STEREO_C_CV_INPUT,"C Stereo Cv");
        configInput(STEREO_D_CV_INPUT,"D Stereo Cv");
		
        configInput(MUTE_A_INPUT,"A mute");
        configInput(MUTE_B_INPUT,"B mute");
        configInput(MUTE_C_INPUT,"C mute");
        configInput(MUTE_D_INPUT,"D mute");

        // onReset();
        panelTheme = (loadDarkAsDefault() ? 1 : 0);
     }

    json_t *dataToJson() override
    {
      json_t *rootJ = json_object();

      json_object_set_new(rootJ, "muteA", json_boolean(muteA));
      json_object_set_new(rootJ, "muteB", json_boolean(muteB));
      json_object_set_new(rootJ, "muteC", json_boolean(muteC));
      json_object_set_new(rootJ, "muteD", json_boolean(muteD));

      // panelTheme
      json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));

      return rootJ;
    }

    void dataFromJson(json_t *rootJ) override
    {

            // panelTheme
            json_t *panelThemeJ = json_object_get(rootJ, "panelTheme");

            if (panelThemeJ)
                panelTheme = json_integer_value(panelThemeJ);

            json_t *MuteAJ = json_object_get(rootJ, "muteA");
            if (MuteAJ)
                muteA = json_boolean_value(MuteAJ);
            json_t *MuteBJ = json_object_get(rootJ, "muteB");
            if (MuteBJ)
                muteB = json_boolean_value(MuteBJ);
            json_t *MuteCJ = json_object_get(rootJ, "muteC");
            if (MuteCJ)
                muteC = json_boolean_value(MuteCJ);
            json_t *MuteDJ = json_object_get(rootJ, "muteD");
            if (MuteDJ)
                muteD = json_boolean_value(MuteDJ);
    }


  void process(const ProcessArgs &args) override
  {

    for (int i = 0; i < 5; i++)
    {
        lineL[i] = inputs[LINEL_INPUT + i].getVoltage();
        lineR[i] = inputs[LINER_INPUT + i].getVoltage();

        AL[i] = lineL[i] * params[LEFT_A_PARAM + i].getValue();
        BL[i] = lineL[i] * params[LEFT_B_PARAM + i].getValue();
        CL[i] = lineL[i] * params[LEFT_C_PARAM + i].getValue();
        DL[i] = lineL[i] * params[LEFT_D_PARAM + i].getValue();

        if(inputs[LEFT_A_CV_INPUT+i].isConnected()) AL[i]*=inputs[LEFT_A_CV_INPUT+i].getVoltage()/10.f;
        if(inputs[LEFT_B_CV_INPUT+i].isConnected()) BL[i]*=inputs[LEFT_B_CV_INPUT+i].getVoltage()/10.f;
        if(inputs[LEFT_C_CV_INPUT+i].isConnected()) CL[i]*=inputs[LEFT_C_CV_INPUT+i].getVoltage()/10.f;
        if(inputs[LEFT_D_CV_INPUT+i].isConnected()) DL[i]*=inputs[LEFT_D_CV_INPUT+i].getVoltage()/10.f;

        AR[i] = lineR[i] * params[RIGHT_A_PARAM + i].getValue();
        BR[i] = lineR[i] * params[RIGHT_B_PARAM + i].getValue();
        CR[i] = lineR[i] * params[RIGHT_C_PARAM + i].getValue();
        DR[i] = lineR[i] * params[RIGHT_D_PARAM + i].getValue();

        if(inputs[RIGHT_A_CV_INPUT+i].isConnected()) AR[i]*=inputs[RIGHT_A_CV_INPUT+i].getVoltage()/10.f;
        if(inputs[RIGHT_B_CV_INPUT+i].isConnected()) BR[i]*=inputs[RIGHT_B_CV_INPUT+i].getVoltage()/10.f;
        if(inputs[RIGHT_C_CV_INPUT+i].isConnected()) CR[i]*=inputs[RIGHT_C_CV_INPUT+i].getVoltage()/10.f;
        if(inputs[RIGHT_D_CV_INPUT+i].isConnected()) DR[i]*=inputs[RIGHT_D_CV_INPUT+i].getVoltage()/10.f;


    }

         if (muteAT.process(params[MUTE_A_PARAM].getValue() + inputs[MUTE_A_INPUT].getVoltage())) muteA = !muteA;
         if (muteBT.process(params[MUTE_B_PARAM].getValue() + inputs[MUTE_B_INPUT].getVoltage())) muteB = !muteB;
         if (muteCT.process(params[MUTE_C_PARAM].getValue() + inputs[MUTE_C_INPUT].getVoltage())) muteC = !muteC;
         if (muteDT.process(params[MUTE_D_PARAM].getValue() + inputs[MUTE_D_INPUT].getVoltage())) muteD = !muteD;

         lights[MUTE_A_LIGHT].setBrightness(muteA ? 1.f : 0.f);
         lights[MUTE_B_LIGHT].setBrightness(muteB ? 1.f : 0.f);
         lights[MUTE_C_LIGHT].setBrightness(muteC ? 1.f : 0.f);
         lights[MUTE_D_LIGHT].setBrightness(muteD ? 1.f : 0.f);

         outAL = (sumAL * params[STEREO_A_PARAM].getValue());
         if (inputs[STEREO_A_CV_INPUT].isConnected())
             outAL *= inputs[STEREO_A_CV_INPUT].getVoltage() / 10.f;
         outBL = (sumBL * params[STEREO_B_PARAM].getValue());
         if (inputs[STEREO_B_CV_INPUT].isConnected())
             outBL *= inputs[STEREO_B_CV_INPUT].getVoltage() / 10.f;
         outCL = (sumCL * params[STEREO_C_PARAM].getValue());
         if (inputs[STEREO_C_CV_INPUT].isConnected())
             outCL *= inputs[STEREO_C_CV_INPUT].getVoltage() / 10.f;
         outDL = (sumDL * params[STEREO_D_PARAM].getValue());
         if (inputs[STEREO_D_CV_INPUT].isConnected())
             outDL *= inputs[STEREO_D_CV_INPUT].getVoltage() / 10.f;

         outAR = (sumAR * params[STEREO_A_PARAM].getValue());
         if (inputs[STEREO_A_CV_INPUT].isConnected())
             outAR *= inputs[STEREO_A_CV_INPUT].getVoltage() / 10.f;
         outBR = (sumBR * params[STEREO_B_PARAM].getValue());
         if (inputs[STEREO_B_CV_INPUT].isConnected())
             outBR *= inputs[STEREO_B_CV_INPUT].getVoltage() / 10.f;
         outCR = (sumCR * params[STEREO_C_PARAM].getValue());
         if (inputs[STEREO_C_CV_INPUT].isConnected())
             outCR *= inputs[STEREO_C_CV_INPUT].getVoltage() / 10.f;
         outDR = (sumDR * params[STEREO_D_PARAM].getValue());
         if (inputs[STEREO_D_CV_INPUT].isConnected())
             outDR *= inputs[STEREO_D_CV_INPUT].getVoltage() / 10.f;

         if (muteA)
         {
             outAR = 0.0;
             outAL = 0.0;
         }

         if (muteB)
         {
             outBR = 0.0;
             outBL = 0.0;
         }

         if (muteC)
         {
             outCR = 0.0;
             outCL = 0.0;
         }

         if (muteD)
         {
             outDR = 0.0;
             outDL = 0.0;
         }

         sumAL = AL[0]+AL[1]+AL[2]+AL[3]+AL[4];
         sumBL = BL[0]+BL[1]+BL[2]+BL[3]+BL[4];
         sumCL = CL[0]+CL[1]+CL[2]+CL[3]+CL[4];
         sumDL = DL[0]+DL[1]+DL[2]+DL[3]+DL[4];
         sumAR = AR[0]+AR[1]+AR[2]+AR[3]+AR[4];
         sumBR = BR[0]+BR[1]+BR[2]+BR[3]+BR[4];
         sumCR = CR[0]+CR[1]+CR[2]+CR[3]+CR[4];
         sumDR = DR[0]+DR[1]+DR[2]+DR[3]+DR[4];

         outputs[LEFT_OUTPUT].setVoltage(sumAL * 0.5f);
         outputs[LEFT_OUTPUT + 1].setVoltage(sumBL * 0.5f);
         outputs[LEFT_OUTPUT + 2].setVoltage(sumCL * 0.5f);
         outputs[LEFT_OUTPUT + 3].setVoltage(sumDL * 0.5f);

         outputs[LEFTC_OUTPUT].setVoltage(sumAL * 0.5f);
         outputs[LEFTC_OUTPUT + 1].setVoltage(sumBL * 0.5f);
         outputs[LEFTC_OUTPUT + 2].setVoltage(sumCL * 0.5f);
         outputs[LEFTC_OUTPUT + 3].setVoltage(sumDL * 0.5f);

         outputs[RIGHT_OUTPUT].setVoltage(sumAR * 0.5f);
         outputs[RIGHT_OUTPUT + 1].setVoltage(sumBR * 0.5f);
         outputs[RIGHT_OUTPUT + 2].setVoltage(sumCR * 0.5f);
         outputs[RIGHT_OUTPUT + 3].setVoltage(sumDR * 0.5f);

         outputs[RIGHTC_OUTPUT].setVoltage(sumAR * 0.5f);
         outputs[RIGHTC_OUTPUT + 1].setVoltage(sumBR * 0.5f);
         outputs[RIGHTC_OUTPUT + 2].setVoltage(sumCR * 0.5f);
         outputs[RIGHTC_OUTPUT + 3].setVoltage(sumDR * 0.5f);

         outputs[STEREOL_OUTPUT].setVoltage(outAL * 0.5f);
         outputs[STEREOL_OUTPUT + 1].setVoltage(outBL * 0.5f);
         outputs[STEREOL_OUTPUT + 2].setVoltage(outCL * 0.5f);
         outputs[STEREOL_OUTPUT + 3].setVoltage(outDL * 0.5f);
         outputs[STEREOR_OUTPUT].setVoltage(outAR * 0.5f);
         outputs[STEREOR_OUTPUT + 1].setVoltage(outBR * 0.5f);
         outputs[STEREOR_OUTPUT + 2].setVoltage(outCR * 0.5f);
         outputs[STEREOR_OUTPUT + 3].setVoltage(outDR * 0.5f);
    }
};

template <typename BASE>
struct ULight : BASE
{
    ULight()
    {
        this->box.size = mm2px(Vec(5, 5));
    }
};
//////////////////////////////////////////////////////////////////
struct DualMatrixWidget : ModuleWidget {

    int lastPanelTheme = -1;
	std::shared_ptr<window::Svg> light_svg;
	std::shared_ptr<window::Svg> dark_svg;
	
    struct PanelThemeItem : MenuItem
    {
        DualMatrix *module;
        int theme;
        void onAction(const event::Action &e) override
        {
            module->panelTheme = theme;
        }
        void step() override
        {
            rightText = (module->panelTheme == theme) ? "✔" : "";
        }
    };
    void appendContextMenu(Menu *menu) override
    {
        MenuLabel *spacerLabel = new MenuLabel();
        menu->addChild(spacerLabel);

        DualMatrix *module = dynamic_cast<DualMatrix *>(this->module);
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

    /////////////////////////////////////
    DualMatrixWidget(DualMatrix *module)
    {
        setModule(module);
        // Main panels from Inkscape
 		light_svg = APP->window->loadSvg(asset::plugin(pluginInstance, "res/Light/DualMatrix.svg"));
		dark_svg = APP->window->loadSvg(asset::plugin(pluginInstance, "res/Dark/DualMatrix.svg"));
		int panelTheme = isDark(module ? (&(((DualMatrix*)module)->panelTheme)) : NULL) ? 1 : 0;// need this here since step() not called for module browser
		setPanel(panelTheme == 0 ? light_svg : dark_svg);	

        int k = 35;
        int j = 29;
        int j1 = 32;



        addChild(createWidget<ScrewBlack>(Vec(15, 0)));
        addChild(createWidget<ScrewBlack>(Vec(box.size.x - 30, 0)));
        addChild(createWidget<ScrewBlack>(Vec(15, 365)));
        addChild(createWidget<ScrewBlack>(Vec(box.size.x - 30, 365)));
        for (int i = 0; i < 5; i++)
        {
            addParam(createParam<FlatA>(Vec(10+i*k, 70+k*0), module, DualMatrix::LEFT_A_PARAM + i));
            addParam(createParam<FlatA>(Vec(10+i*k, 70+k*1), module, DualMatrix::LEFT_B_PARAM + i));
            addParam(createParam<FlatA>(Vec(10+i*k, 70+k*2), module, DualMatrix::LEFT_C_PARAM + i));
            addParam(createParam<FlatA>(Vec(10+i*k, 70+k*3), module, DualMatrix::LEFT_D_PARAM + i));

            addParam(createParam<FlatA>(Vec(box.size.x-40 - i * k, 70 + k * 0), module, DualMatrix::RIGHT_A_PARAM_LAST - i));
            addParam(createParam<FlatA>(Vec(box.size.x-40 - i * k, 70 + k * 1), module, DualMatrix::RIGHT_B_PARAM_LAST - i));
            addParam(createParam<FlatA>(Vec(box.size.x-40 - i * k, 70 + k * 2), module, DualMatrix::RIGHT_C_PARAM_LAST - i));
            addParam(createParam<FlatA>(Vec(box.size.x-40 - i * k, 70 + k * 3), module, DualMatrix::RIGHT_D_PARAM_LAST - i));

            addInput(createInput<PJ301MSPort>(Vec(15 + i * j1, 210 + j * 0), module, DualMatrix::LEFT_A_CV_INPUT + i));
            addInput(createInput<PJ301MSPort>(Vec(15 + i * j1, 210 + j * 1), module, DualMatrix::LEFT_B_CV_INPUT + i));
            addInput(createInput<PJ301MSPort>(Vec(15 + i * j1, 210 + j * 2), module, DualMatrix::LEFT_C_CV_INPUT + i));
            addInput(createInput<PJ301MSPort>(Vec(15 + i * j1, 210 + j * 3), module, DualMatrix::LEFT_D_CV_INPUT + i));

            addInput(createInput<PJ301MSPort>(Vec(box.size.x-45 - i * j1, 210 + j * 0), module, DualMatrix::RIGHT_A_CV_INPUT_LAST - i));
            addInput(createInput<PJ301MSPort>(Vec(box.size.x-45 - i * j1, 210 + j * 1), module, DualMatrix::RIGHT_B_CV_INPUT_LAST - i));
            addInput(createInput<PJ301MSPort>(Vec(box.size.x-45 - i * j1, 210 + j * 2), module, DualMatrix::RIGHT_C_CV_INPUT_LAST - i));
            addInput(createInput<PJ301MSPort>(Vec(box.size.x-45 - i * j1, 210 + j * 3), module, DualMatrix::RIGHT_D_CV_INPUT_LAST - i));
        }
        for (int i = 0; i < 5; i++)
        {
            addInput(createInput<PJ301MLPort>(Vec(16 + i * 43, 330), module, DualMatrix::LINEL_INPUT + i));
            addInput(createInput<PJ301MLPort>(Vec(16 + ((5+i) * 43), 330), module, DualMatrix::LINER_INPUT + i));
        }

        for (int i = 0; i < 4; i++)
        {
            addOutput(createOutput<PJ301MSPort>(Vec(35 + i * j, 5 + j * 0), module, DualMatrix::LEFT_OUTPUT + i));
            addOutput(createOutput<PJ301MSPort>(Vec(45 + i * j, 5 + j * 1), module, DualMatrix::LEFTC_OUTPUT + i));

            addOutput(createOutput<PJ301MSPort>(Vec(box.size.x-65 - i * j, 5 + j * 0), module, DualMatrix::RIGHT_OUTPUT_LAST - i));
            addOutput(createOutput<PJ301MSPort>(Vec(box.size.x-75 - i * j, 5 + j * 1), module, DualMatrix::RIGHTC_OUTPUT_LAST - i));

            addOutput(createOutput<PJ301MLPort>(Vec((box.size.x / 3) +17+ i*j , 5 + j * 0), module, DualMatrix::STEREOL_OUTPUT + i));
            addOutput(createOutput<PJ301MRPort>(Vec((box.size.x / 3) +17+ i*j , 5 + j * 1), module, DualMatrix::STEREOR_OUTPUT + i));
        }

            addParam(createParam<FlatR>(Vec((box.size.x / 2) - 35, 70 + k * 0), module, DualMatrix::STEREO_A_PARAM));
            addParam(createParam<FlatR>(Vec((box.size.x / 2) - 35, 70 + k * 1), module, DualMatrix::STEREO_B_PARAM));
            addParam(createParam<FlatR>(Vec((box.size.x / 2) - 35, 70 + k * 2), module, DualMatrix::STEREO_C_PARAM));
            addParam(createParam<FlatR>(Vec((box.size.x / 2) - 35, 70 + k * 3), module, DualMatrix::STEREO_D_PARAM));

            addInput(createInput<PJ301MSPort>(Vec(box.size.x / 2 +5, 72 + k * 0), module, DualMatrix::STEREO_A_CV_INPUT));
            addInput(createInput<PJ301MSPort>(Vec(box.size.x / 2 +5, 72 + k * 1), module, DualMatrix::STEREO_B_CV_INPUT));
            addInput(createInput<PJ301MSPort>(Vec(box.size.x / 2 +5, 72 + k * 2), module, DualMatrix::STEREO_C_CV_INPUT));
            addInput(createInput<PJ301MSPort>(Vec(box.size.x / 2 +5, 72 + k * 3), module, DualMatrix::STEREO_D_CV_INPUT));

            addInput(createInput<PJ301MSPort>(Vec(box.size.x / 2 + 5, 210 + j * 0), module, DualMatrix::MUTE_A_INPUT));
            addInput(createInput<PJ301MSPort>(Vec(box.size.x / 2 + 5, 210 + j * 1), module, DualMatrix::MUTE_B_INPUT));
            addInput(createInput<PJ301MSPort>(Vec(box.size.x / 2 + 5, 210 + j * 2), module, DualMatrix::MUTE_C_INPUT));
            addInput(createInput<PJ301MSPort>(Vec(box.size.x / 2 + 5, 210 + j * 3), module, DualMatrix::MUTE_D_INPUT));

            addParam(createLightParam<LEDLightBezel<OrangeLight>>(Vec(box.size.x / 2 - 35, 212 + j * 0), module, DualMatrix::MUTE_A_PARAM, DualMatrix::MUTE_A_LIGHT));
            addParam(createLightParam<LEDLightBezel<OrangeLight>>(Vec(box.size.x / 2 - 35, 212 + j * 1), module, DualMatrix::MUTE_B_PARAM, DualMatrix::MUTE_B_LIGHT));
            addParam(createLightParam<LEDLightBezel<OrangeLight>>(Vec(box.size.x / 2 - 35, 212 + j * 2), module, DualMatrix::MUTE_C_PARAM, DualMatrix::MUTE_C_LIGHT));
            addParam(createLightParam<LEDLightBezel<OrangeLight>>(Vec(box.size.x / 2 - 35, 212 + j * 3), module, DualMatrix::MUTE_D_PARAM, DualMatrix::MUTE_D_LIGHT));


    }
    void step() override {
		int panelTheme = isDark(module ? (&(((DualMatrix*)module)->panelTheme)) : NULL) ? 1 : 0;
		if (lastPanelTheme != panelTheme) {
			lastPanelTheme = panelTheme;
			SvgPanel* panel = (SvgPanel*)getPanel();
			panel->setBackground(panelTheme == 0 ? light_svg : dark_svg);
		}
		Widget::step();
	}
};
Model *modelDualMatrix = createModel<DualMatrix, DualMatrixWidget>("DualMatrix");
