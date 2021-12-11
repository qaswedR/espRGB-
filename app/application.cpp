#include "../include/configuration.h"
#include "../include/version.h"
//#include "../include/lightHandler.hpp"
#include "lightHandler.cpp"
#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <Wiring/WString.h>
#include <Libraries/RCSwitch/RCSwitch.h>
//#include <Libraries/Livolo/livolo.h>
#include <Libraries/OneWire/OneWire.h>
#include <Libraries/DS18S20/ds18s20.h>
#include <Libraries/IR/IRremote.h>
#include <Libraries/IR/IRremoteInt.h>
//#include <string>
#include <Libraries/heatpumpir/heatpumpIR.h>

#define HTTP_BR "<br/>"
// download urls, set appropriately
#define SERVER_IP "192.168.0.4"
#define SERVER_URL "http://" SERVER_IP "/firmware/main/"
#define ROM_0_URL  SERVER_URL "rom0.bin"
#define ROM_1_URL  SERVER_URL "rom1.bin"
#define SPIFFS_URL SERVER_URL "spiff_rom.bin"

// If you want, you can define WiFi settings globally in Eclipse Environment Variables
#ifndef WIFI_SSID
#define WIFI_SSID "777DarkDima777" // Put you SSID and Password here
#define WIFI_PWD "12345679"
#endif

//#define LED_PIN 15 // GPIO number
#define r_pin  4
#define g_pin  5
#define b_pin  0
//#define s 100    //из hsv

#define HSV_TO_STATE(h,s,v) 

/*#define IR_ONE_SPACE    1200 /////TEST
 #define IR_ZERO_SPACE   400
 #define IR_BIT_MARK     450
 #define IR_PAUSE_SPACE  0
 #define IR_HEADER_MARK  3400
 #define IR_HEADER_SPACE 1700*/

LightHandler *lights = nullptr;
DS18S20 ReadTemp;
//Timer StartaliveTimer;
Timer aliveTimer;
Timer motionTimer;
Timer tempTimer;
Timer heatDisableTimer, dieTimer, rebootTimer, heatEnableTimer, coreTimer;
//Livolo livolo(12);
//DriverPWM ledPWM;
uint8_t pins[4] = { r_pin, g_pin, b_pin, w_pin }; // List of pins that you want to connect to pwm
float white;

HttpServer server;
HttpClient majordomo;
/////////////////////////////////flags
bool motionFlag = 0;   // 0/1 motion
bool dayFlag;       //светлое/тЄмное врем€ суток
bool motionWorkFlag;       // on/off motion detected
bool valueMSensor;
//uint8_t reboot = 0;
int motionTimeOn;
//uint8_t vNightMotion; //- €ркость ленты ночью

/////////////////////////////////////начальный значени€
int name;
float minimalTemp;
float oldT1;
float newT1, newT2;
float told = 0;
float motionV = 0;
float voff = 0.0;
bool alive = 0;
rBootHttpUpdate* otaUpdater = 0;
static bool needReconnect = false;
float heatSec;
float whiteFreezeSec;
bool HeatMode = false;
//int tempTimerSec = 9;
bool needReboot = false;
int soilMoisturePercent = 0;
uint8_t soilMoistureCheckCounter = 100;

///////////////FUNCTIONS
void ICACHE_FLASH_ATTR onDataSent(HttpClient& client, bool successful);
 void ICACHE_FLASH_ATTR sendLog(String log);
void ICACHE_FLASH_ATTR sendIp();
///////////////////////

void ICACHE_FLASH_ATTR sendResponce(HttpResponse &response)
{
		response.setContentType(ContentType::HTML);
		response.sendString(OK_RESPONCE);
}
void  OtaUpdate_CallBack(bool result) {

	//Serial.println("In callback...");
	if (result == true) {
// success
		uint8 slot;
		slot = rboot_get_current_rom();
		if (slot == 0)
			slot = 1;
		else
			slot = 0;
		rboot_set_current_rom(slot);
		System.restart();
	} else {
// fail

		//Serial.println("CALLBACK OTA FAIL...");
	}
}

