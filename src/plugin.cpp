#include "plugin.hpp"

Plugin *pluginInstance;

void init(rack::Plugin *p)
{
  pluginInstance = p;

  p->addModel(modelNavControl);
   p->addModel(modelBench);
  p->addModel(modelContorno);
  //p->addModel(modelContornoMK2);
  p->addModel(modelTranspose);
  p->addModel(modelUtility);
  p->addModel(modelChord);
  p->addModel(modelBene);
  p->addModel(modelBenePads);
  p->addModel(modelPerfMixer);
  p->addModel(modelDrMix);
  p->addModel(modelPerfMixer4);
  p->addModel(modelVCA4);
  p->addModel(modelVCA530);
  p->addModel(modelRemix);
  p->addModel(modelSmixer);
  p->addModel(modelVerbo);
  p->addModel(modelDVCO);
  p->addModel(modelDAOSC);
  p->addModel(modelTROSC);
  p->addModel(modelTROSCMK2);
  p->addModel(modelSuHa);
  p->addModel(modelSuHaMK2);
  p->addModel(modelFourSeq);
  p->addModel(modelDivider);
  p->addModel(modelUtil2);
  p->addModel(modelSmorph);
  p->addModel(modelBigSmorph);
  p->addModel(modelSPan);
  p->addModel(modelQuePasa);
  p->addModel(modelDualFilter);
  p->addModel(modelOrder);
  p->addModel(modelDualMatrix);
  
  readDarkAsDefault();
}
void destroy() {
	writeDarkAsDefault();
}

// ******** Panel Theme management ********

bool defaultPanelTheme;

void saveDarkAsDefault(bool darkAsDefault) {
	defaultPanelTheme = darkAsDefault;
}
bool loadDarkAsDefault() {
	return defaultPanelTheme;
}


bool isDark(int* panelTheme) {
	if (panelTheme != NULL) {
		return (*panelTheme != 0);
	}
	return defaultPanelTheme;
}

void writeDarkAsDefault() {
	json_t *settingsJ = json_object();
	json_object_set_new(settingsJ, "darkAsDefault", json_boolean(defaultPanelTheme));
	std::string settingsFilename = asset::user("dBiz.json");
	FILE *file = fopen(settingsFilename.c_str(), "w");
	if (file) {
		json_dumpf(settingsJ, file, JSON_INDENT(2) | JSON_REAL_PRECISION(9));
		fclose(file);
	}
	json_decref(settingsJ);
}

void readDarkAsDefault() {
	std::string settingsFilename = asset::user("dBiz.json");
	FILE *file = fopen(settingsFilename.c_str(), "r");
	if (!file) {
		defaultPanelTheme = false;
		writeDarkAsDefault();
		return;
	}
	json_error_t error;
	json_t *settingsJ = json_loadf(file, 0, &error);
	if (!settingsJ) {
		// invalid setting json file
		fclose(file);
		defaultPanelTheme = false;
		writeDarkAsDefault();
		return;
	}
	json_t *darkAsDefaultJ = json_object_get(settingsJ, "darkAsDefault");
	if (darkAsDefaultJ) {
		defaultPanelTheme = json_boolean_value(darkAsDefaultJ);
	}
	else {
		defaultPanelTheme = false;
	}
	
	fclose(file);
	json_decref(settingsJ);
}




