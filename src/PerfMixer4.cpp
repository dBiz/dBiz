////////////////////////////////////////////////////////////////////////////
// <8 Track stereo mixer>
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

///////////////////////////////////////////////////

struct PerfMixer4 : Module {
  enum ParamIds
  {
    MAIN_VOL_PARAM,
    AUX_R1_PARAM,
    AUX_R2_PARAM,
    AUX_S1_PARAM,
    AUX_S2_PARAM,
    ENUMS(VOL_PARAM, 4),
    ENUMS(PAN_PARAM, 4),
    ENUMS(AUX_1_PARAM ,4),
    ENUMS(AUX_2_PARAM ,4),
    ENUMS(MUTE_PARAM, 4),
    NUM_PARAMS
  };
  enum InputIds
  {
    ENUMS(CH_L_INPUT, 4),
    ENUMS(CH_R_INPUT, 4),
    ENUMS(CH_VOL_INPUT, 4),
    ENUMS(CH_MUTE_INPUT, 4),
    ENUMS(CH_PAN_INPUT, 4),
    ENUMS(AUX_1_INPUT, 4),
    ENUMS(AUX_2_INPUT, 4),
    RETURN_1_L_INPUT,
    RETURN_1_R_INPUT,
    RETURN_2_L_INPUT,
    RETURN_2_R_INPUT,
    NUM_INPUTS
  };

  enum OutputIds {
		MIX_OUTPUT_L,
    MIX_OUTPUT_R,
    SEND_1_L_OUTPUT,
    SEND_1_R_OUTPUT,
    SEND_2_L_OUTPUT,
    SEND_2_R_OUTPUT,
		NUM_OUTPUTS
  };

  enum LightIds
  {
    ENUMS(PAN_L_LIGHT, 4),
    ENUMS(PAN_R_LIGHT, 4),
    ENUMS(MUTE_LIGHT, 4),
    ENUMS(FADER_LIGHT, 4),
    ENUMS(METERL_LIGHT, (11*4)),
    ENUMS(METERR_LIGHT, (11*4)),
    NUM_LIGHTS
  };

  dsp::SchmittTrigger mute_triggers[4];

  bool mute_states[4]= {false};

  float ch_l_ins[4]={};
  float ch_r_ins[4]={};
  float channel_outs_l[4]={};
  float channel_outs_r[4]={};
  float channel_s1_L[4]={};
  float channel_s1_R[4]={};
  float channel_s2_L[4]={};
  float channel_s2_R[4]={};
  float left_sum = 0.0;
  float right_sum = 0.0;

  float send_1_L_sum = 0.0;
  float send_1_R_sum = 0.0;
  float send_2_R_sum = 0.0;
  float send_2_L_sum = 0.0;

  dsp::VuMeter2 vuBarsL[4];
  dsp::VuMeter2 vuBarsR[4];
  dsp::ClockDivider lightCounter;

  int panelTheme;

  PerfMixer4()
  {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS,NUM_LIGHTS);

    configParam(MAIN_VOL_PARAM,  0.0, 1.0, 0.5,"Mix Level", "%", 0, 100);
    configParam(AUX_R1_PARAM,  0.0, 1.0, 0.0,"Aux Return 1", "%", 0, 100);
    configParam(AUX_R2_PARAM,  0.0, 1.0, 0.0,"Aux Return 2", "%", 0, 100);
    configParam(AUX_S1_PARAM,  0.0, 1.0, 0.0,"Auz Send 1", "%", 0, 100);
    configParam(AUX_S2_PARAM,  0.0, 1.0, 0.0,"Auz Send 2", "%", 0, 100);

    for(int i=0;i<4;i++)
    {
      configParam(VOL_PARAM + i,  0.0, 1.0, 0.0,"Ch Level", "%", 0, 100);
      configParam(PAN_PARAM + i,  0.0, 1.0, 0.5,"Ch Pan", "%", 0, 100);
      configParam(AUX_1_PARAM + i,  0.0, 1.0, 0.0,"Send 1 Level", "%", 0, 100);
      configParam(AUX_2_PARAM + i,  0.0, 1.0, 0.0,"Send 2 Level", "%", 0, 100);
      configButton(MUTE_PARAM + i);

    }
   lightCounter.setDivision(256);

