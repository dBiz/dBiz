#include "rack.hpp"

#include "app/SvgKnob.hpp"
#include "app/SvgSlider.hpp"
#include "app/SvgPort.hpp"
#include "app/ModuleLightWidget.hpp"
#include "app/SvgSwitch.hpp"
#include "app/SvgScrew.hpp"
#include "asset.hpp"

using namespace rack;

extern Plugin *pluginInstance;

extern Model *modelNavControl;
extern Model *modelBench;
extern Model *modelContorno;
// extern Model *modelContornoMK2;
extern Model *modelTranspose;
extern Model *modelUtility;
extern Model *modelChord;
extern Model *modelBene;
extern Model *modelBenePads;
extern Model *modelPerfMixer;
extern Model *modelDrMix;
extern Model *modelPerfMixer4;
extern Model *modelRemix;
extern Model *modelSmixer;
extern Model *modelVCA4;
extern Model *modelVCA530;
extern Model *modelVerbo;
extern Model *modelDVCO;
extern Model *modelDAOSC;
extern Model *modelTROSC;
extern Model *modelTROSCMK2;
extern Model *modelSuHa;
extern Model *modelSuHaMK2;
extern Model *modelFourSeq;
extern Model *modelDivider;
extern Model *modelUtil2;
extern Model *modelSmorph;
extern Model *modelBigSmorph;
extern Model *modelSPan;
extern Model *modelQuePasa;
extern Model *modelDualFilter;
extern Model *modelOrder;
extern Model *modelDualMatrix;

///////////////////////////////////////////////////////////////////////

 static const std::string lightPanelID = "Light";
 static const std::string darkPanelID = "Dark";

// ******** Panel Theme management ********


void saveDarkAsDefault(bool darkAsDefault);
bool loadDarkAsDefault();

bool isDark(int* panelTheme);

void writeDarkAsDefault();
void readDarkAsDefault();

struct DarkDefaultItem : MenuItem {
	void onAction(const event::Action &e) override {
		saveDarkAsDefault(rightText.empty());// implicitly toggled
	}
};	

///////////////////////////////////////////////////////////////////////////

template <typename TLightBase = RedLight>
struct LEDLightSliderFixed : LEDLightSlider<TLightBase> {
	LEDLightSliderFixed() {
		this->setHandleSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/component/LEDSliderHandle.svg")));
	}
};


////////////////////
// Colors
///// ///////////////

static const NVGcolor BLACK_TRANSPARENT = nvgRGBA(0x00, 0x00, 0x00, 0x00);
static const NVGcolor BLACK = nvgRGB(0x00, 0x00, 0x00);
static const NVGcolor WHITE = nvgRGB(0xff, 0xff, 0xff);
static const NVGcolor RED = nvgRGB(0xed, 0x2c, 0x24);
static const NVGcolor ORANGE = nvgRGB(0xf2, 0xb1, 0x20);
static const NVGcolor YELLOW = nvgRGB(0xf9, 0xdf, 0x1c);
static const NVGcolor GREEN = nvgRGB(0x90, 0xc7, 0x3e);
static const NVGcolor CYAN = nvgRGB(0x22, 0xe6, 0xef);
static const NVGcolor BLUE = nvgRGB(0x29, 0xb2, 0xef);
static const NVGcolor PURPLE = nvgRGB(0xd5, 0x2b, 0xed);
static const NVGcolor LIGHT_GRAY = nvgRGB(0xe6, 0xe6, 0xe6);
static const NVGcolor DARK_GRAY = nvgRGB(0x17, 0x17, 0x17);


 ////////////////////
// Knobs
////////////////////



