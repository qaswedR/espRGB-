#include "../include/configuration.h"
#include "../include/version.h"
#include "../include/otaUpdater.h"
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

// If you want, you can define WiFi settings globally in Eclipse Environment Variables
#ifndef WIFI_SSID
#define WIFI_SSID "777DarkDima777" // Put you SSID and Password here
#define WIFI_PWD "12345679"
#endif

#define TEMP_PIN   16
#define MOTION_PIN 10
#define RC_PIN     12
#define IR_PIN     14
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
Timer heatDisableTimer, dieTimer, heatEnableTimer, coreTimer, rebootTimer;
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
bool alive = 0;
static bool needReconnect = false;
float heatSec;
float whiteFreezeSec;
bool HeatMode = false;
//int tempTimerSec = 9;
bool needReboot = false;
int soilMoisturePercent = 0;
uint8_t soilMoistureCheckCounter = 100;
int irpinState = 0;

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

void startDieTimer()
{
		dieTimer.stop();
		dieTimer.setIntervalMs(210);
		dieTimer.start();	
}
void setLights(float _h)
{
		lights->setVNew(0.0);
		lights->setH(_h);
		lights->setS(100.0);
		lights->setMode(0);	
}
void rebootByDie() {

	setLights(0.0);
	//sendLog("rDie");
	if(!rebootTimer.isStarted())
	{
		rebootTimer.start();
	}
}
void rebootByWifi() {

	setLights(240.0);
	 //   sendLog("rWifi");
		startDieTimer();
//	rebootTimer.start();
}
void rebootByHttp() {

	setLights(120.0);
	//    sendLog("rHttp");
		startDieTimer();
//	rebootTimer.start();
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
		HttpResponse &response) 
{

	// controller unique name
	{
		String _name = request.getQueryParameter("name");
		if (_name.length() > 0) 
		{
			name = _name.toInt();
	
			saveName(String(name));
			aliveTimer.stop();
			aliveTimer.setIntervalMs(1000 * 5);
			aliveTimer.start();
			alive = 0;
			//sendIp();
	
			//delay(10);
			//System.restart();
	
		}
	}
	
	// minimal temp for heater
	{
		String _temp = request.getQueryParameter("temp");
		if (_temp.length() > 0) {
			minimalTemp = _temp.toFloat();
			saveConfig(tempFile, String(minimalTemp));
		}
	}
	
	//ir pin state
	{
		String _irpin = request.getQueryParameter("irpin");
		if (_irpin.length() > 0) {
			irpinState = _irpin.toInt();
			digitalWrite(IR_PIN, irpinState);
		}
	}
	
	//V for motion detect
	{
		String _currV = request.getQueryParameter("currV");
		if (_currV.length() > 0) {
			motionV = _currV.toInt();
		}
	}
	
	// freeze seconds for heater in ms
	{
		String _fs = request.getQueryParameter("fs");
		if (_fs.length() > 0) {
			whiteFreezeSec = _fs.toFloat();
			saveConfig(freezeSecondsFile, String(whiteFreezeSec));
			updateHeater();
		}
	}
	
	// heat seconds for heater in ms
	{
		String _hs = request.getQueryParameter("hs");
		if (_hs.length() > 0) {
			heatSec = _hs.toFloat();
			saveConfig(secondsFile, String(heatSec));
			updateHeater();
	
		}
	}
	
	//config is ok          ?????? why
	{
		String ok = request.getQueryParameter("ok");
		if (ok.length() > 0) {
			if (ok.toInt() == 1) {
				alive = 1;
				//	reboot = 0;
				aliveTimer.stop();
				aliveTimer.setIntervalMs(1000 * 60);
				aliveTimer.start();
				dieTimer.stop();
			}
		}
	}
	
	//motion sensor work
	{
		String mWork = request.getQueryParameter("mWork");
		if (mWork.length() > 0) {
			if (mWork.toInt() == 1) {
	
				motionWorkFlag = 1;
				if(!motionTimer.isStarted())
				{
					motionTimer.start();
				}
				if (request.getQueryParameter("valueMSensor").toInt() == 1) {
					digitalWrite(10, 0);
					valueMSensor = 1;
				} else if (request.getQueryParameter("valueMSensor").toInt() == 0) {
					digitalWrite(10, 1);
					valueMSensor = 0;
				}
			}
			if (mWork.toInt() == 0) {
	
				motionWorkFlag = 0;
				motionTimer.stop();
			}
		}
	}
	
	//smooth light change
	{
		String smoothD = request.getQueryParameter("smoothD");
		if(smoothD.length())
		{
			int smoothDInt = smoothD.toInt();
			if (smoothDInt == 0 || smoothDInt == 1) 
			{
				lights->setSmoothD(smoothDInt);
			}
		}
	}
	
		
	

	response.setContentType(ContentType::HTML);
	String requestStr = "name = " + String(name) + HTTP_BR;
	requestStr += "id = " + String(system_get_chip_id()) + HTTP_BR;
	requestStr += ("mWFlag = " + String(motionWorkFlag) + HTTP_BR);
	//requestStr += ("smoothD = " + String(smoothD) + HTTP_BR);
	requestStr += ("alive = " + String(alive) + HTTP_BR);
	requestStr += ("valueMSensor = " + String(valueMSensor) + HTTP_BR);
	requestStr += ("f version rom1 = " + String(APP_VERSION) + HTTP_BR); //1.11 добавил смену сигнала датчика движени€(1 или 0)
	//requestStr += ("h version  = 1.00  <br/>"); //ROM_0_URL
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
	//requestStr += ("lts = " + String(lights->ledTimerIsStarted()) + HTTP_BR);
	requestStr += ("moisPrst = " + String(soilMoisturePercent) + HTTP_BR);
	requestStr += ("irpinState = " + String(irpinState) + HTTP_BR);
	
	
	
	response.sendString(requestStr);
	//sendLog("config_request");

}

