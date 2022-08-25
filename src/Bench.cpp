////////////////////////////////////////////////////////////////////////////
// <Benchr>
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

using simd::float_4;

/////////////////////////////////////////LFO/////////////////////////////

template <typename T>
struct LowFrequencyOscillator
{
  T phase = 0.f;
  T pw = 0.5f;
  T freq = 1.f;
  bool invert = false;
  bool bipolar = false;
  T resetState = T::mask();

  void setPitch(T pitch)
  {
    pitch = simd::fmin(pitch, 10.f);
    freq = dsp::approxExp2_taylor5(pitch + 30) / 1073741824;
  }

  void setReset(T reset)
  {
    reset = simd::rescale(reset, 0.1f, 2.f, 0.f, 1.f);
    T on = (reset >= 1.f);
    T off = (reset <= 0.f);
    T triggered = ~resetState & on;
    resetState = simd::ifelse(off, 0.f, resetState);
    resetState = simd::ifelse(on, T::mask(), resetState);
    phase = simd::ifelse(triggered, 0.f, phase);
  }
  void step(float dt)
  {
    T deltaPhase = simd::fmin(freq * dt, 0.5f);
    phase += deltaPhase;
    phase -= (phase >= 1.f) & 1.f;
  }

  T tri()
  {
    T p = phase;
    if (bipolar)
      p += 0.25f;
    T v = 4.f * simd::fabs(p - simd::round(p)) - 1.f;
    if (invert)
      v *= -1.f;
    if (!bipolar)
      v += 1.f;
    return v;
  }

  T sqr()
  {
    T v = simd::ifelse(phase < pw, 1.f, -1.f);
    if (invert)
      v *= -1.f;
    if (!bipolar)
      v += 1.f;
    return v;
  }
  T light()
  {
    return simd::sin(2 * T(M_PI) * phase);
  }
};

/////added fine out /////////////////////////////////////////////////
struct Bench : Module {
	enum ParamIds
  {
    MULT_PARAM,
    MULT_LOW_PARAM,
    FADE_LOW_PARAM,
    SWAP_LOW_PARAM,
    LFO_PARAM,
    LFO_WAVE_PARAM,
    A1_PARAM,
    A2_PARAM,
    FADE_PARAM,
    B1_PARAM,
    B2_PARAM,
    SWAP_PARAM,
    VOLTAGE_PARAM,
    UNI_PARAM,
    RATE_PARAM,
    LEVEL_PARAM,
    RANGE_A_PARAM,
    RANGE_B_PARAM,
    NUM_PARAMS
  };
  enum InputIds
  {
    MULT_INPUT,
    MULT_LOW_INPUT,
    SWAP_A_INPUT,
    SWAP_B_INPUT,
    SWAP_LOW_A_INPUT,
    SWAP_LOW_B_INPUT,
    FADE_A_LOW_INPUT,
    FADE_B_LOW_INPUT, 
    LFO_RESET_INPUT,
    A_INPUT,
    B_INPUT,
    A1_INPUT,
    A2_INPUT,
    B1_INPUT,
    B2_INPUT,
    RESET_INPUT,
    NUM_INPUTS
  };

  enum OutputIds {
	ENUMS(MULT_OUTPUT, 3),
  ENUMS(MULT_LOW_OUTPUT, 3),
	SWAP_A_OUTPUT,
	SWAP_B_OUTPUT,
  SWAP_LOW_A_OUTPUT,
	SWAP_LOW_B_OUTPUT,
  FADE_LOW_OUTPUT,
  A1_OUTPUT,
  A2_OUTPUT,
  B1_OUTPUT,
  B2_OUTPUT,
  FADE_OUTPUT,
  LFO_OUTPUT,
  NUM_OUTPUTS
  };

  enum LightIds
  {
    SWAP_BUTTON_LIGHT,
    SWAP_LOW_BUTTON_LIGHT,
    ENUMS(SWAP_A_LIGHT, 3),
    ENUMS(SWAP_B_LIGHT, 3),
    ENUMS(SWAP_LOW_A_LIGHT, 3),
    ENUMS(SWAP_LOW_B_LIGHT, 3),
    MULT_LIGHT,
    MULT_LOW_LIGHT,
    LFO_LIGHT,
    A1_LIGHT,
    A2_LIGHT,
    B1_LIGHT,
    B2_LIGHT,
    FADE_LIGHT,
    FADE_LOW_LIGHT,
    NUM_LIGHTS
  };

