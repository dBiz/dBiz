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

/*struct DualFilterWidget : ModuleWidget
{
	DualFilterWidget();
};*/

struct VerboWidget : ModuleWidget
{
	VerboWidget();
};

//struct NAMEWidget : ModuleWidget {
//	 NAMEWidget(); 
//};
