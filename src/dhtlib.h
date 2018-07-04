#ifndef _TEMPLIB_H
#define _TEMPLIB_H

#include "DHT.h"
#include <Adafruit_Sensor.h>
#include "config.h"

// -------------------------------------------------------------------
// Reading temperature and humidity
//
// Call every time around loop()
// -------------------------------------------------------------------
extern void dht_loop();

extern void dht_setup();

#endif
