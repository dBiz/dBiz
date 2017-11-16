#include "rack.hpp"
#include "dBizComponent.hpp"
//#include "Biquad.h"
//#include "VAStateVariableFilter.h"

using namespace rack;

extern Plugin *plugin;


////////////////////
// module widgets
////////////////////

struct MultipleWidget : ModuleWidget {
	MultipleWidget();
};

struct SubMixWidget : ModuleWidget {
	SubMixWidget();
};

struct TransposeWidget : ModuleWidget {
	TransposeWidget();
};

struct ChordWidget : ModuleWidget {
	ChordWidget();
};

struct PerfMixerWidget : ModuleWidget {
	   PerfMixerWidget(); 
};

struct BeneWidget : ModuleWidget {
	   BeneWidget(); 
};

struct BenePadsWidget : ModuleWidget
{
	BenePadsWidget();
};

struct VCA530Widget : ModuleWidget
{
	VCA530Widget();
};

struct DVCOWidget : ModuleWidget
{
	DVCOWidget();
};

struct VerboWidget : ModuleWidget
{
	VerboWidget();
};

struct DAOSCWidget : ModuleWidget
{
	DAOSCWidget();
};

//struct NAMEWidget : ModuleWidget {
//	 NAMEWidget(); 
//};
