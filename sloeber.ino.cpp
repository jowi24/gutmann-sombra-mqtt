#ifdef __IN_ECLIPSE__
//This is a automatic generated file
//Please do not modify this file
//If you touch this file your change will be overwritten during the next build
//This file has been generated on 2018-01-15 23:43:04

#include "Arduino.h"
#include "Arduino.h"
#include "IotBase.h"

void mqtt_callback(char* topic, byte* payload, unsigned int length) ;
ICACHE_RAM_ATTR void IsrResync() ;
ICACHE_RAM_ATTR void IsrTimer() ;
ICACHE_RAM_ATTR void checkButtons(int inputPin) ;
ICACHE_RAM_ATTR void updateButtonState(int buttonId, int buttonPin) ;
ICACHE_RAM_ATTR void checkLeds(int inputPin) ;
ICACHE_RAM_ATTR void updateLedStateCounter(int ledId, int ledPinState) ;
void setup() ;
void loop() ;

#include "HoodControl.ino"


#endif