  bool swap=false;
  bool swap2=false;
  LowFrequencyOscillator<float_4> oscillator[2];
  dsp::SchmittTrigger swap_trigger;
  dsp::SchmittTrigger swap_low_trigger;
  dsp::ClockDivider lightDivider;
  
  Bench()
  {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

    configParam(MULT_PARAM, 0.0, 1.0 , 0.0,"Mult att");
    configParam(MULT_LOW_PARAM, 0.0, 1.0 , 0.0,"Mult att");
    configParam(LFO_PARAM, 0.0, 1.0 , 0.0,"Lfo Level");
    configParam(LFO_WAVE_PARAM, 0.0, 2.0 , 0.0,"Lfo wave select");
    configParam(A1_PARAM, 0.0, 1.0 , 0.0,"A1 Level");
    configParam(A2_PARAM, 0.0, 1.0, 0.0, "A2 Level");
    configParam(FADE_PARAM, 0.0, 1.0 , 0.0,"Fade A/B");
    configParam(FADE_LOW_PARAM, 0.0, 1.0 , 0.0,"Fade A/B");
    configParam(B1_PARAM, 0.0, 1.0, 0.0, "B1 Level");
    configParam(B2_PARAM, 0.0, 1.0, 0.0, "B2 Level");
    configParam(SWAP_PARAM, 0.0, 1.0 , 0.0,"Swap");
    configParam(SWAP_LOW_PARAM, 0.0, 1.0 , 0.0,"Swap");
    configParam(VOLTAGE_PARAM, 0.0, 5.0 , 0.0,"Fixed Volt Att");
    configParam(UNI_PARAM, 0.0, 1.0 , 0.0,"Uni/Bipolar Lfo");
    configParam(RATE_PARAM, -8.f, 10.f, 1.f, "Frequency", " Hz", 2, 1);
    configParam(LEVEL_PARAM, 0.0, 1.0 , 0.0,"Fade Level");
    configParam(RANGE_A_PARAM, 0.0, 2.0 , 0.0,"Input A selector");
    configParam(RANGE_B_PARAM, 0.0, 2.0, 0.0, "Input B selector");

    lightDivider.setDivision(512);

    onReset();

    panelTheme = (loadDarkAsDefault() ? 1 : 0);

  }


	int panelTheme;

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


    float mult_level = params[MULT_PARAM].getValue();
    float mult_low_level = params[MULT_LOW_PARAM].getValue();
    float freqParam = params[RATE_PARAM].getValue();
    float lfo_level = params[LFO_PARAM].getValue();
    float volt = params[VOLTAGE_PARAM].getValue();

    float fade_level = params[LEVEL_PARAM].getValue();

    float a1_level = params[A1_PARAM].getValue();
    float a2_level = params[A2_PARAM].getValue();
    float b1_level = params[B1_PARAM].getValue();
    float b2_level = params[B2_PARAM].getValue();

    float a1_in = 0.f;
    float a2_in = 0.f;
    float b1_in = 0.f;
    float b2_in = 0.f;
    float m_in = inputs[MULT_INPUT].getVoltage();
    float m1_in = inputs[MULT_LOW_INPUT].getVoltage();
    float m_out[3] = {};
     float m1_out[3] = {};

    float aswap_in = 0.f;
    float bswap_in = 0.f;
    float low_aswap_in = 0.f;
    float low_bswap_in = 0.f;

    if(inputs[SWAP_A_INPUT].isConnected())
      aswap_in = inputs[SWAP_A_INPUT].getVoltage();
      else
        aswap_in = m_in * mult_level;

    if (inputs[SWAP_B_INPUT].isConnected())
      bswap_in = inputs[SWAP_B_INPUT].getVoltage();
      else
       bswap_in = m_in * mult_level * -1 ;

