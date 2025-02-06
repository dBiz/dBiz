
#include "plugin.hpp"

using namespace std;



struct Order : Module {
  enum ParamIds
  {
    G1_PARAM,
    G2_PARAM,
    G3_PARAM,
    G4_PARAM,
    G1_POLAR_PARAM,
    G2_POLAR_PARAM,
    G3_POLAR_PARAM,
    G4_POLAR_PARAM,
    G1_SHM_PARAM,
    G2_SHM_PARAM,
    G3_SHM_PARAM,
    G4_SHM_PARAM,
    G1_SHP_PARAM,
    G2_SHP_PARAM,
    G3_SHP_PARAM,
    G4_SHP_PARAM,
    ENUMS(G1_ATT_PARAM, 4),
    ENUMS(G2_ATT_PARAM, 4),
    ENUMS(G3_ATT_PARAM, 4),
    ENUMS(G4_ATT_PARAM, 4),
    MASTER_PARAM,
    MASTER_SHP_PARAM,
    MASTER_SHM_PARAM,

    NUM_PARAMS

  };
  enum InputIds
  {
    G1_INPUT,
    G2_INPUT,
    G3_INPUT,
    G4_INPUT,
    MASTER_INPUT,
    MASTER_SCALE,
    INPUT,
    NUM_INPUTS
  };
  enum OutputIds
  {
    ENUMS(G1_ATT_OUTPUT, 4),
    ENUMS(G2_ATT_OUTPUT, 4),
    ENUMS(G3_ATT_OUTPUT, 4),
    ENUMS(G4_ATT_OUTPUT, 4),
    NUM_OUTPUTS
  };

  enum LighIds
  {
    G1_LIGHT,
    G2_LIGHT,
    G3_LIGHT,
    G4_LIGHT,
    NUM_LIGHTS
  };

  int panelTheme;
  
  //////////////////////////////		
  
  void processGroup(float &G, float Sp, float Sm, float att[], float Out[], bool polarity, int attOutputId, float MSP, float MSM, float MASTER) {
    // Gestione della polarità
    if (polarity) {
        G = rescale(G, -5.0, 5.0, 0.0, 10.0);
    } else {
        G = -G;  // Inversione completa della polarità
    }

    // Calcolo per ogni attenuatore
    for (int i = 0; i < 4; i++) {
        Out[i] = (((att[i] + Sm + Sp) * G) / 5.0f);
        if (outputs[attOutputId + i].isConnected()) {
            float output = (Out[i] + MSP + MSM) * MASTER;
            outputs[attOutputId + i].setVoltage(clamp(output, -10.0f, 10.0f));
        }
    }
}

///////////////////////////////////
  

