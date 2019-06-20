#include "plugin.hpp"

Plugin *pluginInstance;

void init(rack::Plugin *p)
{
  pluginInstance = p;
 

  p->addModel(modelMultiple);
  p->addModel(modelContorno);
  p->addModel(modelTranspose);
  p->addModel(modelUtility);
  p->addModel(modelChord);
  p->addModel(modelBene);
  p->addModel(modelBenePads);
  p->addModel(modelPerfMixer);
  p->addModel(modelVCA4);
  p->addModel(modelVCA530);
  p->addModel(modelRemix);
  p->addModel(modelSmixer);
  p->addModel(modelVerbo);
  p->addModel(modelDVCO);
  p->addModel(modelDAOSC);
  p->addModel(modelTROSC);
  p->addModel(modelSuHa);
  p->addModel(modelFourSeq);
  p->addModel(modelDivider);
  p->addModel(modelUtil2);
  p->addModel(modelSmorph);
}