void  whiteOff() {

		lights->setWhite(false);
//	HW_pwm.analogWrite(w_pin, 0);
	heatDisableTimer.stop();

	if (HeatMode) {
		heatEnableTimer.setIntervalMs(1000 * whiteFreezeSec);
		heatEnableTimer.startOnce();

	}
}

void  heatOnWitePin() {
	heatEnableTimer.stop();

	if (HeatMode) {
		
		lights->setWhite(true);
	//	HW_pwm.analogWrite(w_pin, duty - 1);
		heatDisableTimer.setIntervalMs(1000 * heatSec);
		heatDisableTimer.startOnce();
	}
}
void  reboot(){
	System.restart();	
}
void ICACHE_FLASH_ATTR rebootNow() {

//	saveConfig(nameFile, String(name));
							needReboot = true;
	//System.restart();	
	
			sendLog("rebootNow");
							
					//		delay(500);
	//Serial.println("REBOOT...");
	//System.restart();
}
void  OtaUpdate() {

	uint8 slot;
	rboot_config bootconf;

	//Serial.println("Updating...");

// need a clean object, otherwise if run before and failed will not run again
	if (otaUpdater)
		delete otaUpdater;
	otaUpdater = new rBootHttpUpdate();

// select rom slot to flash
	bootconf = rboot_get_config();
	slot = bootconf.current_rom;
	if (slot == 0)
		slot = 1;
	else
		slot = 0;

#ifndef RBOOT_TWO_ROMS
// flash rom to position indicated in the rBoot config rom table
	otaUpdater->addItem(bootconf.roms[slot], ROM_0_URL);
#else
// flash appropriate rom
	if (slot == 0) {
		otaUpdater->addItem(bootconf.roms[slot], ROM_0_URL);
	} else {
		otaUpdater->addItem(bootconf.roms[slot], ROM_1_URL);
	}
#endif

#ifndef DISABLE_SPIFFS
// use user supplied values (defaults for 4mb flash in makefile)
	if (slot == 0) {
		otaUpdater->addItem(RBOOT_SPIFFS_0, SPIFFS_URL);
	} else {
		otaUpdater->addItem(RBOOT_SPIFFS_1, SPIFFS_URL);
	}
#endif

// request switch and reboot on success
//otaUpdater->switchToRom(slot);
// and/or set a callback (called on failure or success without switching requested)
	otaUpdater->setCallback(OtaUpdate_CallBack);

// start update
	otaUpdater->start();
}

void  Switch() {
	uint8 before, after;
	before = rboot_get_current_rom();
	if (before == 0)
		after = 1;
	else
		after = 0;
	//Serial.printf("Swapping from rom %d to rom %d.\r\n", before, after);
	rboot_set_current_rom(after);
	//Serial.println("Restarting...\r\n");
	System.restart();
}

 void ICACHE_FLASH_ATTR downloadString(String query, String obj = "")
 {
	 String resQuery;
	 resQuery += "http://";
	 resQuery += SERVER_IP;
	 resQuery += "/objects/?object=";
	 resQuery += obj == "" ? String("espRGB") += name : obj;
	// resQuery += name;
	 resQuery += "&op=m&m=";
	 resQuery += query;
	 
			majordomo.downloadString(resQuery
							, onDataSent);
 }

 void ICACHE_FLASH_ATTR sendLog(String log)
 {
	downloadString("log&mess="+log);
 }
 
void ICACHE_FLASH_ATTR onDataSent(HttpClient& client, bool successful) {
}

void ICACHE_FLASH_ATTR updateHeater() {
	if (heatSec > 0 && whiteFreezeSec > 0) {    //&& heatSec < whiteFreezeSec) {
	//	updateHeater();
		heatDisableTimer.setIntervalMs(1000 * heatSec);
		heatEnableTimer.setIntervalMs(1000 * whiteFreezeSec);

		//если таймеры не запущены
		if (!heatDisableTimer.isStarted() && !heatEnableTimer.isStarted()) {
			//запускаем таймер
			heatEnableTimer.startOnce();
		}

		HeatMode = true;
		white = 4;

	} else {

		HeatMode = false;
		heatDisableTimer.stop();
		heatEnableTimer.stop();
		white = 0;
	}
}