  Order()
  {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

    configParam(G1_PARAM, 0.0, 1.0, 0.0, "Group A Level", "%", 0, 100);
    configParam(G1_SHP_PARAM, 0.0, 10.0, 0.0, "Group A Shift Positive");
    configParam(G1_SHM_PARAM, -10.0, 0.0, 0, "Group A Shift Negative");
    configParam(G2_PARAM, 0.0, 1.0, 0.0, "Group B Level", "%", 0, 100);
    configParam(G2_SHP_PARAM, 0.0, 10.0, 0.0, "Group B Shift Positive");
    configParam(G2_SHM_PARAM, -10.0, 0.0, 0, "Group B Shift Negative");
    configParam(G3_PARAM, 0.0, 1.0, 0.0, "Group C Level", "%", 0, 100);
    configParam(G3_SHP_PARAM, 0.0, 10.0, 0.0, "Group C Shift Positive");
    configParam(G3_SHM_PARAM, -10.0, 0.0, 0, "Group C Shift Negative");
    configParam(G4_PARAM, 0.0, 1.0, 0.0, "Group D Level", "%", 0, 100);
    configParam(G4_SHP_PARAM, 0.0, 10.0, 0.0, "Group D Shift Positive");
    configParam(G4_SHM_PARAM, -10.0, 0.0, 0, "Group D Shift Negative");

    configParam(MASTER_PARAM, 0.0, 1, 0, "Master", "%", 0, 100);
    configParam(MASTER_SHP_PARAM, 0.0, 10.0, 0, "Master Shift Positive");
    configParam(MASTER_SHM_PARAM, -10.0, 0.0, 0, "Master Shift Negative");

    configParam(G1_POLAR_PARAM, 0.f,1.f,0.f, "Group A Polarity");
    configParam(G2_POLAR_PARAM, 0.f,1.f,0.f, "Group B Polarity");
    configParam(G3_POLAR_PARAM, 0.f,1.f,0.f, "Group C Polarity");
    configParam(G4_POLAR_PARAM, 0.f,1.f,0.f, "Group D Polarity");
	
	configInput(G1_INPUT,"Group A");
    configInput(G2_INPUT,"Group B");
    configInput(G3_INPUT,"Group C");
    configInput(G4_INPUT,"Group D");
    configInput(MASTER_INPUT,"Master Cv");
    configInput(MASTER_SCALE,"Master Range Cv");
    configInput(INPUT,"Input");


    for (int i = 0; i < 4; i++)
    {
        configParam(G1_ATT_PARAM + i, -5.0, 5.0, 0, string::f("Group A%d Att",i+1));
        configParam(G2_ATT_PARAM + i, -5.0, 5.0, 0, string::f("Group B%d Att",i+1));
        configParam(G3_ATT_PARAM + i, -5.0, 5.0, 0, string::f("Group C%d Att",i+1));
        configParam(G4_ATT_PARAM + i, -5.0, 5.0, 0, string::f("Group D%d Att",i+1));
		
		configOutput(G1_ATT_OUTPUT+i,string::f("Group A%d",i+1));
		configOutput(G2_ATT_OUTPUT+i,string::f("Group B%d",i+1));
		configOutput(G3_ATT_OUTPUT+i,string::f("Group C%d",i+1));
		configOutput(G4_ATT_OUTPUT+i,string::f("Group D%d",i+1));
    }
    //onReset();

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
    
      bool pol1 = params[G1_POLAR_PARAM].getValue() > 0.f;
      bool pol2 = params[G2_POLAR_PARAM].getValue() > 0.f;
      bool pol3 = params[G3_POLAR_PARAM].getValue() > 0.f;
      bool pol4 = params[G4_POLAR_PARAM].getValue() > 0.f;

      float G1 = 0.f, G2 = 0.f, G3 = 0.f, G4 = 0.f;

      float Sp1 = 0.f, Sp2 = 0.f, Sp3 = 0.f, Sp4 = 0.f;
      float Sm1 = 0.f, Sm2 = 0.f, Sm3 = 0.f, Sm4 = 0.f;

      float att1[4] = {};
      float att2[4] = {};
      float att3[4] = {};
      float att4[4] = {};

      float Out1[4] = {};
      float Out2[4] = {};
      float Out3[4] = {};
      float Out4[4] = {};

      Sp1 = params[G1_SHP_PARAM].getValue();
      Sm1 = params[G1_SHM_PARAM].getValue();
      Sp2 = params[G2_SHP_PARAM].getValue();
      Sm2 = params[G2_SHM_PARAM].getValue();
      Sp3 = params[G3_SHP_PARAM].getValue();
      Sm3 = params[G3_SHM_PARAM].getValue();
      Sp4 = params[G4_SHP_PARAM].getValue();
      Sm4 = params[G4_SHM_PARAM].getValue();

      G1 = params[G1_PARAM].getValue();
      if(inputs[G1_INPUT].isConnected()) G1*=inputs[G1_INPUT].getVoltage();
      G2 = params[G2_PARAM].getValue();
      if(inputs[G2_INPUT].isConnected()) G2*=inputs[G2_INPUT].getVoltage();
      G3 = params[G3_PARAM].getValue();
      if(inputs[G3_INPUT].isConnected()) G3*=inputs[G3_INPUT].getVoltage();
      G4 = params[G4_PARAM].getValue();
      if(inputs[G4_INPUT].isConnected()) G4*=inputs[G4_INPUT].getVoltage();

      float MSP = params[MASTER_SHP_PARAM].getValue();
	  float MSM = params[MASTER_SHM_PARAM].getValue();
	  float MASTER = params[MASTER_PARAM].getValue();
	  if (inputs[MASTER_INPUT].isConnected()) {
		MASTER *= inputs[MASTER_INPUT].getVoltage();
	  }


      for (int i = 0; i < 4; i++)
      {
        att1[i] = params[G1_ATT_PARAM + i].getValue();
        att2[i] = params[G2_ATT_PARAM + i].getValue();
        att3[i] = params[G3_ATT_PARAM + i].getValue();
        att4[i] = params[G4_ATT_PARAM + i].getValue();
      }

        lights[G1_LIGHT].setBrightness(pol1);
        lights[G2_LIGHT].setBrightness(pol2);
        lights[G3_LIGHT].setBrightness(pol3);
        lights[G4_LIGHT].setBrightness(pol4);
		
	processGroup(G1, Sp1, Sm1, att1, Out1, pol1, G1_ATT_OUTPUT, MSP, MSM, MASTER);
	processGroup(G2, Sp2, Sm2, att2, Out2, pol2, G2_ATT_OUTPUT, MSP, MSM, MASTER);
	processGroup(G3, Sp3, Sm3, att3, Out3, pol3, G3_ATT_OUTPUT, MSP, MSM, MASTER);
	processGroup(G4, Sp4, Sm4, att4, Out4, pol4, G4_ATT_OUTPUT, MSP, MSM, MASTER);



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
struct OrderWidget : ModuleWidget
{

    int lastPanelTheme = -1;
	std::shared_ptr<window::Svg> light_svg;
	std::shared_ptr<window::Svg> dark_svg;
	
  struct PanelThemeItem : MenuItem {
    Order *module;
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

    Order *module = dynamic_cast<Order*>(this->module);
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
OrderWidget(Order *module){
  setModule(module);
  // Main panels from Inkscape
 		light_svg = APP->window->loadSvg(asset::plugin(pluginInstance, "res/Light/Order.svg"));
		dark_svg = APP->window->loadSvg(asset::plugin(pluginInstance, "res/Dark/Order.svg"));
		int panelTheme = isDark(module ? (&(((Order*)module)->panelTheme)) : NULL) ? 1 : 0;// need this here since step() not called for module browser
		setPanel(panelTheme == 0 ? light_svg : dark_svg);	

int knob= 40;
int jack=35;

addParam(createParam<VerboR>(Vec(250, 225 ), module, Order::MASTER_SHM_PARAM));
addParam(createParam<RoundAzz>(Vec(305,300), module, Order::MASTER_SHP_PARAM));
addParam(createParam<RoundWhy>(Vec(15, 280), module, Order::MASTER_PARAM));

addParam(createParam<FlatA>(Vec(60, 30), module, Order::G1_PARAM));
addParam(createParam<FlatA>(Vec(60, 30 + 45 * 1), module, Order::G2_PARAM));
addParam(createParam<FlatA>(Vec(60, 30 + 45 * 2), module, Order::G3_PARAM));
addParam(createParam<FlatA>(Vec(60, 30 + 45 * 3), module, Order::G4_PARAM));

addParam(createParam<FlatR>(Vec(110 + knob * 4, 30), module, Order::G1_SHM_PARAM));
addParam(createParam<FlatR>(Vec(110 + knob * 4, 30 + 45 * 1), module, Order::G2_SHM_PARAM));
addParam(createParam<FlatR>(Vec(110 + knob * 4, 30 + 45 * 2), module, Order::G3_SHM_PARAM));
addParam(createParam<FlatR>(Vec(110 + knob * 4, 30 + 45 * 3), module, Order::G4_SHM_PARAM));
 
addParam(createParam<FlatG>(Vec(110 + knob * 5, 30), module, Order::G1_SHP_PARAM));
addParam(createParam<FlatG>(Vec(110 + knob * 5, 30 + 45 * 1), module, Order::G2_SHP_PARAM));
addParam(createParam<FlatG>(Vec(110 + knob * 5, 30 + 45 * 2), module, Order::G3_SHP_PARAM));
addParam(createParam<FlatG>(Vec(110 + knob * 5, 30 + 45 * 3), module, Order::G4_SHP_PARAM));

addInput(createInput<PJ301MSPort>(Vec(17 ,32), module, Order::G1_INPUT));
addInput(createInput<PJ301MSPort>(Vec(17, 32 + (45 * 1)), module, Order::G2_INPUT));
addInput(createInput<PJ301MSPort>(Vec(17, 32 + (45 * 2)), module, Order::G3_INPUT));
addInput(createInput<PJ301MSPort>(Vec(17, 32 + (45 * 3)), module, Order::G4_INPUT));

addInput(createInput<PJ301MSPort>(Vec(20, 30 + ((jack + 5) * 5)), module, Order::MASTER_INPUT));

for (int i = 0; i < 4; i++)
{
    addParam(createParam<FlatA>(Vec(105 + knob * i, 30), module, Order::G1_ATT_PARAM + i));
    addParam(createParam<FlatA>(Vec(105 + knob * i, 30 + 45 * 1), module, Order::G2_ATT_PARAM + i));
    addParam(createParam<FlatA>(Vec(105 + knob * i, 30 + 45 * 2), module, Order::G3_ATT_PARAM + i));
    addParam(createParam<FlatA>(Vec(105 + knob * i, 30 + 45 * 3), module, Order::G4_ATT_PARAM + i));

    addOutput(createOutput<PJ301MSPort>(Vec(100 + jack * i, 211), module, Order::G1_ATT_OUTPUT + i));
    addOutput(createOutput<PJ301MSPort>(Vec(100 + jack * i, 211 + jack * 1), module, Order::G2_ATT_OUTPUT + i));
    addOutput(createOutput<PJ301MSPort>(Vec(100 + jack * i, 211 + jack * 2), module, Order::G3_ATT_OUTPUT + i));
    addOutput(createOutput<PJ301MSPort>(Vec(100 + jack * i, 211 + jack * 3), module, Order::G4_ATT_OUTPUT + i));
}

addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<OrangeLight>>>(Vec(78, 225), module, Order::G1_POLAR_PARAM, Order::G1_LIGHT));
addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<OrangeLight>>>(Vec(78, 225+ jack * 1), module, Order::G2_POLAR_PARAM, Order::G2_LIGHT));
addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<OrangeLight>>>(Vec(78, 225+ jack * 2), module, Order::G3_POLAR_PARAM, Order::G3_LIGHT));
addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<OrangeLight>>>(Vec(78, 225+ jack * 3), module, Order::G4_POLAR_PARAM, Order::G4_LIGHT));

//Screw
addChild(createWidget<ScrewBlack>(Vec(15, 0)));
addChild(createWidget<ScrewBlack>(Vec(box.size.x - 30, 0)));
addChild(createWidget<ScrewBlack>(Vec(15, 365)));
addChild(createWidget<ScrewBlack>(Vec(box.size.x - 30, 365)));


}
void step() override {
		int panelTheme = isDark(module ? (&(((Order*)module)->panelTheme)) : NULL) ? 1 : 0;
		if (lastPanelTheme != panelTheme) {
			lastPanelTheme = panelTheme;
			SvgPanel* panel = (SvgPanel*)getPanel();
			panel->setBackground(panelTheme == 0 ? light_svg : dark_svg);
		}
		Widget::step();
	}
};
Model *modelOrder = createModel<Order, OrderWidget>("Order");
