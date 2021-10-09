///////////////////////////////////////////////////
//  dBiz Empty
// 
///////////////////////////////////////////////////

#include "plugin.hpp"

using namespace std;

/////added fine out /////////////////////////////////////////////////

struct Empty : Module {
    enum ParamIds
    {
  
        NUM_PARAMS
    };
    enum InputIds
    {

        NUM_INPUTS
    };
    enum OutputIds {
	
        NUM_OUTPUTS
};

    enum LighIds {
        ENUMS(AMOUNT_LIGHT, 3),
        NUM_LIGHTS
    };

  Empty() 
  {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
    panelTheme = (loadDarkAsDefault() ? 1 : 0);
  }

  json_t *dataToJson() override
  {
    json_t *rootJ = json_object();

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
  }

  void process(const ProcessArgs &args) override 
  {




  }
};

//////////////////////////////////////////////////////////////////
struct EmptyWidget : ModuleWidget 
{

  SvgPanel *darkPanel;
  struct PanelThemeItem : MenuItem
  {
    BenePads *module;
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

    BenePads *module = dynamic_cast<BenePads *>(this->module);
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

  EmptyWidget(Empty *module)
  {
    setModule(module);
    setModule(module);
    setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Light/BenePad.svg")));
    if (module)
    {
      darkPanel = new SvgPanel();
      darkPanel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Dark/BenePad.svg")));
      darkPanel->visible = false;
      addChild(darkPanel);
    }

    //Screw
    addChild(createWidget<ScrewBlack>(Vec(15, 0)));
    addChild(createWidget<ScrewBlack>(Vec(box.size.x - 30, 0)));
    addChild(createWidget<ScrewBlack>(Vec(15, 365)));
    addChild(createWidget<ScrewBlack>(Vec(box.size.x - 30, 365)));

    void step() override
    {
      if (module)
      {
        panel->visible = ((((BenePads *)module)->panelTheme) == 0);
        darkPanel->visible = ((((BenePads *)module)->panelTheme) == 1);
      }
      Widget::step();
    }
};
Model *modelEmpty = createModel<Empty, EmptyWidget>("Empty");