void ICACHE_FLASH_ATTR handleConfig(HttpRequest &request,
		HttpResponse &response) {

	if (request.getQueryParameter("name").length() > 0) {
		name = request.getQueryParameter("name").toInt();

		saveConfig(nameFile, String(name));
		aliveTimer.stop();
		aliveTimer.setIntervalMs(1000 * 5);
		aliveTimer.start();
        alive = 0;
		//sendIp();

		//delay(10);
		//System.restart();

	}

	if (request.getQueryParameter("temp").length() > 0) {
		minimalTemp = request.getQueryParameter("temp").toFloat();
		saveConfig(tempFile, String(minimalTemp));
	}
	
	if (request.getQueryParameter("currV").length() > 0) {
		motionV = request.getQueryParameter("currV").toInt();
	}

	if (request.getQueryParameter("fs").length() > 0) {
		whiteFreezeSec = request.getQueryParameter("fs").toFloat();
		saveConfig(freezeSecondsFile, String(whiteFreezeSec));
		updateHeater();
	}

	if (request.getQueryParameter("hs").length() > 0) {
		heatSec = request.getQueryParameter("hs").toFloat();
		saveConfig(secondsFile, String(heatSec));
		updateHeater();

	}

	if (request.getQueryParameter("ok").length() > 0) {
		if (request.getQueryParameter("ok").toInt() == 1) {
			alive = 1;
		//	reboot = 0;
		aliveTimer.stop();
		aliveTimer.setIntervalMs(1000 * 60);
		aliveTimer.start();
			dieTimer.stop();
		}
	}

	if (request.getQueryParameter("mWork").length() > 0) {
		if (request.getQueryParameter("mWork").toInt() == 1) {

			motionWorkFlag = 1;
			motionTimer.start();
			if (request.getQueryParameter("valueMSensor").toInt() == 1) {
				digitalWrite(10, 0);
				valueMSensor = 1;
			} else if (request.getQueryParameter("valueMSensor").toInt() == 0) {
				digitalWrite(10, 1);
				valueMSensor = 0;
			}
		}
		if (request.getQueryParameter("mWork").toInt() == 0) {

			motionWorkFlag = 0;
			motionTimer.stop();
		}
	}
	if (request.getQueryParameter("smoothD").length() > 0
			&& request.getQueryParameter("smoothD").toInt() >= 0
			&& request.getQueryParameter("smoothD").toInt() < 50) {
		lights->setSmoothD(request.getQueryParameter("smoothD").toInt());
	}
	
	if (request.getQueryParameter("voff").length() > 0)
	{
		 voff = request.getQueryParameter("voff").toFloat();
		
	}
		
	

	response.setContentType(ContentType::HTML);
	String requestStr = "name = " + String(name) + HTTP_BR;
	requestStr += "id = " + String(system_get_chip_id()) + HTTP_BR;
	requestStr += ("mWFlag = " + String(motionWorkFlag) + HTTP_BR);
	//requestStr += ("smoothD = " + String(smoothD) + HTTP_BR);
	requestStr += ("voff = " + String(voff) + HTTP_BR);
	requestStr += ("alive = " + String(alive) + HTTP_BR);
	requestStr += ("valueMSensor = " + String(valueMSensor) + HTTP_BR);
	requestStr += ("f version rom1 = " + String(APP_VERSION) + HTTP_BR); //1.11 добавил смену сигнала датчика движени€(1 или 0)
	requestStr += ("h version  = 1.00  <br/>"); //ROM_0_URL
	requestStr += ("path = " + String(ROM_0_URL) + HTTP_BR); //ROM_0_URL
	requestStr += ("h = " + String(lights->getH()) + HTTP_BR); //ROM_0_URL
	requestStr += ("s = " + String(lights->getS()) + HTTP_BR); //ROM_0_URL
	requestStr += ("v = " + String(lights->getV()) + HTTP_BR); //ROM_0_URL
	requestStr += ("wFrzSec = " + String(whiteFreezeSec) + HTTP_BR); //ROM_0_URL
	requestStr += ("heatSec = " + String(heatSec) + HTTP_BR); //ROM_0_URL
	requestStr +=
			("heat on= " + String(heatDisableTimer.isStarted()) + HTTP_BR);
	requestStr += ("freeze on = " + String(heatEnableTimer.isStarted())
			+ HTTP_BR);
	requestStr += ("HeatMode = " + String(HeatMode) + HTTP_BR);
	requestStr += ("Temp1 = " + String(newT1) + HTTP_BR);
	requestStr += ("T2 = " + String(newT2) + HTTP_BR);
	//response.sendString("s = " + String(heatSeconds) + HTTP_BR);

	requestStr += ("minTemp = " + String(minimalTemp) + HTTP_BR);
	//requestStr += ("currVmotion = " + String(motionV) + HTTP_BR);
	requestStr += ("mf = " + String(motionFlag) + HTTP_BR);
	requestStr += ("mti = " + String(motionTimer.getIntervalMs()) + HTTP_BR);
	requestStr += ("lts = " + String(lights->ledTimerIsStarted()) + HTTP_BR);
	requestStr += ("moisPrst = " + String(soilMoisturePercent) + HTTP_BR);
	
	
	
	response.sendString(requestStr);
			sendLog("config request");

}

