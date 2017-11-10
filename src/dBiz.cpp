#include "dBiz.hpp"
#include "math.h"
#include "dsp/filter.hpp"
#include "dsp/fir.hpp"
#include "dsp/frame.hpp"
#include "dsp/ode.hpp"
#include "dsp/samplerate.hpp"

Plugin *plugin;

void init(rack::Plugin *p)
{
	plugin = p;
	
	p->slug = "dBiz";

#ifdef VERSION
	p->version = TOSTRING(VERSION);
#endif

	p->addModel(createModel<MultipleWidget>("dBiz", "dBiz", "dBiz Multiple", "Multiple"));
	p->addModel(createModel<SubMixWidget>("dBiz", "dBiz", "dBiz SubMix", "SubMix"));
	p->addModel(createModel<TransposeWidget>("dBiz", "dBiz", "dBiz Transpose", "Transpose"));
	p->addModel(createModel<ChordWidget>("dBiz", "dBiz", "dBiz Chord", "Chord"));
	p->addModel(createModel<PerfMixerWidget>("dBiz", "dBiz", "PerfMixer", "Performance mixer"));
	p->addModel(createModel<BeneWidget>("dBiz", "dBiz", "Cartesian", "Bene"));
	p->addModel(createModel<BenePadsWidget>("dBiz", "dBiz", "CartesianPads", "BenePads"));
	p->addModel(createModel<VCA530Widget>("dBiz", "dBiz", "VCA530", "VCA530"));
//	p->addModel(createModel<DualFilterWidget>("dBiz", "dBiz", "DualFilter", "DualFilter"));
	p->addModel(createModel<VerboWidget>("dBiz", "dBiz", "VerboOsc", "VerboOsc"));
}

