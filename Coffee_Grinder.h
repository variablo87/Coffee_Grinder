/*!
 * @file Coffee_Grinder.h
 *
 * This is part of Coffee Grinder.
 *
 *
 * Written by variablo87.
 *
 *
 */

#ifndef _COFFEE_GRINDER_H_
#define _COFFEE_GRINDER_H_


#include <Arduino.h>
#include <EEPROM.h>
#include <WebSocketsServer.h>
#include "HX711.h"

typedef struct grinderSetting{
	int autoMode;
	int manuMode;
	int setpoint_weight;
	int updateTime;
	int scale_is_connected;
	int calibration_weight;
};

typedef union MEM_Grinder{
	grinderSetting grinder;
	int webSetting[sizeof(grinderSetting)];
};

class Coffee_Grinder
{
  public:
    Coffee_Grinder(int relaisPin,int outNegated,int sckPin, int doutPin);
	
	void setup();
    void loop();
    void setScaleFaktor();
  	void resetScale();
  	void tare();
  	void start();
  	void stop();
  	  	
  	MEM_Grinder mem;
  	float weight;
  	float scale_faktor;
    uint8_t socketNumber;
  private:
    int neg;
    int m_relaisPin;
  	int m_sckPin; 
  	int m_doutPin;
  	long scale_offset;
  	
  	
  	float calibration_factor;
  		
  	unsigned long currentTime;
  	unsigned long loopTime;
  	
  	HX711 scale;
};

#endif