void handleIr(HttpRequest &request, HttpResponse &response) {

	sendResponce(response);
	if (request.getQueryParameter("code").length() > 0) {
		unsigned long code = request.getQueryParameter("code").toInt();
IRsend irsend(14);
		irsend.sendNEC(code, 32);
////Serial.print(i); //Serial.print("="); //Serial.println(code);

	//	delay(1);
	}
//	if (request.getPostParameter("raw").length() > 0) {
//		IRSenderBitBang irSender(14);
//		int IR_ONE_SPACE = 1200;
//		int IR_ZERO_SPACE = 450;
//		int IR_BIT_MARK = 450;
//		int IR_PAUSE_SPACE = 0;
//		int IR_HEADER_MARK = 1650;
//		int IR_HEADER_SPACE = 1700;
//		unsigned int l;
//		String str = request.getPostParameter("raw");
////Serial.println("          OK     ");
//		char symbols[1000];
//		l = str.length() + 5;
////Serial.println(str);
//		irSender.space(0);
//		irSender.setFrequency(38);
//		str.toCharArray(symbols, 300);
////while (char symbol = *symbols) {
//	//	HW_pwm.setPeriod(0);     ----------это нужное
//	//	ledTimer.stop();       ----------это нужное
//	//	tempTimer.stop();         ----------это нужное
//	//	motionTimer.stop();        ----------это нужное
//		delay(10);
//		irSender.space(0);
//		irSender.mark(IR_HEADER_MARK);
////irSender.space(IR_HEADER_SPACE);
////for (int qw = 0; qw < 5; qw++) {
//		for (int i = 0; i < l; i++) {
//			//symbols++;
//
//			//str.toCharArray(symbols, 2);
//			//char symbol = str.substring(0,1).toChar;
//			//str.remove(0, 1);
//			////Serial.print("i ");//Serial.println(i);
//
//			////Serial.print("symbols ");//Serial.println(symbols[i]);
//			////Serial.print("symbols ");//Serial.println(symbols[i]);
//			switch (symbols[i]) {
//
//			case '1':
//				irSender.mark(IR_BIT_MARK);
//				irSender.space(IR_ONE_SPACE);
//				////Serial.println("send 1 ");
//
//				break;
//			case '0':
//				irSender.mark(IR_BIT_MARK);
//				irSender.space(IR_ZERO_SPACE);
//				////Serial.println("send 0 ");
//				break;
//			case 'W':
//				irSender.mark(IR_PAUSE_SPACE);
//				break;
//			case 'H':
//				irSender.mark(IR_HEADER_MARK);
//				////Serial.println("send H ");
//				break;
//			case 'h':
//				irSender.space(IR_HEADER_SPACE);
//				////Serial.println("send h ");
//				break;
//			}
//		}
//		irSender.space(0);
//
////Serial.println("          OK     ");
//
//		irSender.space(0);
//
//	}
	//HW_pwm.setPeriod(period);  ----------это нужное
	//ledTimer.start();         ----------это нужное
	//tempTimer.start();        ----------это нужное
	//motionTimer.start();        ----------это нужное
		//	sendLog("handleIr");
}

