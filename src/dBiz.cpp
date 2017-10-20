#include "dBiz.hpp"
#include <math.h>


Plugin *plugin;

void init(rack::Plugin *p) {
	plugin = p;
	plugin->slug = "dBiz";
	plugin->name = "dBiz";
	
	
	createModel<MultipleWidget>(plugin, "dBiz Multiple",    "Multiple");
	createModel<SubMixWidget>(plugin, "dBiz SubMix",    "SubMix");
	createModel<TransposeWidget>(plugin, "dBiz Transpose",    "Transpose");
	createModel<ChordWidget>(plugin, "dBiz Chord",    "Chord");
	createModel<PerfMixerWidget>(plugin, "PerfMixer", "Performance mixer");
	createModel<MentalCartesianWidget>(plugin, "Cartesian", "Bene");

	//createModel<NAMEWidget>(plugin, "NAME", "NAME");


}
