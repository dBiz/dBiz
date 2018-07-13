#include "dBiz.hpp"
#include <math.h>



Plugin *plugin; 

void init(rack::Plugin *p)
{
	plugin = p;
	p->slug = TOSTRING(SLUG);
	p->version = TOSTRING(VERSION);

    p->addModel(modeldBizBlank);
    p->addModel(modelMultiple);
    p->addModel(modelContorno);
    p->addModel(modelChord);
    p->addModel(modelUtility);
    p->addModel(modelTranspose);
    p->addModel(modelBene);
    p->addModel(modelBene2);
    p->addModel(modelBenePads);
    p->addModel(modelSubMix);
    p->addModel(modelRemix);
    p->addModel(modelPerfMixer);
    p->addModel(modelVCA4);
    p->addModel(modelVCA530);
    p->addModel(modelSmiX);
    p->addModel(modelVerbo);
    p->addModel(modelDVCO);
    p->addModel(modelDAOSC);
    p->addModel(modelTROSC);
    p->addModel(modelDualFilter);
    p->addModel(modelSuHa);
    p->addModel(modelFourSeq);
    p->addModel(modelDivider);
    p->addModel(modelUtil2);
}

