#ifndef INCLUDE_CONFIGURATION_H_
#define INCLUDE_CONFIGURATION_H_

//#include <user_config.h>
#include <SmingCore/SmingCore.h>

// If you want, you can define WiFi settings globally in Eclipse Environment Variables


// Pin for communication with DHT sensor
//#define DHT_PIN 1 // UART0 TX pin
//#define DHT_PIN 12

// Pin for trigger control output
//#define CONTROL_PIN 3 // UART0 RX pin
//#define CONTROL_PIN 15

#define NameFile ".name.conf"
#define TempFile ".temp.conf"
#define SecondFile ".sec.conf" // leading point for security reasons :)
#define VFile ".v.conf"
#define FreezeSecFile ".fsec.conf"

enum fileType
{
	nameFile, //name
	tempFile, //min temp from heater
	secondsFile, // seconds heat 1 time
	vFile, //britshess,
	freezeSecondsFile
};

String loadConfig(fileType type);
void saveConfig(fileType type, String strVal);

int loadName();
void saveName(String name);
//extern void startWebClock();

//extern MeteoConfig ActiveConfig;

#endif /* INCLUDE_CONFIGURATION_H_ */

