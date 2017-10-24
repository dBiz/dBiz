#include "rack.hpp"
#include "dBizComponent.hpp"

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

//struct NAMEWidget : ModuleWidget {
//	 NAMEWidget(); 
//};