    low_aswap_in=inputs[SWAP_LOW_A_INPUT].getVoltage();
    low_bswap_in=inputs[SWAP_LOW_B_INPUT].getVoltage();


    for (int m = 0; m < 3; m++)
    {
      if (m == 2)
      {
        m_out[m] = m_in * mult_level * -1;
        outputs[MULT_OUTPUT + m].setVoltage(m_out[m]);
      }
      else
      {
        m_out[m] = m_in * mult_level;
        outputs[MULT_OUTPUT + m].setVoltage(m_out[m]);
      }
    }

    for (int m1 = 0; m1 < 3; m1++)
    {
      if (m1 == 2)
      {
        m1_out[m1] = m1_in * mult_low_level * -1;
        outputs[MULT_LOW_OUTPUT + m1].setVoltage(m1_out[m1]);
      }
      else
      {
        m1_out[m1] = m1_in * mult_low_level;
        outputs[MULT_LOW_OUTPUT + m1].setVoltage(m1_out[m1]);
      }
    }


    for (int c = 0; c < 4; c += 4)
    {
      //auto *oscillator = &oscillators[1 / 4];
     

      oscillator[0].bipolar = (params[UNI_PARAM].getValue() == 0.f);
      oscillator[1].bipolar = (params[UNI_PARAM].getValue() == 0.f);

      float_4 pitch = freqParam;
      // FM1, polyphonic
            
      oscillator[0].setPitch(pitch);
      oscillator[1].setPitch(pitch);
      oscillator[1].invert=true;

      oscillator[0].step(args.sampleTime);
      oscillator[0].setReset(inputs[RESET_INPUT].getPolyVoltageSimd<float_4>(c));
      oscillator[1].step(args.sampleTime);
      oscillator[1].setReset(inputs[RESET_INPUT].getPolyVoltageSimd<float_4>(c));


      // Outputs
      if (params[LFO_WAVE_PARAM].getValue()==0)
        outputs[LFO_OUTPUT].setVoltageSimd(5.f * oscillator[0].tri() * lfo_level,c);
        else
          outputs[LFO_OUTPUT].setVoltageSimd(5.f * oscillator[0].sqr() * lfo_level,c);


      if (inputs[A1_INPUT].isConnected())
      {
        a1_in = inputs[A1_INPUT].getVoltage();
        outputs[A1_OUTPUT].setVoltage(a1_in * a1_level);
      }
      else
      {
        switch ((int)params[RANGE_A_PARAM].getValue())
         {
          case 0:
            outputs[A1_OUTPUT].setVoltage(volt * a1_level);
            break;
          case 1:
            outputs[A1_OUTPUT].setVoltage(0.f);
            break;
          default:
            outputs[A1_OUTPUT].setVoltageSimd(5.f * oscillator[1].tri() * a1_level, c);
            break;
         }
        }

      if(inputs[A2_INPUT].isConnected())
       {
         a2_in = inputs[A2_INPUT].getVoltage();
       }
       else
        {
          switch ((int)params[RANGE_A_PARAM].getValue())
          {
          case 0:
            a2_in = a2_level*volt;
            break;
          case 1:
            a2_in = 0.f;
            break;
          default:
          a2_in=outputs[A1_OUTPUT].getVoltage();
            break;
          }
        }


        if (inputs[B1_INPUT].isConnected())
        {
          b1_in = inputs[B1_INPUT].getVoltage();
          outputs[B1_OUTPUT].setVoltage(b1_in * b1_level);
        }
        else
        {
          switch ((int)params[RANGE_B_PARAM].getValue())
          {
          case 0:
            outputs[B1_OUTPUT].setVoltage(volt * b1_level);
            break;
          case 1:
            outputs[B1_OUTPUT].setVoltage(0.f);
            break;
          default:
            outputs[B1_OUTPUT].setVoltageSimd(5.f * oscillator[1].sqr() * b1_level, c);
            break;
          }
        }

        if (inputs[B2_INPUT].isConnected())
        {
          b2_in = inputs[B2_INPUT].getVoltage();
        }
        else
        {
          switch ((int)params[RANGE_B_PARAM].getValue())
          {
          case 0:
            b2_in = a2_level * volt;
            break;
          case 1:
            b2_in = 0.f;
            break;
          default:
            b2_in = outputs[B1_OUTPUT].getVoltage();
            break;
          }
        }

        outputs[A2_OUTPUT].setVoltage(a2_in * a2_level);
        outputs[B2_OUTPUT].setVoltage(b2_in * b2_level);

        float a_channel = (outputs[A1_OUTPUT].getVoltage() + outputs[A2_OUTPUT].getVoltage() / 2.f);
        float b_channel = (outputs[B1_OUTPUT].getVoltage() + outputs[B2_OUTPUT].getVoltage() / 2.f);

        float a_low_channel = (inputs[FADE_A_LOW_INPUT].getVoltage());
        float b_low_channel = (inputs[FADE_B_LOW_INPUT].getVoltage());

        float xfade = params[FADE_PARAM].getValue();
        float xfade2 = params[FADE_LOW_PARAM].getValue();
        outputs[FADE_OUTPUT].setVoltage(((b_channel * (1 - xfade)) + (a_channel * xfade))*fade_level);
        outputs[FADE_LOW_OUTPUT].setVoltage((b_low_channel * (1 - xfade2)) + (a_low_channel * xfade2));

      }

