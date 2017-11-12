#include "dBiz.hpp"
<<<<<<< HEAD
#include <math.h>
=======
#include "math.h"
>>>>>>> 655532f335869e15e3ffb45719c858309e3f93eb
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

<<<<<<< HEAD
	p->addModel(createModel<MultipleWidget>("dBiz","Multiple", "Multiple",UTILITY_TAG));
	p->addModel(createModel<SubMixWidget>("dBiz","dBiz SubMix", "SubMix",MIXER_TAG));
	p->addModel(createModel<TransposeWidget>("dBiz","Transpose","Transpose",UTILITY_TAG));
	p->addModel(createModel<ChordWidget>("dBiz","dBiz Chord","Chord",UTILITY_TAG));
	p->addModel(createModel<PerfMixerWidget>("dBiz","PerfMixer", "Performance mixer",MIXER_TAG));
	p->addModel(createModel<BeneWidget>("dBiz","Bene", "Bene",SEQUENCER_TAG));
	p->addModel(createModel<BenePadsWidget>("dBiz","BenePads","BenePads",UTILITY_TAG));
	p->addModel(createModel<VCA530Widget>("dBiz","VCA530", "VCA530",AMPLIFIER_TAG));
	p->addModel(createModel<VerboWidget>("dBiz","VerboOsc", "VerboOsc",OSCILLATOR_TAG));
	p->addModel(createModel<DVCOWidget>("dBiz","Dual Oscillator", "Dual Oscillator",OSCILLATOR_TAG));

=======
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
>>>>>>> 655532f335869e15e3ffb45719c858309e3f93eb
}