void sendTemp() {
	if(++soilMoistureCheckCounter >= 6)
	{
		soilMoistureCheckCounter = 0;
		static const int airVal = 865;
		static const int waterVal = 645;
		static const int sensorPin = A0;
		int measuredVal = analogRead(sensorPin);
		int newSoilMoisturePercent = map(measuredVal, airVal, waterVal, 0, 100);
		//if (soilMoisturePercent != newSoilMoisturePercent && alive && newSoilMoisturePercent)
		{
			soilMoisturePercent = newSoilMoisturePercent;
			if(soilMoisturePercent < 0)
			{
				soilMoisturePercent = 0;
			}else if(soilMoisturePercent > 100)
			{
				soilMoisturePercent = 100;			
			}
			String resQuery;
			resQuery += "moistureChanged&mea=";
			resQuery += soilMoisturePercent;
			String strName;
			strName += "temp";
			strName += name;
			downloadString(resQuery, strName);
		}
	}
	
	uint8_t a;
	//uint64_t info;
//	MeteoConfig cfg = loadConfig();

	bool newTemp1 = false;
	bool newTemp2 = false;
	if (!ReadTemp.MeasureStatus())  // the last measurement completed
	{
		static int sz = ReadTemp.GetSensorsCount();
		//	if (sz) // is minimum 1 sensor detected ?
		for (a = 0; a < sz; a++) // prints for all sensors
		{
			//Serial.print(" T=");
			if (ReadTemp.IsValidTemperature(a)) // temperature read correctly ?
			{
				static float currentTemp;
				currentTemp = ReadTemp.GetCelsius(a);

				if (currentTemp > 50.0 || currentTemp < -10.0
						|| (currentTemp > -0.07 && currentTemp < -0.05) )
					continue;

				if (a > 0) {
				//	if (newT2 != currentTemp) {
						newTemp2 = true;
						newT2 = currentTemp;
					//}
				} else {
					//	newT1 = currentTemp;
				//	if (newT1 != currentTemp) {
						newTemp1 = true;
						newT1 = currentTemp;
				//	}
				}
				////Serial.print(ReadTemp.GetCelsius(a));

			} else {

			}
		}

		if (newTemp1 || newTemp2) {
		//	sendLog("sendTemp");
			static bool doubleFreeze = false, halfFreeze = false;
			static int defoltFreezeSec = 0;

			if (alive)
			{
	             String resQuery;
	             resQuery += "tempChanged&t=";
	             resQuery += newTemp1 ? String(newT1) : "";
	             resQuery += "&t2=";
	             resQuery += newTemp2 ? String(newT2) : "";
	             String strName;
				 strName += "temp";
				 strName += name;
				downloadString(resQuery, strName);
			}

			if (minimalTemp > 0 && HeatMode && alive == 0)

			{
				if (!newTemp1 && newTemp2) {
					newT1 = newT2;
				} else if (!newTemp2 && newTemp1) {
					newT2 = newT1;
				}

				//if temp high - enable double time freeze
				if (newT1 > minimalTemp && newT2 > minimalTemp) {

					//если подогреваетс€
					if (halfFreeze) {
						//выключаем обогрев
						halfFreeze = false;
						whiteFreezeSec = defoltFreezeSec;
					}

					//если не охлаждаетс€
					if (!doubleFreeze) {
						//охлаждаем
						doubleFreeze = true;
						defoltFreezeSec = whiteFreezeSec;
						//	whiteFreezeSec += whiteFreezeSec / 3;
					}
					//включаем подогрев
					//if (whiteFreezeSec + 0.1 > heatSec) {
					whiteFreezeSec += 0.1;
					//	}

					//если температура ниже выставленной
				} else {
					//если включено охлаждение
					if (doubleFreeze) {
						//выключаем охлаждение
						doubleFreeze = false;
						whiteFreezeSec = defoltFreezeSec;
					} else {
						//если не включен подогрев
						if (!halfFreeze) {
							//включаем подогрев
							halfFreeze = true;
							defoltFreezeSec = whiteFreezeSec;
						}
						//включаем подогрев
						if (whiteFreezeSec - 0.1 > heatSec) {
							whiteFreezeSec -= 0.1;
						}

					}

				}
			}
		} 
	} else {

	}
	ReadTemp.StartMeasure();

}