      if (lightDivider.process())
      {
        float l = outputs[LFO_OUTPUT].getVoltage() / 10.f;
        lights[LFO_LIGHT].setBrightness(l);
        float a1 = outputs[A1_OUTPUT].getVoltage() / 10.f;
        lights[A1_LIGHT].setBrightness(a1);
        float a2 = outputs[A2_OUTPUT].getVoltage() / 10.f;
        lights[A2_LIGHT].setBrightness(a2);
        float b1 = outputs[B1_OUTPUT].getVoltage() / 10.f;
        lights[B1_LIGHT].setBrightness(b1);
        float b2 = outputs[B2_OUTPUT].getVoltage() / 10.f;
        lights[B2_LIGHT].setBrightness(b2);
        float fa = outputs[FADE_OUTPUT].getVoltage() / 10.f;
        lights[FADE_LIGHT].setBrightness(fa);
        float mu = outputs[MULT_OUTPUT].getVoltage() / 10.f;
        lights[MULT_LIGHT].setBrightness(mu);
        float fal = outputs[FADE_LOW_OUTPUT].getVoltage() / 10.f;
        lights[FADE_LOW_LIGHT].setBrightness(fal);
        float mul = outputs[MULT_LOW_OUTPUT].getVoltage() / 10.f;
        lights[MULT_LOW_LIGHT].setBrightness(mul);

      }

      if (swap_trigger.process(params[SWAP_PARAM].getValue()))
      {
        swap = !swap;
      }
      lights[SWAP_BUTTON_LIGHT].setSmoothBrightness(swap ? 1.0 : 0.0, args.sampleTime);

      if (swap_low_trigger.process(params[SWAP_LOW_PARAM].getValue()))
      {
        swap2 = !swap2;
      }
      lights[SWAP_LOW_BUTTON_LIGHT].setSmoothBrightness(swap2 ? 1.0 : 0.0, args.sampleTime);



      if (swap)
      {
        outputs[SWAP_A_OUTPUT].setVoltage(aswap_in);
        outputs[SWAP_B_OUTPUT].setVoltage(bswap_in);

        lights[SWAP_A_LIGHT + 0].setSmoothBrightness(-1, args.sampleTime * lightDivider.getDivision());
        lights[SWAP_A_LIGHT + 1].setSmoothBrightness(1, args.sampleTime * lightDivider.getDivision());
        lights[SWAP_A_LIGHT + 2].setBrightness(0.f);

        lights[SWAP_B_LIGHT + 0].setSmoothBrightness(1, args.sampleTime * lightDivider.getDivision());
        lights[SWAP_B_LIGHT + 1].setSmoothBrightness(-1, args.sampleTime * lightDivider.getDivision());
        lights[SWAP_B_LIGHT + 2].setBrightness(0.f);
        
      }
      else
      {
        outputs[SWAP_A_OUTPUT].setVoltage(bswap_in);
        outputs[SWAP_B_OUTPUT].setVoltage(aswap_in);

        lights[SWAP_A_LIGHT + 0].setSmoothBrightness(1, args.sampleTime * lightDivider.getDivision());
        lights[SWAP_A_LIGHT + 1].setSmoothBrightness(-1, args.sampleTime * lightDivider.getDivision());
        lights[SWAP_A_LIGHT + 2].setBrightness(0.f);

        lights[SWAP_B_LIGHT + 0].setSmoothBrightness(-1, args.sampleTime * lightDivider.getDivision());
        lights[SWAP_B_LIGHT + 1].setSmoothBrightness(1, args.sampleTime * lightDivider.getDivision());
        lights[SWAP_B_LIGHT + 2].setBrightness(0.f);
      }