   onReset();

   panelTheme = (loadDarkAsDefault() ? 1 : 0);
  }





void process(const ProcessArgs &args) override {

  send_1_L_sum = 0.0;
  send_1_R_sum = 0.0;
  send_2_L_sum = 0.0;
  send_2_R_sum = 0.0;
  left_sum = 0.0;
  right_sum = 0.0;


  float pan_cv[4]={};
  float pan_pos[4]={};

    // mute triggers

    for  (int i = 0 ; i < 4; i++)
      {

        if (mute_triggers[i].process(params[MUTE_PARAM + i].getValue()+inputs[CH_MUTE_INPUT+i].getVoltage()))
          {
           // mute_states[i] ^= true;
           mute_states[i] = !mute_states[i];
          }

          lights[MUTE_LIGHT + i].setBrightness(mute_states[i] ? 1.f : 0.f);
      }


    for (int i = 0 ; i < 4 ; i++)
    {
      pan_cv[i] = inputs[CH_PAN_INPUT + i].getVoltage() / 5;
      pan_pos[i] = pan_cv[i] + params[PAN_PARAM + i].getValue();
      if (pan_pos[i] < 0)
        pan_pos[i] = 0;
      if (pan_pos[i] > 1)
        pan_pos[i] = 1;

      lights[PAN_L_LIGHT+i].setBrightness(1-pan_pos[i]);
      lights[PAN_R_LIGHT+i].setBrightness(pan_pos[i]);

      ch_l_ins[i] = (inputs[CH_L_INPUT + i].getVoltage() * std::pow(params[VOL_PARAM + i].getValue(),2.f));
      if(inputs[CH_VOL_INPUT+i].isConnected())
       ch_l_ins[i] *= (inputs[CH_VOL_INPUT + i].getVoltage() / 10.0f);

      ch_r_ins[i] = (inputs[CH_R_INPUT + i].getVoltage() * std::pow(params[VOL_PARAM + i].getValue(), 2.f));
      if(inputs[CH_VOL_INPUT+i].isConnected())
       ch_r_ins[i]*= (inputs[CH_VOL_INPUT + i].getVoltage() / 10.0f);

      if (mute_states[i] )
      {
        ch_l_ins[i] = 0.0;
        ch_r_ins[i] = 0.0;
      }

          if(!inputs[CH_R_INPUT+i].getVoltage())
          {
            channel_outs_l[i] = ch_l_ins[i] * (1 - pan_pos[i]) * 3;
            channel_outs_r[i] = ch_l_ins[i] * pan_pos[i] * 3;
          }
          else
          {
            channel_outs_l[i] = ch_l_ins[i] * 2;
            channel_outs_r[i] = ch_r_ins[i] * 2;
          }


          channel_s1_L[i] = (channel_outs_l[i] * params[AUX_1_PARAM + i].getValue());
          if(inputs[AUX_1_INPUT+i].isConnected())
          channel_s1_L[i] *= (inputs[AUX_1_INPUT + i].getVoltage() /10.f);

          channel_s2_L[i] = (channel_outs_l[i] * params[AUX_2_PARAM + i].getValue());
          if(inputs[AUX_2_INPUT+i].isConnected())
          channel_s2_L[i] *= (inputs[AUX_2_INPUT + i].getVoltage() /10.f);

          channel_s1_R[i] = (channel_outs_r[i] * params[AUX_1_PARAM + i].getValue());
          if(inputs[AUX_1_INPUT+i].isConnected())
          channel_s1_R[i] *= (inputs[AUX_1_INPUT + i].getVoltage() /10.f);

          channel_s2_R[i] = (channel_outs_r[i] * params[AUX_2_PARAM + i].getValue());
          if(inputs[AUX_2_INPUT+i].isConnected())
          channel_s2_R[i] *= (inputs[AUX_2_INPUT + i].getVoltage() /10.f);


          vuBarsL[i].process(args.sampleTime,channel_outs_l[i] / 10.0);
          vuBarsR[i].process(args.sampleTime,channel_outs_r[i] / 10.0);

  if (lightCounter.process())
  {
    for(int i=0;i<4;i++){

          for (int l = 1; l < 11; l++)
          {
            lights[METERL_LIGHT + (i * 11)+l].setBrightness(vuBarsL[i].getBrightness(-3.f * (l + 1), -3.f * l));
            lights[METERR_LIGHT + (i * 11)+l].setBrightness(vuBarsR[i].getBrightness(-3.f * (l + 1), -3.f * l));
            }
            float b = vuBarsL[i].getBrightness(-24.f, 0.f);
            lights[FADER_LIGHT + i].setBrightness(b);
        }

  }

            send_1_L_sum += channel_s1_L[i];
            send_1_R_sum += channel_s1_R[i];
            send_2_L_sum += channel_s2_L[i];
            send_2_R_sum += channel_s2_R[i];
            left_sum += channel_outs_l[i];
            right_sum += channel_outs_r[i];

    }


    // get returns

    float return_1_l = inputs[RETURN_1_L_INPUT].getVoltage() * params[AUX_R1_PARAM].getValue();
    float return_1_r = inputs[RETURN_1_R_INPUT].getVoltage() * params[AUX_R1_PARAM].getValue();
    float return_2_l = inputs[RETURN_2_L_INPUT].getVoltage() * params[AUX_R2_PARAM].getValue();
    float return_2_r = inputs[RETURN_2_R_INPUT].getVoltage() * params[AUX_R2_PARAM].getValue();


  	float mix_l = (left_sum + return_1_l + return_2_l)  * params[MAIN_VOL_PARAM].getValue()*0.5;
    float mix_r = (right_sum + return_1_r + return_2_r) * params[MAIN_VOL_PARAM].getValue()*0.5;


    float send_1_L_mix = (send_1_L_sum) * params[AUX_S1_PARAM].getValue();
    float send_1_R_mix = (send_1_R_sum) * params[AUX_S1_PARAM].getValue();
    float send_2_L_mix = (send_2_L_sum) * params[AUX_S2_PARAM].getValue();
    float send_2_R_mix = (send_2_R_sum) * params[AUX_S2_PARAM].getValue();

    outputs[MIX_OUTPUT_L].setVoltage(mix_l);
    outputs[MIX_OUTPUT_R].setVoltage(mix_r);


    outputs[SEND_1_L_OUTPUT].setVoltage(3 * send_1_L_mix);
    outputs[SEND_1_R_OUTPUT].setVoltage(3 * send_1_R_mix);
    outputs[SEND_2_L_OUTPUT].setVoltage(3 * send_2_L_mix);
    outputs[SEND_2_R_OUTPUT].setVoltage(3 * send_2_R_mix);


  }
  void onReset() override
  {
    for (int i = 0; i < 4; i++)
    {
      mute_states[i] = false;
    }
  }

  json_t *dataToJson() override
  {
    json_t *rootJ = json_object();

    // mute states
    json_t *mute_statesJ = json_array();
    for (int i = 0; i < 4; i++)
    {
      json_t *mute_stateJ = json_boolean(mute_states[i]);
      json_array_append_new(mute_statesJ, mute_stateJ);
    }
    json_object_set_new(rootJ, "mutes", mute_statesJ);

    json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));

    return rootJ;
  } 
  
  void dataFromJson(json_t *rootJ) override
  {
    // mute states
    json_t *mute_statesJ = json_object_get(rootJ, "mutes");
    if (mute_statesJ)
    {
      for (int i = 0; i < 4; i++)
      {
        json_t *mute_stateJ = json_array_get(mute_statesJ, i);
        if (mute_stateJ)
          mute_states[i] = json_boolean_value(mute_stateJ);
      }
    }
    json_t *panelThemeJ = json_object_get(rootJ, "panelTheme");
    if (panelThemeJ)
      panelTheme = json_integer_value(panelThemeJ);
  }

};
template <typename BASE>
struct MuteLight : BASE
{
  MuteLight()
  {
    this->box.size = Vec(17.0, 17.0);
  }
};