void handleIr(HttpRequest &request, HttpResponse &response) {

	sendResponce(response);
	if (request.getQueryParameter("code").length() > 0) {
		unsigned long code = request.getQueryParameter("code").toInt();
IRsend irsend(IR_PIN);
		irsend.sendNEC(code, 32);
////Serial.print(i); //Serial.print("="); //Serial.println(code);

	//	delay(1);
	}
//	if (request.getPostParameter("raw").length() > 0) {
//		IRSenderBitBang irSender(IR_PIN);
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
		
		if (soilMoisturePercent != newSoilMoisturePercent && alive && newSoilMoisturePercent)
		{
			soilMoisturePercent = newSoilMoisturePercent;
			String resQuery;
			resQuery += "moistureChanged&mea=";
			resQuery += soilMoisturePercent;
			String strName;
			strName += "temp";
			strName += name;
			downloadString(resQuery, strName);
		}
	}
	
	//uint8_t a;
	//uint64_t info;
//	MeteoConfig cfg = loadConfig();

	bool newTemp1 = false;
	bool newTemp2 = false;
	if (!ReadTemp.MeasureStatus())  // the last measurement completed
	{
		static int sz = ReadTemp.GetSensorsCount();
		//	if (sz) // is minimum 1 sensor detected ?
		for (uint8_t a = 0; a < sz; a++) // prints for all sensors
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
				//char buffer[10];
				//int ret = sprintf(buffer, "%f_%d", currentTemp, a);
				//if(ret > 0)
				//	sendLog(buffer);
				////Serial.print(ReadTemp.GetCelsius(a));

			} else {
				//float currentTemp = ReadTemp.GetCelsius(a);
				//char buffer[30];
				//int ret = sprintf(buffer, "not_val_%f", currentTemp);
				//if(ret > 0)
				//	sendLog(buffer);

			}
		}

		if (newTemp1 || newTemp2) {
		//	sendLog("sendTemp");
			static bool doubleFreeze = false, halfFreeze = false;
			static int defoltFreezeSec = 0;

			if (alive)
			{
				//char resQueryBuffer[100];
				//int ret = sprintf(resQueryBuffer, "tempChanged&t=%f&t2=%f", newT1, newT2);
	            String resQuery;
	            resQuery += "tempChanged&t=";
	            resQuery += newTemp1 ? String(newT1) : "";
	            resQuery += "&t2=";
	            resQuery += newTemp2 ? String(newT2) : "";
				//if(ret > 0)
				//{
				//	char strNameBuffer[10];
				//	ret = sprintf(strNameBuffer, "temp%d", name);
				//	if(ret > 0)
				//	{
				//		downloadString(resQueryBuffer, strNameBuffer);						
				//	}
				//}
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

	int code = request.getQueryParameter("code").toInt();
	int bit = request.getQueryParameter("bit").toInt();
	if (code > 0
			&& bit < 70
			&& bit > 0) 
	{
				
		RCSwitch mySwitch;
		mySwitch.enableTransmit(RC_PIN);
		mySwitch.send(code,	bit);
		//delay(1);
	}
}

void handleOTA(HttpRequest &request, HttpResponse &response) {

	sendResponce(response);
	if (request.getQueryParameter("start") == "now") {

		delay(1);
		OtaUpdate();
	}
	else if (request.getQueryParameter("restart") == "now") {
		rebootByHttp();
	}
	sendLog("handleOTA");

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
	//sendTemp();

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
		
		WifiStation.waitConnection(connectOk, 125, rebootByWifi);
	//	WifiStation.connect();
		needReconnect = false;
	}
}

