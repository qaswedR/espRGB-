#include "../include/lightHandler.hpp"
#include "../include/configuration.h"

#define period  1000       // умножить на 65 дл¤ 1024. duty = 66666

  void HSVtoRGB(float *r, float *g, float *b, float h, float s, float v);

  static LightHandler *Instance = nullptr;
 

 LightHandler::LightHandler(uint8_t *pins, float *_white, float *_voff, bool *_motionFlag):
 white(*_white),
 voff(*_voff),
 pinPtr(pins),
 motionFlag(*_motionFlag)
 {
		Instance = this;
   HW_pwm = new HardwarePWM(pins, 4);
   HW_pwm->setPeriod(period);
   setDuty(HW_pwm->getMaxDuty());
        Instance->ledTimer.initializeMs(10, [](){
			Instance->loop();
		}).start();
 }
  
void LightHandler::handleHsvImpl(HttpRequest &request, HttpResponse &response)
{
	//ledTimer.restart();
		response.setContentType(ContentType::HTML);
		response.sendString(OK_RESPONCE);
	String paramWhite = request.getQueryParameter("white");
	int iParamWhite = paramWhite.toInt();
	if (paramWhite.length() > 0 && iParamWhite > 0 && iParamWhite <= 100) {
		if (iParamWhite == 1) {
			white = 1;
		} else if (iParamWhite == 2) {

		  HW_pwm->analogWrite(w_pin,  duty - 1);
			white = 2;
			if(!whiteOffTimer)
			{
		    	whiteOffTimer = new Timer;
			}
			whiteOffTimer->initializeMs(100, [](){
				Instance->white = 0;
		        Instance->HW_pwm->analogWrite(w_pin, 0);
				delete Instance->whiteOffTimer;
				Instance->whiteOffTimer = nullptr;
				}).startOnce();
		}

	} else {
		if (white < 3)
			white = 0;
	}

	String paramDelay = request.getQueryParameter("delay");
	int iparamDelay = paramDelay.toInt();
	if (paramDelay.length() > 0) {
		if (iparamDelay >= 0 && iparamDelay < 256) {
			d_light = iparamDelay;
		}
	} /*else {
	 d = 0;
	 }*/

	String paramV = request.getQueryParameter("v");
	float iparamV = paramV.toFloat();

	if (paramV.length() > 0) {
		if (request.getQueryParameter("v_new").length() > 0) {
			if (iparamV >= 0 && iparamV <= 100) {
				v = iparamV;

				saveConfig(vFile, String(v));
			}
			if (request.getQueryParameter("v_new").toFloat () >= 0
					&& request.getQueryParameter("v_new").toFloat () <= 100) {
				v_new = request.getQueryParameter("v_new").toFloat ();

			}
		} else {
			v_new = iparamV;
			if(voff == v_new)
			{
				motionFlag = 0;
			}
		}
	} else {
		if (request.getQueryParameter("v_new").length() > 0
				&& request.getQueryParameter("v_new").toFloat () >= 0.0
				&& request.getQueryParameter("v_new").toFloat () <= 100.0) {
			v_new = request.getQueryParameter("v_new").toFloat ();
		}
	}

	String paramMode = request.getQueryParameter("mode");
	float iparamMode = paramMode.toFloat ();
	String paramH = request.getQueryParameter("h");
	float iparamH = paramH.toFloat ();
	String paramS = request.getQueryParameter("s");
	float iparamS = paramS.toFloat ();

	if (paramH.length() > 0 && iparamH >= 0 && iparamH <= 360) {
		if(paramMode.length() > 0 && iparamMode > 0 && iparamMode <= 10) {
			mode = iparamMode;
			h_new = iparamH;
			}
			else{
				h = iparamH;
				}
		}else if(paramMode.length() > 0 && iparamMode > 0 && iparamMode <= 10)
		{
			mode = iparamMode;	
		}
		
			if (paramS.length() == 0) {
				if(white != 2)
				{
			       if(mode == 2)
				   {
				    s_new = 100;
				   }
				   else
				   {
				   s = 100;
				   }
				}
				
			} else {
				if (iparamS >= 0 && iparamS <= 100) {
				if(mode == 2)
				{
				 s_new = iparamS;
				}
				else
				{
					s = iparamS;
				}
				}
			}
			
	
	{
		ledTimer.start();
	}
//	sendLog("handleHsv");

}