void ICACHE_FLASH_ATTR handleRC(HttpRequest &request, HttpResponse &response) {
		//	sendLog("handleRC");
	sendResponce(response);

	if (request.getQueryParameter("code").toInt() > 0
			&& request.getQueryParameter("bit").toInt() < 70
			&& request.getQueryParameter("bit").toInt() > 0) 
	{
				
		RCSwitch mySwitch;
		mySwitch.enableTransmit(12);
		mySwitch.send(request.getQueryParameter("code").toInt(),
				request.getQueryParameter("bit").toInt());
		//delay(1);
	}
}

void handleOTA(HttpRequest &request, HttpResponse &response) {

	sendLog("handleOTA");
	if (request.getQueryParameter("start") == "now") {

		delay(5);
		OtaUpdate();
		response.setContentType(ContentType::HTML);
		response.sendString("OTA start.\n");
	}
	if (request.getQueryParameter("restart") == "now") {
		response.setContentType(ContentType::HTML);
		response.sendString("Restart.\n <br/>");
		dieTimer.stop();
		dieTimer.setIntervalMs(1000);
		dieTimer.start();
		rebootTimer.start();
		lights->setVNew(0.0);
	}

}

void aliveSend() {
	Serial.println(APP_VERSION);
	
	aliveTimer.stop();
	aliveTimer.setIntervalMs(1000 * 5);
	aliveTimer.start();
	sendIp();
}

void startWebServer() {
	server.listen(80);
	//server.addPath("/", onIndex);
	//server.addPath("/rgb", handleRGB);
	server.addPath("/hsv", lights->handleHsv);
	server.addPath("/ota", handleOTA);
	server.addPath("/rc", handleRC);
	//server.addPath("/livolo", handleLivolo);
	server.addPath("/ir", handleIr);
	//server.addPath("/modHSV", handleModHsv);
	server.addPath("/config", handleConfig);

//server.setDefaultHandler(onFile);

//Serial.println(WifiStation.getIP());

	sendIp();

	//}
//	StartaliveTimer.start();
//	startAliveSend();
	aliveTimer.start();

	//delay(10);
	//sendTemp();
	aliveSend();
	//delay(10);
	sendTemp();

}

void connectOk() {
	needReconnect = true;
	////Serial.println("I'm CONNECTED");

	/*if (!fileExist("index.html") || !fileExist("bootstrap.css.gz") || !fileExist("jquery.js.gz"))
	 {
	 // Download server content at first
	 downloadTimer.initializeMs(3000, downloadContentFiles).start();
	 }
	 else
	 {*/
	startWebServer();
//}
}

