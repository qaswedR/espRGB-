#ifndef LIGHT_HANDLER_H_
#define LIGHT_HANDLER_H_

#include <SmingCore/HardwarePWM.h>
#include <SmingCore/SmingCore.h>


#define OK_RESPONCE "<!DOCTYPE html><head>OK</head></body></html>"
#define w_pin  15


#pragma once
enum pinsEnum{
  RedPin,
  GreenPin,
  BluePin,
  WhitePin
};

typedef void (*callBackSetV)();

class LightHandler {
public:

  LightHandler(uint8_t *pins, float *white, callBackSetV _callback);
public:
static void  handleHsv(HttpRequest &request, HttpResponse &response);
void handleHsvImpl(HttpRequest &request, HttpResponse &response);
  void  setVNew(float _v)
  {
	  v_new = _v;
	  startTimer();
  }
  
  void setHNew(float _h)
  {
	  h_new = _h;
	  startTimer();
  }
  void setH(float _h)
  {
	  h = _h;
	  startTimer();
  }
  
  void setSNew(float _s)
  {
	  s_new = _s;
	  startTimer();
  }
  
  void setS(float _s)
  {
	  s = _s;
	  startTimer();
  }
  
  void ICACHE_FLASH_ATTR setMode(uint8_t _mode)
  {
	  mode = _mode;
	  startTimer();
  }
  
  void ICACHE_FLASH_ATTR setDuty(unsigned int _duty)
  {
	  duty = _duty;
  }
  void ICACHE_FLASH_ATTR setSmoothD(int _smoothD)
  {
	  smoothD = _smoothD;
  }
  
  float ICACHE_FLASH_ATTR getH()
  {
	  return h;
  }
  float ICACHE_FLASH_ATTR getS()
  {
	  return s;
  }
  float  getV()
  {
	  return v;
  }
  bool  ledTimerIsStarted()
  {
	  return ledTimer.isStarted();
  }
  void  setWhite(bool isHight)
  {
	  HW_pwm->analogWrite(w_pin, isHight ?  duty - 1 : 0);
  }
  callBackSetV callback;
void loop();
Timer ledTimer, *whiteOffTimer = nullptr;
private:
bool countUp = true;
int counterSmooth = 0;  //счЄтчик дл¤ смены ¤ркости
int smoothD = 0;   //передаваемый параметр задержки смены ¤ркости
unsigned int counter_light = 0;
float h_new, s_new, v_new; //новые hsv дл¤ toHSV()
float h = 1;  //текущие значени¤ hsv
float s = 1;
float v = 0;
uint8_t *pinPtr;
float r, g, b;
float &white;
int d_light = 0;
uint8_t mode = 0;
unsigned int duty;
HardwarePWM *HW_pwm;

	void startTimer()
	{
		if(!ledTimer.isStarted())
		{
			ledTimer.start();
		}
	}
};

#endif /* LIGHT_HANDLER_H_ */