template <typename BASE>
struct MeterLight : BASE
{
  MeterLight()
  {
    this->box.size = Vec(4, 4);
    this->bgColor = nvgRGBAf(0.0, 0.0, 0.0, 0.1);
  }
};


struct PerfMixer4Widget : ModuleWidget {


  SvgPanel* darkPanel;
  struct PanelThemeItem : MenuItem {
    PerfMixer4 *module;
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

    PerfMixer4 *module = dynamic_cast<PerfMixer4*>(this->module);
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
PerfMixer4Widget(PerfMixer4 *module){
  setModule(module);
  setPanel(APP->window->loadSvg(asset::plugin(pluginInstance,  "res/Light/PerfMixer4.svg")));
  if (module) {
    darkPanel = new SvgPanel();
    darkPanel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Dark/PerfMixer4.svg")));
    darkPanel->visible = false;
    addChild(darkPanel);
  }

  int column_1 = 70;
  int lb=4;
  int right_column = 188;
  int top=50;
  int top_row = 60;
  int row_spacing = 28;
  int row_in = 40;
  int column_spacing = 30;

  addParam(createParam<LRoundWhy>(Vec(right_column + 5, 10), module, PerfMixer4::MAIN_VOL_PARAM)); // master volume
  
  addParam(createParam<MicroBlu>(Vec(right_column+7.5, 225 ), module, PerfMixer4::AUX_R1_PARAM));  //Aux
  addParam(createParam<MicroBlu>(Vec(right_column+7.5, 285 ), module, PerfMixer4::AUX_R2_PARAM));
  addParam(createParam<MicroBlu>(Vec(right_column+7.5, 102.5 ), module, PerfMixer4::AUX_S1_PARAM));
  addParam(createParam<MicroBlu>(Vec(right_column+7.5, 160 ), module, PerfMixer4::AUX_S2_PARAM));

  // outputs

  addOutput(createOutput<PJ301MLPort>(Vec(right_column + 5, 60), module, PerfMixer4::MIX_OUTPUT_L));
  addOutput(createOutput<PJ301MRPort>(Vec(right_column + 32, 60), module, PerfMixer4::MIX_OUTPUT_R));

  addOutput(createOutput<PJ301MLPort>(Vec(right_column + 35, 100), module, PerfMixer4::SEND_1_L_OUTPUT));
  addOutput(createOutput<PJ301MRPort>(Vec(right_column + 35, 126), module, PerfMixer4::SEND_1_R_OUTPUT));

  addOutput(createOutput<PJ301MLPort>(Vec(right_column + 35, 160), module, PerfMixer4::SEND_2_L_OUTPUT));
  addOutput(createOutput<PJ301MRPort>(Vec(right_column + 35, 186), module, PerfMixer4::SEND_2_R_OUTPUT));

  addInput(createInput<PJ301MLPort>(Vec(right_column + 35, 225), module, PerfMixer4::RETURN_1_L_INPUT));
  addInput(createInput<PJ301MRPort>(Vec(right_column + 35, 251), module, PerfMixer4::RETURN_1_R_INPUT));

  addInput(createInput<PJ301MLPort>(Vec(right_column + 35, 285), module, PerfMixer4::RETURN_2_L_INPUT));
  addInput(createInput<PJ301MRPort>(Vec(right_column + 35, 311), module, PerfMixer4::RETURN_2_R_INPUT));

  // channels

  addInput(createInput<PJ301MCPort>(Vec(column_1 + column_spacing * 0, 14), module, PerfMixer4::AUX_1_INPUT + 0));
  addInput(createInput<PJ301MCPort>(Vec(column_1 + column_spacing * 1, 14), module, PerfMixer4::AUX_1_INPUT + 1));
  addInput(createInput<PJ301MCPort>(Vec(column_1 + column_spacing * 2, 14), module, PerfMixer4::AUX_1_INPUT + 2));
  addInput(createInput<PJ301MCPort>(Vec(column_1 + column_spacing * 3, 14), module, PerfMixer4::AUX_1_INPUT + 3));

  addInput(createInput<PJ301MCPort>(Vec(column_1 + column_spacing * 0, 40), module, PerfMixer4::AUX_2_INPUT + 0));
  addInput(createInput<PJ301MCPort>(Vec(column_1 + column_spacing * 1, 40), module, PerfMixer4::AUX_2_INPUT + 1));
  addInput(createInput<PJ301MCPort>(Vec(column_1 + column_spacing * 2, 40), module, PerfMixer4::AUX_2_INPUT + 2));
  addInput(createInput<PJ301MCPort>(Vec(column_1 + column_spacing * 3, 40), module, PerfMixer4::AUX_2_INPUT + 3));

  addInput(createInput<PJ301MIPort>(Vec(lb, top + row_in * 0), module, PerfMixer4::CH_L_INPUT + 0));
  addInput(createInput<PJ301MIPort>(Vec(lb, top + row_in * 1), module, PerfMixer4::CH_L_INPUT + 1));
  addInput(createInput<PJ301MIPort>(Vec(lb, top + row_in * 2), module, PerfMixer4::CH_L_INPUT + 2));
  addInput(createInput<PJ301MIPort>(Vec(lb, top + row_in * 3), module, PerfMixer4::CH_L_INPUT + 3));

  addInput(createInput<PJ301MIPort>(Vec(lb + 26, top + row_in * 0), module, PerfMixer4::CH_R_INPUT + 0));
  addInput(createInput<PJ301MIPort>(Vec(lb + 26, top + row_in * 1), module, PerfMixer4::CH_R_INPUT + 1));
  addInput(createInput<PJ301MIPort>(Vec(lb + 26, top + row_in * 2), module, PerfMixer4::CH_R_INPUT + 2));
  addInput(createInput<PJ301MIPort>(Vec(lb + 26, top + row_in * 3), module, PerfMixer4::CH_R_INPUT + 3));

  addInput(createInput<PJ301MCPort>(Vec(column_1 + column_spacing * 0 - 1, top_row + row_spacing * 6 - 48 + top), module, PerfMixer4::CH_VOL_INPUT + 0));
  addInput(createInput<PJ301MCPort>(Vec(column_1 + column_spacing * 1 - 1, top_row + row_spacing * 6 - 48 + top), module, PerfMixer4::CH_VOL_INPUT + 1));
  addInput(createInput<PJ301MCPort>(Vec(column_1 + column_spacing * 2 - 1, top_row + row_spacing * 6 - 48 + top), module, PerfMixer4::CH_VOL_INPUT + 2));
  addInput(createInput<PJ301MCPort>(Vec(column_1 + column_spacing * 3 - 1, top_row + row_spacing * 6 - 48 + top), module, PerfMixer4::CH_VOL_INPUT + 3));

  addInput(createInput<PJ301MOrPort>(Vec(column_1 + column_spacing * 0 - 1, top_row + row_spacing * 6 + top + 6), module, PerfMixer4::CH_PAN_INPUT + 0));
  addInput(createInput<PJ301MOrPort>(Vec(column_1 + column_spacing * 1 - 1, top_row + row_spacing * 6 + top + 6), module, PerfMixer4::CH_PAN_INPUT + 1));
  addInput(createInput<PJ301MOrPort>(Vec(column_1 + column_spacing * 2 - 1, top_row + row_spacing * 6 + top + 6), module, PerfMixer4::CH_PAN_INPUT + 2));
  addInput(createInput<PJ301MOrPort>(Vec(column_1 + column_spacing * 3 - 1, top_row + row_spacing * 6 + top + 6), module, PerfMixer4::CH_PAN_INPUT + 3));

  addInput(createInput<PJ301MCPort>(Vec(column_1 + column_spacing * 0 - 1, top_row + row_spacing * 6 + top + 55), module, PerfMixer4::CH_MUTE_INPUT + 0));
  addInput(createInput<PJ301MCPort>(Vec(column_1 + column_spacing * 1 - 1, top_row + row_spacing * 6 + top + 55), module, PerfMixer4::CH_MUTE_INPUT + 1));
  addInput(createInput<PJ301MCPort>(Vec(column_1 + column_spacing * 2 - 1, top_row + row_spacing * 6 + top + 55), module, PerfMixer4::CH_MUTE_INPUT + 2));
  addInput(createInput<PJ301MCPort>(Vec(column_1 + column_spacing * 3 - 1, top_row + row_spacing * 6 + top + 55), module, PerfMixer4::CH_MUTE_INPUT + 3));

  for (int i = 0; i < 4; i++)
  {

    addParam(createParam<MicroBlu>(Vec(column_1 + column_spacing * i, 75), module, PerfMixer4::AUX_1_PARAM + i));
    addParam(createParam<MicroBlu>(Vec(column_1 + column_spacing * i, 105), module, PerfMixer4::AUX_2_PARAM + i));

    // addParam(createParam<LEDSliderBlue>(Vec(column_1 + column_spacing * i - 5, top_row + row_spacing * 2 - 20 + top), module, PerfMixer4::VOL_PARAM + i));
    addParam(createLightParam<LEDLightSliderFixed<BlueLight>>(Vec(column_1 + column_spacing * i - 5, top_row + row_spacing * 2 - 20 + top), module, PerfMixer4::VOL_PARAM + i, PerfMixer4::FADER_LIGHT + i));
    /////////////////////////////////////////////////////

    addChild(createLight<MeterLight<OrangeLight>>(Vec(column_1 + column_spacing * i + 1, top_row + row_spacing * 6 + top - 13), module, PerfMixer4::PAN_L_LIGHT + i));
    addChild(createLight<MeterLight<OrangeLight>>(Vec(column_1 + column_spacing * i + 20, top_row + row_spacing * 6 + top - 13), module, PerfMixer4::PAN_R_LIGHT + i));

    addParam(createParam<Trim>(Vec(column_1 + column_spacing * i + 3, top_row + row_spacing * 6 + top - 12), module, PerfMixer4::PAN_PARAM + i));

    ////////////////////////////////////////////////////////

    addParam(createParam<LEDB>(Vec(column_1 + column_spacing * i + 3, top_row + row_spacing * 7 + 10.5 + top - 4), module, PerfMixer4::MUTE_PARAM + i));
    addChild(createLight<MuteLight<BlueLight>>(Vec(column_1 + column_spacing * i + 4.5, top_row + row_spacing * 7 + 12 + top - 4), module, PerfMixer4::MUTE_LIGHT + i));

    addChild(createLight<MeterLight<PurpleLight>>(Vec(column_1 + 19 + column_spacing * i - 5, top_row + row_spacing * 2 - 27 + top + 7.5), module, PerfMixer4::METERL_LIGHT + (11 * i)));
    addChild(createLight<MeterLight<PurpleLight>>(Vec(column_1 + 19 + column_spacing * i - 5, top_row + row_spacing * 2 - 27 + top + 7.5 * 2), module, PerfMixer4::METERL_LIGHT + 1 + (11 * i)));
    addChild(createLight<MeterLight<BlueLight>>(Vec(column_1 + 19 + column_spacing * i - 5, top_row + row_spacing * 2 - 27 + top + 7.5 * 3), module, PerfMixer4::METERL_LIGHT + 2 + (11 * i)));
    addChild(createLight<MeterLight<BlueLight>>(Vec(column_1 + 19 + column_spacing * i - 5, top_row + row_spacing * 2 - 27 + top + 7.5 * 4), module, PerfMixer4::METERL_LIGHT + 3 + (11 * i)));
    addChild(createLight<MeterLight<BlueLight>>(Vec(column_1 + 19 + column_spacing * i - 5, top_row + row_spacing * 2 - 27 + top + 7.5 * 5), module, PerfMixer4::METERL_LIGHT + 4 + (11 * i)));
    addChild(createLight<MeterLight<BlueLight>>(Vec(column_1 + 19 + column_spacing * i - 5, top_row + row_spacing * 2 - 27 + top + 7.5 * 6), module, PerfMixer4::METERL_LIGHT + 5 + (11 * i)));
    addChild(createLight<MeterLight<BlueLight>>(Vec(column_1 + 19 + column_spacing * i - 5, top_row + row_spacing * 2 - 27 + top + 7.5 * 7), module, PerfMixer4::METERL_LIGHT + 6 + (11 * i)));
    addChild(createLight<MeterLight<WhiteLight>>(Vec(column_1 + 19 + column_spacing * i - 5, top_row + row_spacing * 2 - 27 + top + 7.5 * 8), module, PerfMixer4::METERL_LIGHT + 7 + (11 * i)));
    addChild(createLight<MeterLight<WhiteLight>>(Vec(column_1 + 19 + column_spacing * i - 5, top_row + row_spacing * 2 - 27 + top + 7.5 * 9), module, PerfMixer4::METERL_LIGHT + 8 + (11 * i)));
    addChild(createLight<MeterLight<WhiteLight>>(Vec(column_1 + 19 + column_spacing * i - 5, top_row + row_spacing * 2 - 27 + top + 7.5 * 10), module, PerfMixer4::METERL_LIGHT + 9 + (11 * i)));
    addChild(createLight<MeterLight<WhiteLight>>(Vec(column_1 + 19 + column_spacing * i - 5, top_row + row_spacing * 2 - 27 + top + 7.5 * 11), module, PerfMixer4::METERL_LIGHT + 10 + (11 * i)));

    addChild(createLight<MeterLight<PurpleLight>>(Vec(column_1 + 24 + column_spacing * i - 5, top_row + row_spacing * 2 - 27 + top + 7.5), module, PerfMixer4::METERR_LIGHT + (11 * i)));
    addChild(createLight<MeterLight<PurpleLight>>(Vec(column_1 + 24 + column_spacing * i - 5, top_row + row_spacing * 2 - 27 + top + 7.5 * 2), module, PerfMixer4::METERR_LIGHT + 1 + (11 * i)));
    addChild(createLight<MeterLight<BlueLight>>(Vec(column_1 + 24 + column_spacing * i - 5, top_row + row_spacing * 2 - 27 + top + 7.5 * 3), module, PerfMixer4::METERR_LIGHT + 2 + (11 * i)));
    addChild(createLight<MeterLight<BlueLight>>(Vec(column_1 + 24 + column_spacing * i - 5, top_row + row_spacing * 2 - 27 + top + 7.5 * 4), module, PerfMixer4::METERR_LIGHT + 3 + (11 * i)));
    addChild(createLight<MeterLight<BlueLight>>(Vec(column_1 + 24 + column_spacing * i - 5, top_row + row_spacing * 2 - 27 + top + 7.5 * 5), module, PerfMixer4::METERR_LIGHT + 4 + (11 * i)));
    addChild(createLight<MeterLight<BlueLight>>(Vec(column_1 + 24 + column_spacing * i - 5, top_row + row_spacing * 2 - 27 + top + 7.5 * 6), module, PerfMixer4::METERR_LIGHT + 5 + (11 * i)));
    addChild(createLight<MeterLight<BlueLight>>(Vec(column_1 + 24 + column_spacing * i - 5, top_row + row_spacing * 2 - 27 + top + 7.5 * 7), module, PerfMixer4::METERR_LIGHT + 6 + (11 * i)));
    addChild(createLight<MeterLight<WhiteLight>>(Vec(column_1 + 24 + column_spacing * i - 5, top_row + row_spacing * 2 - 27 + top + 7.5 * 8), module, PerfMixer4::METERR_LIGHT + 7 + (11 * i)));
    addChild(createLight<MeterLight<WhiteLight>>(Vec(column_1 + 24 + column_spacing * i - 5, top_row + row_spacing * 2 - 27 + top + 7.5 * 9), module, PerfMixer4::METERR_LIGHT + 8 + (11 * i)));
    addChild(createLight<MeterLight<WhiteLight>>(Vec(column_1 + 24 + column_spacing * i - 5, top_row + row_spacing * 2 - 27 + top + 7.5 * 10), module, PerfMixer4::METERR_LIGHT + 9 + (11 * i)));
    addChild(createLight<MeterLight<WhiteLight>>(Vec(column_1 + 24 + column_spacing * i - 5, top_row + row_spacing * 2 - 27 + top + 7.5 * 11), module, PerfMixer4::METERR_LIGHT + 10 + (11 * i)));
  }


//Screw

  addChild(createWidget<ScrewBlack>(Vec(15, 0)));
	addChild(createWidget<ScrewBlack>(Vec(box.size.x-30, 0)));
	addChild(createWidget<ScrewBlack>(Vec(15, 365)));
	addChild(createWidget<ScrewBlack>(Vec(box.size.x-30, 365)));

}

void step() override {
  if (module) {
    Widget* panel = getPanel();
    panel->visible = ((((PerfMixer4*)module)->panelTheme) == 0);
    darkPanel->visible  = ((((PerfMixer4*)module)->panelTheme) == 1);
  }
  Widget::step();
}
};
Model *modelPerfMixer4 = createModel<PerfMixer4, PerfMixer4Widget>("PerfMixer4");