      if (swap2)
      {
        outputs[SWAP_LOW_A_OUTPUT].setVoltage(low_aswap_in);
        outputs[SWAP_LOW_B_OUTPUT].setVoltage(low_bswap_in);

        lights[SWAP_LOW_A_LIGHT + 0].setSmoothBrightness(-1, args.sampleTime * lightDivider.getDivision());
        lights[SWAP_LOW_A_LIGHT + 1].setSmoothBrightness(1, args.sampleTime * lightDivider.getDivision());
        lights[SWAP_LOW_A_LIGHT + 2].setBrightness(0.f);

        lights[SWAP_LOW_B_LIGHT + 0].setSmoothBrightness(1, args.sampleTime * lightDivider.getDivision());
        lights[SWAP_LOW_B_LIGHT + 1].setSmoothBrightness(-1, args.sampleTime * lightDivider.getDivision());
        lights[SWAP_LOW_B_LIGHT + 2].setBrightness(0.f);
      }
      else
      {
       outputs[SWAP_LOW_A_OUTPUT].setVoltage(low_bswap_in);
       outputs[SWAP_LOW_B_OUTPUT].setVoltage(low_aswap_in);

        lights[SWAP_LOW_A_LIGHT + 0].setSmoothBrightness(1, args.sampleTime * lightDivider.getDivision());
        lights[SWAP_LOW_A_LIGHT + 1].setSmoothBrightness(-1, args.sampleTime * lightDivider.getDivision());
        lights[SWAP_LOW_A_LIGHT + 2].setBrightness(0.f);

        lights[SWAP_LOW_B_LIGHT + 0].setSmoothBrightness(-1, args.sampleTime * lightDivider.getDivision());
        lights[SWAP_LOW_B_LIGHT + 1].setSmoothBrightness(1, args.sampleTime * lightDivider.getDivision());
        lights[SWAP_LOW_B_LIGHT + 2].setBrightness(0.f);

      }
      

    }
  };

//////////////////////////////////////////////////////////////////
struct BenchWidget : ModuleWidget {
	
int lastPanelTheme = -1;
	std::shared_ptr<window::Svg> light_svg;
	std::shared_ptr<window::Svg> dark_svg;
	
	struct PanelThemeItem : MenuItem {
	  Bench *module;
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

