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

extern Model *modelMultiple;
extern Model *modelContorno;
extern Model *modelTranspose;
extern Model *modelUtility;
extern Model *modelChord;  
extern Model *modelBene;  
extern Model *modelBenePads;
extern Model *modelPerfMixer;
extern Model *modelRemix;
extern Model *modelSmixer;
extern Model *modelVCA4;
extern Model *modelVCA530;
extern Model *modelVerbo;
extern Model *modelDVCO; 
extern Model *modelDAOSC;
extern Model *modelTROSC;
extern Model *modelSuHa;
extern Model *modelFourSeq;
extern Model *modelDivider;
extern Model *modelUtil2;
extern Model *modelSmorph;

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

struct DKnob : app::SVGKnob
{
	DKnob()
	{   
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/component/DKnob.svg")));
		box.size = Vec(50, 50);
	}
}; 

struct SDKnob : app::SVGKnob
{
	SDKnob()
	{
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/component/SDKnob.svg")));
		box.size = Vec(30, 30);
	}
};

struct VerboL : app::SVGKnob
{
	VerboL()
	{
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/component/VerboL.svg")));
		box.size = Vec(80, 80);
	}
};

struct VerboS : app::SVGKnob
{
	VerboS()
	{
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/component/VerboS.svg")));
		box.size = Vec(35, 35);
	}
};

struct VerboDL : app::SVGKnob
{
	VerboDL()
	{
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/component/VerboDL.svg")));
		box.size = Vec(80, 80);
	}
};

struct VerboDS : app::SVGKnob
{
	VerboDS()
	{
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/component/VerboDS.svg")));
		box.size = Vec(35, 35);
	}
};


struct SmallKnob : app::SVGKnob
{
	SmallKnob()
	{
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
	}
};


struct SmallBlu : SmallKnob
{
	SmallBlu()
	{
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/component/SmallBlu.svg")));
	}
};
struct MicroBlu : SmallBlu
{
	MicroBlu()
	{
		box.size = Vec(25, 25);
	}
};

struct LargeBlu : SmallBlu
{
	LargeBlu()
	{
		box.size = Vec(45, 45);
	}
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
	DaviesKnob()
	{
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
		box.size = Vec(15, 15);
	}
};

struct BorderKnob : DaviesKnob
{
	BorderKnob()
	{
		box.size = Vec(55, 55);
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/component/BorderKnob.svg")));
	}
};

struct RoundAzz : DaviesKnob
{
	RoundAzz()
	{
		box.size = Vec(30, 30);
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/component/RoundAzz.svg")));
	}
};
struct RoundRed : DaviesKnob
{
	RoundRed()
	{
		box.size = Vec(30, 30);
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/component/RoundRed.svg")));
	}
};

struct RoundWhy : DaviesKnob
{
	RoundWhy()
	{
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/component/RoundWhy.svg")));
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
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/component/LRoundWhy.svg")));
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

struct FlatA : DaviesKnob
{
	FlatA()
	{
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/component/FlatA.svg")));
		box.size = Vec(30, 30);
	}
};
struct FlatASnap : FlatA
{

	FlatASnap()
	{
		snap = true;
	}
};

struct FlatR : DaviesKnob
{
	FlatR()
	{
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/component/FlatR.svg")));
		box.size = Vec(30, 30);
	}
};
struct FlatS : DaviesKnob
{
	FlatS()
	{
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/component/FlatS.svg")));
		box.size = Vec(30, 30);
	}
};
struct FlatG : DaviesKnob
{
	FlatG()
	{
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/component/FlatG.svg")));
		box.size = Vec(30, 30);
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

struct WhiteLight : GrayModuleLightWidget
{
	WhiteLight()
	{
		addBaseColor(WHITE);
	}
};


template <typename BASE>
struct BigLight : BASE
{
	BigLight()
	{
		this->box.size = Vec(20, 20);
	}
};

template <typename BASE>
struct HugeLight : BASE
{
	HugeLight()
	{
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
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance,"res/component/LEDB_0.svg")));
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