void vCallback()
{
	if(motionWorkFlag)
	{
		motionFlag = 0;
		motionTimer.setIntervalMs(50);
		motionTimer.start();
	}
}

void motion() {
	if (digitalRead(MOTION_PIN) == valueMSensor) 
	{
		if (!motionFlag)
		{
			//	if(v_new < 0.9 && v < 0.9)
			{
				lights->setVNew(motionV);
			}
			motionFlag = 1;				
		}
		motionTimer.setIntervalMs(1000*10);
		motionTimer.start();
		
		downloadString("motionDetected&");
	}else
	{
		if(motionFlag)
		{
			motionFlag = 0;
			motionTimer.setIntervalMs(50);
			motionTimer.start();
		}
	}
}

void init() {
	//lights = LightHandler::getInstance(pins, &white, &voff, &motionFlag);
//	lights = &LightHandler(pins, &white, &voff, &motionFlag);
	//spiffs_mount();
	pinMode(MOTION_PIN, INPUT);
	pinMode(w_pin, 1);
	digitalWrite(w_pin, 0);
	pinMode(IR_PIN, 1);
//	pinMode(13, 1);
	digitalWrite(IR_PIN, 0);
	Serial.begin(SERIAL_BAUD_RATE);   		// 115200 by default
	Serial.systemDebugOutput(0);   		// Debug output to serial
//	ledPWM.initialize();
			//pinMode(LED_PIN, OUTPUT);
	//Serial.print("Duty: ");
	//Serial.println(HW_pwm.getMaxDuty());
	WifiStation.enable(true);
	WifiStation.config(WIFI_SSID, WIFI_PWD);
	WifiAccessPoint.enable(false);
	ReadTemp.Init(TEMP_PIN);  // select PIN It's required for one-wire initialization!
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
	WifiStation.waitConnection(connectOk, 50, rebootByWifi);

	System.setCpuFrequency(eCF_160MHz);
	//Serial.setCallback(serialCallBack);
	delay(10);
	tempTimer.initializeMs(1000 * 20, sendTemp).start();
	motionTimer.initializeMs(50, motion);
	aliveTimer.initializeMs(1000 * 5, aliveSend);
	heatDisableTimer.initializeMs(1000 * 5, whiteOff);
	dieTimer.initializeMs(1000 * 60 * 9, rebootByDie).start();
	rebootTimer.initializeMs(350, reboot);
	heatEnableTimer.initializeMs(0, heatOnWitePin);
	coreTimer.initializeMs(1000*10, coreHandler).start();
	
	name = loadName();
	lights = new LightHandler(pins, &white, &vCallback);

	minimalTemp = loadConfig(tempFile).toFloat();
	heatSec = loadConfig(secondsFile).toFloat();
	whiteFreezeSec = loadConfig(freezeSecondsFile).toFloat();
	float cfgV = loadConfig(vFile).toFloat();
	lights->setVNew(cfgV);

	updateHeater();
}