	  Bench *module = dynamic_cast<Bench*>(this->module);
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
BenchWidget(Bench *module){
   setModule(module);
   
    // Main panels from Inkscape
 		light_svg = APP->window->loadSvg(asset::plugin(pluginInstance, "res/Light/Bench.svg"));
		dark_svg = APP->window->loadSvg(asset::plugin(pluginInstance, "res/Dark/Bench.svg"));
		int panelTheme = isDark(module ? (&(((Bench*)module)->panelTheme)) : NULL) ? 1 : 0;// need this here since step() not called for module browser
		setPanel(panelTheme == 0 ? light_svg : dark_svg);	
		

   //Screw

   int space=40;

  addChild(createWidget<ScrewBlack>(Vec(15, 0)));
  addChild(createWidget<ScrewBlack>(Vec(box.size.x-30, 0)));
  addChild(createWidget<ScrewBlack>(Vec(15, 365)));
  addChild(createWidget<ScrewBlack>(Vec(box.size.x-30, 365)));
   //

  addInput(createInput<PJ301MIPort>(Vec(15, 20),  module, Bench::MULT_INPUT));
  addInput(createInput<PJ301MCPort>(Vec(15+space*1, 20),  module, Bench::LFO_RESET_INPUT));
addOutput(createOutput<PJ301MOPort>(Vec(15+space*2, 20),  module, Bench::LFO_OUTPUT));;
  addInput(createInput<PJ301MIPort>(Vec(15+space*3, 20),  module, Bench::A1_INPUT));
  addInput(createInput<PJ301MIPort>(Vec(15+space*4, 20),  module, Bench::A2_INPUT));
  addInput(createInput<PJ301MIPort>(Vec(15+space*5, 20),  module, Bench::A_INPUT));
  addInput(createInput<PJ301MIPort>(Vec(15+space*6, 20),  module, Bench::B1_INPUT));
  addInput(createInput<PJ301MIPort>(Vec(15+space*7, 20),  module, Bench::B2_INPUT));
  addInput(createInput<PJ301MIPort>(Vec(15+space*8, 20),  module, Bench::SWAP_A_INPUT));


   addParam(createParam<SilverSwitch>(Vec(15+space*2,65), module, Bench::LFO_WAVE_PARAM));
addOutput(createOutput<PJ301MOPort>(Vec(15+space*3,  65),  module, Bench::A1_OUTPUT));
addOutput(createOutput<PJ301MOPort>(Vec(15+space*4,  65),  module, Bench::A2_OUTPUT));
  addInput(createInput<PJ301MIPort>(Vec(15+space*5,  65),  module, Bench::B_INPUT));
addOutput(createOutput<PJ301MOPort>(Vec(15+space*6,  65),  module, Bench::B1_OUTPUT));
addOutput(createOutput<PJ301MOPort>(Vec(15+space*7,  65),  module, Bench::B2_OUTPUT));
  addInput(createInput<PJ301MIPort>(Vec(15+space*8,  65),  module, Bench::SWAP_B_INPUT));



addParam(createParam<SilverSwitch>(Vec(15+space*1, 110), module, Bench::UNI_PARAM));
addParam(createParam<VerboS>(Vec(13 + space * 2, 107), module, Bench::RATE_PARAM));
addParam(createParam<SilverSwitch3>(Vec(15+space*3, 110), module, Bench::RANGE_A_PARAM));
addParam(createParam<VerboDS>(Vec(13 + space * 4, 107), module, Bench::LEVEL_PARAM));
addOutput(createOutput<PJ301MOPort>(Vec(15+space*5, 110),  module, Bench::FADE_OUTPUT));
addParam(createParam<SilverSwitch3>(Vec(15+space*6, 110), module, Bench::RANGE_B_PARAM));

addOutput(createOutput<PJ301MOPort>(Vec(15+space*8, 110),  module, Bench::SWAP_A_OUTPUT));
addChild(createLight<MediumLight<RedGreenBlueLight>>(Vec(5 + space * 8, 110), module, Bench::SWAP_A_LIGHT));

addOutput(createOutput<PJ301MOPort>(Vec(15+space*8, 155),  module, Bench::SWAP_B_OUTPUT));
addChild(createLight<MediumLight<RedGreenBlueLight>>(Vec(5 + space * 8, 155), module, Bench::SWAP_B_LIGHT));

addParam(createLightParam<LEDLightSliderFixed<GreenLight>>(Vec(17+space*1, 160), module, Bench::MULT_PARAM, Bench::MULT_LIGHT));
addParam(createLightParam<LEDLightSliderFixed<YellowLight>>(Vec(17+space*2, 160), module, Bench::LFO_PARAM, Bench::LFO_LIGHT));
addParam(createLightParam<LEDLightSliderFixed<YellowLight>>(Vec(17 + space * 3, 160), module, Bench::A1_PARAM, Bench::A1_LIGHT));
addParam(createLightParam<LEDLightSliderFixed<YellowLight>>(Vec(17 + space * 4, 160), module, Bench::A2_PARAM, Bench::A2_LIGHT));
addParam(createLightParam<LEDLightSliderFixed<GreenLight>>(Vec(17+space*5, 160), module, Bench::FADE_PARAM, Bench::FADE_LIGHT));
addParam(createLightParam<LEDLightSliderFixed<YellowLight>>(Vec(17 + space * 6, 160), module, Bench::B1_PARAM, Bench::B1_LIGHT));
addParam(createLightParam<LEDLightSliderFixed<YellowLight>>(Vec(17 + space * 7, 160), module, Bench::B2_PARAM, Bench::B2_LIGHT));

addParam(createLightParam<LEDLightBezel<RedLight>>(Vec(337,217), module, Bench::SWAP_PARAM,Bench::SWAP_BUTTON_LIGHT));

  for(int i=0;i<3;i++)
  {
  	addOutput(createOutput<PJ301MOPort>(Vec(15, 65 + (45*i)), module, Bench::MULT_OUTPUT+i));;
  }

  addParam(createParam<VerboS>(Vec(10, 215), module, Bench::VOLTAGE_PARAM));

////////////////////////////////////////LOWER PART//////////////////////////////////////////////

addInput(createInput<PJ301MIPort>(Vec(15, 275),  module, Bench::SWAP_LOW_A_INPUT));
addInput(createInput<PJ301MIPort>(Vec(15, 275+45),  module, Bench::SWAP_LOW_B_INPUT));
addOutput(createOutput<PJ301MOPort>(Vec(15+space, 275),  module, Bench::SWAP_LOW_A_OUTPUT));
addOutput(createOutput<PJ301MOPort>(Vec(15+space, 275+45),  module, Bench::SWAP_LOW_B_OUTPUT));
addInput(createInput<PJ301MIPort>(Vec(15+space*2, 275),  module, Bench::MULT_LOW_INPUT));
addOutput(createOutput<PJ301MOPort>(Vec(15+space*2, 275+45),  module, Bench::MULT_LOW_OUTPUT+0));
addOutput(createOutput<PJ301MOPort>(Vec(15+space*3, 275),  module, Bench::MULT_LOW_OUTPUT+1));
addOutput(createOutput<PJ301MOPort>(Vec(15+space*3, 275+45),  module, Bench::MULT_LOW_OUTPUT+2));
addInput(createInput<PJ301MIPort>(Vec(15+space*4, 275),  module, Bench::FADE_A_LOW_INPUT));
addInput(createInput<PJ301MIPort>(Vec(15+space*4, 275+45),  module, Bench::FADE_B_LOW_INPUT));
addOutput(createOutput<PJ301MOPort>(Vec(15+space*5, 275),  module, Bench::FADE_LOW_OUTPUT));

addParam(createLightParam<LEDLightBezel<RedLight>>(Vec(17+space*5,277+45), module, Bench::SWAP_LOW_PARAM,Bench::SWAP_LOW_BUTTON_LIGHT));


addParam(createParam<VerboDS>(Vec(25 + space * 6, 290), module, Bench::MULT_LOW_PARAM));
addChild(createLight<MediumLight<OrangeLight>>(Vec(55 + space * 6, 280), module, Bench::MULT_LOW_LIGHT));

addParam(createParam<VerboS>(Vec(15 + space * 7.5, 290), module, Bench::FADE_LOW_PARAM));
addChild(createLight<MediumLight<OrangeLight>>(Vec(45 + space * 7.5, 280), module, Bench::FADE_LOW_LIGHT));

addChild(createLight<MediumLight<RedGreenBlueLight>>(Vec(43 + space * 5, 275+48), module, Bench::SWAP_LOW_A_LIGHT));
addChild(createLight<MediumLight<RedGreenBlueLight>>(Vec(43 + space * 5, 290+48), module, Bench::SWAP_LOW_B_LIGHT));


}
void step() override {
		int panelTheme = isDark(module ? (&(((Bench*)module)->panelTheme)) : NULL) ? 1 : 0;
		if (lastPanelTheme != panelTheme) {
			lastPanelTheme = panelTheme;
			SvgPanel* panel = (SvgPanel*)getPanel();
			panel->setBackground(panelTheme == 0 ? light_svg : dark_svg);
		}
		Widget::step();
	}
};
Model *modelBench = createModel<Bench, BenchWidget>("Bench");
