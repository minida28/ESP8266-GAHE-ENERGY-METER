#ifndef _MYMODBUS_H
#define _MYMODBUS_H

#include "SimpleModbusMaster.h"
//#include <SoftwareSerial.h>
#include "config.h"



//extern SoftwareSerial swSer;

extern char bufwattThreshold[10];
extern char bufVoltage[10];
extern char bufAmpere[10];
extern char bufWatt[10];
extern char bufVar[10];
extern char bufFrequency[10];
extern char bufPstkwh[10];
extern char bufPstkvarh[10];
extern char bufNgtkvarh[10];
extern char bufPowerFactor[10];
extern char bufApparentPower[10];
extern char bufUnk2[10];
extern char bufRequestsPACKET3[10];
extern char bufSuccessful_requestsPACKET3[10];
extern char bufFailed_requestsPACKET3[10];
extern char bufException_errorsPACKET3[10];
extern char bufConnectionPACKET3[10];


// -------------------------------------------------------------------
// Setting-up Modbus
// -------------------------------------------------------------------
extern void modbus_setup();


// -------------------------------------------------------------------
// Reading registers
//
// Call every time around loop()
// -------------------------------------------------------------------
extern void modbus_loop();


#endif