struct Trim : app::SVGKnob
{
	widget::SvgWidget* bg;
	Trim()
	{
		minAngle = -0.80 * M_PI;
		maxAngle = 0.80 * M_PI;

		bg = new widget::SvgWidget;
		fb->addChildBelow(bg, tw);

		setSvg(Svg::load(asset::plugin(pluginInstance,"res/component/Trim.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance,"res/component/Trim-bg.svg")));
	}
};

struct VAKnob : app::SVGKnob
{
	widget::SvgWidget* bg;
	widget::SvgWidget* fg;

	VAKnob()
	{
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;

		bg = new widget::SvgWidget;
		fb->addChildBelow(bg, tw);

		fg = new widget::SvgWidget;
		fb->addChildAbove(fg, tw);
	}
};

 struct DKnob : VAKnob
 {
	DKnob()
	{
		setSvg(Svg::load(asset::plugin(pluginInstance,"res/component/DKnob.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance,"res/component/DKnob-bg.svg")));
		fg->setSvg(Svg::load(asset::plugin(pluginInstance,"res/component/DKnob-cap.svg")));
	}
};

struct SDKnob : VAKnob
{
	SDKnob()
	{
		setSvg(Svg::load(asset::plugin(pluginInstance,"res/component/SDKnob.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance,"res/component/SDKnob-bg.svg")));
		fg->setSvg(Svg::load(asset::plugin(pluginInstance,"res/component/SDKnob-cap.svg")));
	}
};

struct SDKnobSnap : SDKnob
{

	SDKnobSnap()
	{
		snap = true;
	}
};

struct VerboLarge : app::SVGKnob
{
	widget::SvgWidget* bg;

	VerboLarge()
	{
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;

		bg = new widget::SvgWidget;
		fb->addChildBelow(bg, tw);		
	}
};

struct VerboR : VerboLarge
{
	VerboR()
	{
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;

		setSvg(Svg::load(asset::plugin(pluginInstance,"res/component/VerboL.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance,"res/component/VerboR-bg.svg")));
	}
};

struct VerboL : VerboLarge
{
	VerboL()
	{
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;

		setSvg(Svg::load(asset::plugin(pluginInstance,"res/component/VerboL.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance,"res/component/VerboL-bg.svg")));
	}
};

struct VerboDL : VerboLarge
{
	VerboDL()
	{
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;

		setSvg(Svg::load(asset::plugin(pluginInstance,"res/component/VerboL.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance,"res/component/VerboDL.svg")));
	}
};

struct VerboSmall : app::SVGKnob
{
	widget::SvgWidget* bg;
	widget::SvgWidget* fg;

	VerboSmall()
	{
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;

		bg = new widget::SvgWidget;
		fb->addChildBelow(bg, tw);

		fg = new widget::SvgWidget;
		fb->addChildAbove(fg, tw);
	}
};

struct VerboDS : VerboSmall
{
	VerboDS()
	{

		setSvg(Svg::load(asset::plugin(pluginInstance,"res/component/VerboDS.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance,"res/component/VerboDS-bg.svg")));
		fg->setSvg(Svg::load(asset::plugin(pluginInstance,"res/component/VerboDS-cap.svg")));
	}
};

struct VerboRS : VerboSmall
{
	VerboRS()
	{

		setSvg(Svg::load(asset::plugin(pluginInstance,"res/component/VerboRS.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance,"res/component/VerboDS-bg.svg")));
		fg->setSvg(Svg::load(asset::plugin(pluginInstance,"res/component/VerboRS-cap.svg")));
	}
};

struct VerboDSSnapKnob : VerboDS
{
	VerboDSSnapKnob()
	{
		snap = true;
	};
};


struct VerboS : VerboSmall
{
	VerboS()
	{
		setSvg(Svg::load(asset::plugin(pluginInstance,"res/component/VerboS.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance,"res/component/VerboS-bg.svg")));
		fg->setSvg(Svg::load(asset::plugin(pluginInstance,"res/component/VerboS-cap.svg")));
	}
};

struct VerboXS : app::SVGKnob
{
	widget::SvgWidget* bg;
	widget::SvgWidget* fg;

	VerboXS()
	{
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;

		bg = new widget::SvgWidget;
		fb->addChildBelow(bg, tw);

		fg = new widget::SvgWidget;
		fb->addChildAbove(fg, tw);

		setSvg(Svg::load(asset::plugin(pluginInstance,"res/component/VerboXS.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance,"res/component/VerboXS-bg.svg")));
		fg->setSvg(Svg::load(asset::plugin(pluginInstance,"res/component/VerboXS-cap.svg")));
	}
};


struct VerboXDS : app::SVGKnob
{
	VerboXDS()
	{
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;

		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/component/VerboXDS.svg")));
		box.size = Vec(50, 50);
	}
};


struct SmallKnob : app::SVGKnob
{
	widget::SvgWidget* bg;
	widget::SvgWidget* fg;

	SmallKnob()
	{
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;

		bg = new widget::SvgWidget;
		fb->addChildBelow(bg, tw);

		fg = new widget::SvgWidget;
		fb->addChildAbove(fg, tw);

	}
};


struct MicroBlu : SmallKnob
{
	MicroBlu()
	{
		setSvg(Svg::load(asset::plugin(pluginInstance,"res/component/SmallBlu.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance,"res/component/Small-bg.svg")));
		fg->setSvg(Svg::load(asset::plugin(pluginInstance,"res/component/SmallBlu-cap.svg")));
	}
};


struct MicroBluSnapKnob : MicroBlu
{
	MicroBluSnapKnob()
	{
		snap = true;
	};
};


struct SmallBla : SmallKnob
{
	SmallBla()
	{
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/component/SmallBla.svg")));
	}
};

struct LargeBla : SmallBla
{
	LargeBla()
	{
		box.size = Vec(45, 45);
	}
};

struct DaviesKnob : app::SVGKnob
{
	widget::SvgWidget* bg;
	widget::SvgWidget* fg;

	DaviesKnob()
	{
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
		
		bg = new widget::SvgWidget;
		fb->addChildBelow(bg, tw);

	}
};

struct RoundAzz : DaviesKnob
{
	RoundAzz()
	{
		setSvg(Svg::load(asset::plugin(pluginInstance,"res/component/Round.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance,"res/component/RoundAzz-bg.svg")));
		
	}
};
struct RoundRed : DaviesKnob
{
	RoundRed()
	{
		setSvg(Svg::load(asset::plugin(pluginInstance,"res/component/Round.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance,"res/component/RoundRed-bg.svg")));
	}
};

struct RoundWhy : DaviesKnob
{
	RoundWhy()
	{
		setSvg(Svg::load(asset::plugin(pluginInstance,"res/component/RoundWhite.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance,"res/component/RoundWhite-bg.svg")));
	}
};

struct RoundWhySnapKnob : RoundWhy
{
	RoundWhySnapKnob()
	{
		snap = true;
	};
};

struct LRoundWhy : RoundWhy
{
	LRoundWhy()
	{
		setSvg(Svg::load(asset::plugin(pluginInstance,"res/component/LRoundWhite.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance,"res/component/LRoundWhite-bg.svg")));
	}
};
struct HRoundWhy : SmallKnob
{
	HRoundWhy()
	{

		setSvg(Svg::load(asset::plugin(pluginInstance, "res/component/HRoundWhite.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/component/HRoundWhite-bg.svg")));
		fg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/component/HRoundWhite-fg.svg")));
	}
};

struct RoundBlu : DaviesKnob
{
	RoundBlu()
	{
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/component/RoundBlu.svg")));
	}
};

struct LRoundBlu : RoundBlu
{
	LRoundBlu()
	{
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/component/LRoundBlu.svg")));
	}
};

struct FlatA : SmallKnob
{
	FlatA()
	{
		setSvg(Svg::load(asset::plugin(pluginInstance,"res/component/FlatA.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance,"res/component/FlatA-bg.svg")));
		fg->setSvg(Svg::load(asset::plugin(pluginInstance,"res/component/Flat-cap.svg")));
	}
};
struct FlatASnap : FlatA
{

	FlatASnap()
	{
		snap = true;
	}
};

struct FlatR : SmallKnob
{
	FlatR()
	{
		setSvg(Svg::load(asset::plugin(pluginInstance,"res/component/FlatR.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance,"res/component/FlatR-bg.svg")));
		fg->setSvg(Svg::load(asset::plugin(pluginInstance,"res/component/Flat-cap.svg")));
	}
};

struct FlatG : SmallKnob
{
	FlatG()
	{
		setSvg(Svg::load(asset::plugin(pluginInstance,"res/component/FlatG.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance,"res/component/FlatG-bg.svg")));
		fg->setSvg(Svg::load(asset::plugin(pluginInstance,"res/component/Flat-cap.svg")));
	}
};

struct FlatGSnap : FlatG
{

	FlatGSnap()
	{
		snap = true;
	}
};


//////////////////////
//slider
///////////////////

struct SlidePot : app::SvgSlider
{
	SlidePot()
	{
		math::Vec margin = math::Vec(3.5, 3.5);
		maxHandlePos = math::Vec(-1, -2).plus(margin);
		minHandlePos = math::Vec(-1, 87).plus(margin);
		setBackgroundSvg(APP->window->loadSvg(asset::plugin(pluginInstance,"res/component/SlidePot.svg")));
		setHandleSvg(APP->window->loadSvg(asset::plugin(pluginInstance,"res/component/SlidePotHandle.svg")));
		background->box.pos = margin;
		box.size = background->box.size.plus(margin.mult(2));
	}
};

struct SlidePot2 : app::SvgSlider
{
	SlidePot2()
	{
		math::Vec margin = math::Vec(3.5, 3.5);
		maxHandlePos = math::Vec(-10, -2).plus(margin);
		minHandlePos = math::Vec(-10, 87).plus(margin);
		setBackgroundSvg(APP->window->loadSvg(asset::plugin(pluginInstance,"res/component/SlidePot.svg")));
		setHandleSvg(APP->window->loadSvg(asset::plugin(pluginInstance,"res/component/SlidePotHandle2.svg")));
		background->box.pos = margin;
		box.size = background->box.size.plus(margin.mult(2));
	}
};

struct SlidePotR : app::SvgSlider
{
	SlidePotR()
	{
		math::Vec margin = math::Vec(3.5, 3.5);
		maxHandlePos = math::Vec(-1, -2).plus(margin);
		minHandlePos = math::Vec(-1, 87).plus(margin);
		setBackgroundSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/component/SlidePot.svg")));
		setHandleSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/component/SlidePotHandleR.svg")));
		background->box.pos = margin;
		box.size = background->box.size.plus(margin.mult(2));
	}
};

struct SlidePotL : app::SvgSlider
{
	SlidePotL()
	{
		math::Vec margin = math::Vec(3.5, 3.5);
		maxHandlePos = math::Vec(-10, -2).plus(margin);
		minHandlePos = math::Vec(-10, 137).plus(margin);
		setBackgroundSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/component/SlidePotL.svg")));
		setHandleSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/component/SlidePotHandle2.svg")));
		background->box.pos = margin;
		box.size = background->box.size.plus(margin.mult(2));
	}
};

////////////////////
// Lights
////////////////////


struct OrangeLight : GrayModuleLightWidget
{
    OrangeLight()
    {
        addBaseColor(ORANGE);
    }
};

struct CyanLight : GrayModuleLightWidget
{
	CyanLight()
	{
		addBaseColor(CYAN);
	}
};

struct PurpleLight : GrayModuleLightWidget
{
	PurpleLight()
	{
		addBaseColor(PURPLE);
	}
};

struct WhitheLight : GrayModuleLightWidget
{
	WhitheLight()
	{
		addBaseColor(WHITE);
	}
};




template <typename BASE>
struct BigLight : BASE
{
	BigLight()
	{
		this->borderColor = color::BLACK_TRANSPARENT;
		this->bgColor = color::BLACK_TRANSPARENT;
		this->box.size = Vec(20, 20);
	}
};

template <typename BASE>
struct HugeLight : BASE
{
	HugeLight()
	{
		this->borderColor = color::BLACK_TRANSPARENT;
		this->bgColor = color::BLACK_TRANSPARENT;
		this->box.size = Vec(24, 24);
	}
};




////////////////////
// Jacks
////////////////////

struct PJ301MRPort : SVGPort
{
	PJ301MRPort()
	{
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/component/PJ301MR.svg")));
	}
};

struct PJ301MLPort : SVGPort
{
	PJ301MLPort()
	{
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/component/PJ301ML.svg")));
	}
};

struct PJ301MIPort : SVGPort
{
	PJ301MIPort()
	{
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/component/PJ301MA.svg")));
	}
};

struct PJ301MVAPort : SVGPort
{
	PJ301MVAPort()
	{
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/component/PJ301MVA.svg")));
	}
};

struct PJ301MOrPort : SVGPort
{
	PJ301MOrPort()
	{
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/component/PJ301MO.svg")));
	}
};

struct PJ301MOPort : SVGPort
{
	PJ301MOPort()
	{
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/component/PJ301MB.svg")));
	}
};

struct PJ301MCPort : SVGPort
{
	PJ301MCPort()
	{
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/component/PJ301MW.svg")));
	}
};

struct PJ301MBPort : SVGPort
{
	PJ301MBPort()
	{
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/component/PJ3410.svg")));
	}
};


//
////////////////////////
//  SWITCHES
////////////////////////

struct SilverSwitch : app::SVGSwitch
{
	SilverSwitch()
	{
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance,"res/component/SilverSwitch_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance,"res/component/SilverSwitch_2.svg")));
	}
};

struct SilverSwitch3 : app::SVGSwitch
{
	SilverSwitch3()
	{
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance,"res/component/SilverSwitch_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance,"res/component/SilverSwitch_1.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance,"res/component/SilverSwitch_2.svg")));
	}
};

struct CKSSS : app::SVGSwitch
{
	CKSSS()
	{
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance,"res/component/CKSS_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance,"res/component/CKSS_1.svg")));
	}
};

struct LEDB : app::SVGSwitch
{
	LEDB()
	{
		momentary = true;
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance,"res/component/LEDB_0.svg")));
	}
};

struct LEDB2 : app::SVGSwitch
{
	LEDB2()
	{
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance,"res/component/LEDB_0.svg")));
	}
};

struct BLEDB : app::SVGSwitch
{
	BLEDB()
	{
		momentary = true;
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/component/BLEDB_0.svg")));
	}
};

struct LEDT : app::SVGSwitch
{
	LEDT()
	{
		momentary = true;
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance,"res/component/LEDS_0.svg")));
	}
};

struct MCKSSS : app::SVGSwitch
{
	MCKSSS()
	{
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance,"res/component/MCKSSS_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance,"res/component/MCKSSS_1.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance,"res/component/MCKSSS_2.svg")));
	}
};

struct MCKSSS2 : app::SVGSwitch
{
	MCKSSS2()
	{
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance,"res/component/MCKSSS_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance,"res/component/MCKSSS_2.svg")));
	}
};

struct BPush : app::SVGSwitch
{
	BPush()
	{
		momentary = true;
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance,"res/component/BPush_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance,"res/component/BPush_1.svg")));
	}
};
struct BPushR : app::SVGSwitch
{
	BPushR()
	{
		momentary = true;
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/component/BPushR_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/component/BPushR_1.svg")));
	}
};
