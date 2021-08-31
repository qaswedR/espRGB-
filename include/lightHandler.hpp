#ifndef LIGHT_HANDLER_H_
#define LIGHT_HANDLER_H_

#include <SmingCore/HardwarePWM.h>
#include <SmingCore/SmingCore.h>


#define OK_RESPONCE "Ok.\n <br/>"
#define w_pin  15


#pragma once
enum pinsEnum{
  RedPin,
  GreenPin,
  BluePin,
  WhitePin
};


class LightHandler {
public:

  LightHandler(uint8_t *pins, float *white, float *voff, bool *motionFlag);
public:
static void ICACHE_FLASH_ATTR handleHsv(HttpRequest &request, HttpResponse &response);
void ICACHE_FLASH_ATTR handleHsvImpl(HttpRequest &request, HttpResponse &response);
  void ICACHE_FLASH_ATTR setVNew(float _v)
  {
	  v_new = _v;
	  ledTimer.start();
  }
  
  void ICACHE_FLASH_ATTR setHNew(float _h)
  {
	  h_new = _h;
  }
  
  void ICACHE_FLASH_ATTR setSNew(float _s)
  {
	  s_new = _s;
  }
  
  void ICACHE_FLASH_ATTR setMode(uint8_t _mode)
  {
	  mode = _mode;
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
  float ICACHE_FLASH_ATTR getV()
  {
	  return v;
  }
  bool ICACHE_FLASH_ATTR ledTimerIsStarted()
  {
	  return ledTimer.isStarted();
  }
  void ICACHE_FLASH_ATTR setWhite(bool isHight)
  {
	  HW_pwm->analogWrite(w_pin, isHight ?  duty - 1 : 0);
  }
void ICACHE_FLASH_ATTR loop();
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
float &white, &voff;
int d_light = 0;
uint8_t mode = 0;
unsigned int duty;
bool &motionFlag;
HardwarePWM *HW_pwm;
};

#endif /* LIGHT_HANDLER_H_ */