void LightHandler::handleHsv(HttpRequest &request, HttpResponse &response) 
{
	Instance->handleHsvImpl(request, response);
}

  
void LightHandler::loop() {
	counter_light++;
	counterSmooth++;
	bool needStop = true;
	if (counter_light >= d_light) {
		switch (mode) {

		case 0:  //мгновенное включение цвета и плавное ¤ркости
		{
			//if (v == v_new)
			//ledTimer.stop();
			break;
		}

		case 1:                          //h туда сюда
			// если count = 1
		{
			needStop = false;
			if (countUp == true) {
				h += 0.2;
				if (h >= 359) {
					h = 359;
					countUp = false;
				}
			} else {
				h -= 0.2;
				if (h <= 0) {
					h = 0;
					countUp = true;
				}
			}
			break;
		}
		case 2:   		// выход h на заданынй
		{

			if (h < h_new) {
			needStop = false;
				h+= 1;
				if (h >= h_new) {
					h = h_new;
					//if (v == v_new)
					//ledTimer.stop();
				}
			}

			if (h > h_new) {
			needStop = false;
				h-= 1;
				if (h <= h_new) {
					h = h_new;
					//if (v == v_new)
					//ledTimer.stop();
				}
			}
			
			if (s < s_new) {
			needStop = false;
				s+= 0.5;
				if (s >= s_new) {
					s = s_new;
					//if (v == v_new)
					//ledTimer.stop();
				}
			}

			if (s > s_new) {
			    needStop = false;
				s-= 0.5;
				if (s <= s_new) {
					s = s_new;
					//if (v == v_new)
					//ledTimer.stop();
				}
			}
		///	if (h == h_new)
			//	if (v == v_new)
					//ledTimer.stop();
					break;
		}
		case 3:   		// выход h на заданынй
		{

			if (countUp == true) {
				h += 1;
				if (h >= 359) {
					h = 359;
					countUp = false;
				}
			} else {
				h -= 1;
				if (h <= 0) {
					h = 0;
					countUp = true;
				}
			}

			break;
		}

		}

		counter_light = 0;
	}
	// ¤ркость на позицию

	if (counterSmooth >= smoothD) {
		if (v < v_new) {
			needStop = false;
			v+=0.2;
			if (v >= v_new) {
				v = v_new;
			}
		}
		if (v > v_new) {
			needStop = false;
			v-=0.2;
			if (v <= v_new) {
				v = v_new;
			}
		}
		counterSmooth = 0;
	}

	HSVtoRGB(&r, &g, &b, h, s, v);
	r *= duty;
	g *= duty;
	b *= duty;
	HW_pwm->analogWrite(pinPtr[RedPin], r);
	//delay(1);
	HW_pwm->analogWrite(pinPtr[GreenPin], g);
	//delay(1);
	HW_pwm->analogWrite(pinPtr[BluePin], b);
	//delay(1);

	if (white == 1) {
		if(mode == 2)
		{
			if(s < 45)
			 HW_pwm->analogWrite(w_pin, v * duty / 100);
			 else
			 HW_pwm->analogWrite(w_pin,0);
			 
		}
		else
		{
		  HW_pwm->analogWrite(w_pin, v * duty / 100);
		}
	} else if (white == 0) {
		HW_pwm->analogWrite(w_pin, 0);
	}
	
	if(needStop)
	{
		ledTimer.stop();
	}
	//digitalWrite(15,1);
	////Serial.println(w);//Serial.println(r);//Serial.println(g);//Serial.println(b);

}
  
  
  
  
  
  
  
  
  
  
  void HSVtoRGB(float *r, float *g, float *b, float h, float s, float v) {
	float sn = (float) s / 100;
	float vn = (float) v / 100;

	int i;
	float f, p, q, t;
	if (s == 0) {
		// achromatic (grey)
		*r = *g = *b = vn;
		return;
	}
	h /= 60;      // sector 0 to 5
	i = floor(h);
	f = h - i;      // factorial part of h
	p = vn * (1 - sn);
	q = vn * (1 - sn * f);
	t = vn * (1 - sn * (1 - f));
	switch (i) {
	case 0:
		*r = vn;
		*g = t;
		*b = p;
		break;
	case 1:
		*r = q;
		*g = vn;
		*b = p;
		break;
	case 2:
		*r = p;
		*g = vn;
		*b = t;
		break;
	case 3:
		*r = p;
		*g = q;
		*b = vn;
		break;
	case 4:
		*r = t;
		*g = p;
		*b = vn;
		break;
	default:    // case 5:
		*r = vn;
		*g = p;
		*b = q;
		break;
	}

}