void sendIp()
{ 
		name = loadConfig(nameFile).toInt();
		if (name == 0) {
			String query;
			query+= "WhoAmi&id=";
			query+= system_get_chip_id();
			query+= "&ip=";
			query+= WifiStation.getIP().toString();
			
			downloadString(query, "espRGBbuf");

		} else {
			String query;
			query+= "changeIP&id=";
			query+= system_get_chip_id();
			query+= "&ip=";
			query+= WifiStation.getIP().toString();
			query+= "&a=";
			query+= alive;
			query+= "&v=";
			query+= APP_VERSION;
			
			downloadString(query);
		}	
}

void coreHandler()
{
	if (!WifiStation.isConnected() && needReconnect) 
	{
		
		WifiStation.waitConnection(connectOk, 125, rebootNow);
		needReconnect = false;
	}
}

void vCallback(float _newV)
{
	if(int(_newV*100) == int(voff*100))
	{
		motionFlag = 0;
	}
}

void motion() {
	if (digitalRead(10) == valueMSensor) {
		if (!motionFlag)
		{
			//	if(v_new < 0.9 && v < 0.9)
			{
				lights->setVNew(motionV);
			}
			motionFlag = 1;				
		}
		motionTimer.setIntervalMs(1000*10);
		
		downloadString("motionDetected&");
	}else{
		motionTimer.setIntervalMs(50);
	} 

}

void init() {
 lights = new LightHandler(pins, &white, &vCallback);
	//lights = LightHandler::getInstance(pins, &white, &voff, &motionFlag);
//	lights = &LightHandler(pins, &white, &voff, &motionFlag);
	//spiffs_mount();
	pinMode(10, INPUT);
	pinMode(w_pin, 1);
	digitalWrite(w_pin, 0);
	pinMode(14, 1);
	pinMode(13, 1);
	digitalWrite(14, 0);
	Serial.begin(SERIAL_BAUD_RATE);   		// 115200 by default
	Serial.systemDebugOutput(0);   		// Debug output to serial
//	ledPWM.initialize();
			//pinMode(LED_PIN, OUTPUT);
	//Serial.print("Duty: ");
	//Serial.println(HW_pwm.getMaxDuty());
	WifiStation.enable(true);
	WifiStation.config(WIFI_SSID, WIFI_PWD);
	WifiAccessPoint.enable(false);
	ReadTemp.Init(16);  // select PIN It's required for one-wire initialization!
	ReadTemp.StartMeasure(); // first measure start,result after 1.2 seconds * number of sensors
//	int slot = rboot_get_current_rom();
	if (rboot_get_current_rom() == 0) {
//debugf("trying to mount spiffs at %x, length %d", RBOOT_SPIFFS_0 + 0x40200000, SPIFF_SIZE);
		spiffs_mount_manual(RBOOT_SPIFFS_0 + 0x40200000, SPIFF_SIZE);
	} else {
//debugf("trying to mount spiffs at %x, length %d", RBOOT_SPIFFS_1 + 0x40200000, SPIFF_SIZE);
		spiffs_mount_manual(RBOOT_SPIFFS_1 + 0x40200000, SPIFF_SIZE);
	}

	WifiStation.connect();
	WifiStation.waitConnection(connectOk, 50, rebootNow);

	System.setCpuFrequency(eCF_160MHz);
	//Serial.setCallback(serialCallBack);
	delay(10);
	tempTimer.initializeMs(1000 * 20, sendTemp).start();
	motionTimer.initializeMs(50, motion);
	aliveTimer.initializeMs(1000 * 5, aliveSend);
	heatDisableTimer.initializeMs(1000 * 5, whiteOff);
	dieTimer.initializeMs(1000 * 60 * 9, reboot).start();
	rebootTimer.initializeMs(1000 * 3, rebootNow);
	heatEnableTimer.initializeMs(0, heatOnWitePin);
	coreTimer.initializeMs(1000*10, coreHandler).start();

	minimalTemp = loadConfig(tempFile).toFloat();
	heatSec = loadConfig(secondsFile).toFloat();
	whiteFreezeSec = loadConfig(freezeSecondsFile).toFloat();

	updateHeater();
}

