#include "../include/configuration.h"

#include <SmingCore/SmingCore.h>

ParamsConfig ActiveConfig;

ParamsConfig loadConfig() {
	DynamicJsonBuffer jsonBuffer;
	ParamsConfig cfg;

	int size = fileGetSize(METEO_CONFIG_FILE);
	char* jsonString = new char[size + 1];
	fileGetContent(METEO_CONFIG_FILE, jsonString, size + 1);
	JsonObject& root = jsonBuffer.parseObject(jsonString);

	JsonObject& network = root["rt"];
	cfg.name = String((const char*) network["name"]).toInt();
	cfg.minimalTemp = String((const char*) network["mt"]).toFloat();
	cfg.heatSec = String((const char*) network["hs"]).toFloat();
	cfg.whiteFreezeSec = String((const char*) network["fs"]).toFloat();

	delete[] jsonString;

	return cfg;
}

void saveConfig(ParamsConfig& cfg) {
	ActiveConfig = cfg;

	DynamicJsonBuffer jsonBuffer;
	JsonObject& root = jsonBuffer.createObject();
	JsonObject& network = jsonBuffer.createObject();
	root["rt"] = network;
	network["name"] = String(cfg.name).c_str();
	network["mt"] = String(cfg.minimalTemp).c_str();
	network["hs"] = String(cfg.heatSec).c_str();
	network["fs"] = String(cfg.whiteFreezeSec).c_str();

	/*JsonObject& correction = jsonBuffer.createObject();
	 root["correction"] = correction;
	 correction["name"] = cfg.name;
	 correction["ssid"] = cfg.ssid;
	 correction["pswd"] = cfg.pswd;*/

	char buf[300];
	root.prettyPrintTo(buf, sizeof(buf));
	fileSetContent(METEO_CONFIG_FILE, buf);
}
