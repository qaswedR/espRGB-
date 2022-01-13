#ifndef INCLUDE_CONFIGURATION_H_
#define INCLUDE_CONFIGURATION_H_

#include <user_config.h>
#include <SmingCore/SmingCore.h>

// If you want, you can define WiFi settings globally in Eclipse Environment Variables


// Pin for communication with DHT sensor
//#define DHT_PIN 1 // UART0 TX pin
//#define DHT_PIN 12

// Pin for trigger control output
//#define CONTROL_PIN 3 // UART0 RX pin
//#define CONTROL_PIN 15

#define METEO_CONFIG_FILE ".meteo.conf" // leading point for security reasons :)



struct ParamsConfig
{
	ParamsConfig()
	{
		name = 0;
		minimalTemp = 0;
		heatSec = 0;
		whiteFreezeSec = 0;
	}
	int name; // Temperature adjustment
	float minimalTemp;
	float heatSec;
	float whiteFreezeSec;

};

ParamsConfig loadConfig();
void saveConfig(ParamsConfig& cfg);
//extern void startWebClock();

extern ParamsConfig ActiveConfig;

#endif /* INCLUDE_CONFIGURATION_H_ */

