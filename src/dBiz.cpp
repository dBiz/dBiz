#include "dBiz.hpp"
#include <math.h>


Plugin *plugin; 

void init(rack::Plugin *p)
{
	plugin = p;
	p->slug = "dBiz";
#ifdef VERSION
	p->version = TOSTRING(VERSION);
#endif

	p->addModel(createModel<dBizBlankWidget>("dBiz", "Blank", "Blank", BLANK_TAG));
	p->addModel(createModel<MultipleWidget>("dBiz","Multiple", "Multiple",UTILITY_TAG));
	p->addModel(createModel<ContornoWidget>("dBiz","Contorno", "Contorno",ENVELOPE_GENERATOR_TAG));
	p->addModel(createModel<ChordWidget>("dBiz","dBiz Chord","Chord",UTILITY_TAG));
	p->addModel(createModel<UtilityWidget>("dBiz","Utility", "Utility",QUANTIZER_TAG));
	p->addModel(createModel<TransposeWidget>("dBiz","Transpose","Transpose",UTILITY_TAG));
	p->addModel(createModel<BeneWidget>("dBiz","Bene", "Bene",SEQUENCER_TAG));
	p->addModel(createModel<Bene2Widget>("dBiz", "Bene2", "Bene2", SEQUENCER_TAG));
	p->addModel(createModel<BenePadsWidget>("dBiz","BenePads","BenePads",UTILITY_TAG));
	p->addModel(createModel<SubMixWidget>("dBiz","dBiz SubMix", "SubMix",MIXER_TAG));
	p->addModel(createModel<RemixWidget>("dBiz", "dBiz Remix", "Remix", MIXER_TAG));
	p->addModel(createModel<PerfMixerWidget>("dBiz","PerfMixer", "Performance mixer",MIXER_TAG));
	p->addModel(createModel<VCA530Widget>("dBiz","VCA530", "VCA530",AMPLIFIER_TAG));
	p->addModel(createModel<VerboWidget>("dBiz","VerboOsc", "VerboOsc",OSCILLATOR_TAG));
	p->addModel(createModel<DVCOWidget>("dBiz","Dual Oscillator", "Dual Oscillator",OSCILLATOR_TAG));
	p->addModel(createModel<DAOSCWidget>("dBiz", "Dual SineBank Oscillator", "Dual Sine Bank Oscillator", OSCILLATOR_TAG));

}

