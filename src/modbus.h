#ifndef _MYMODBUS_H
#define _MYMODBUS_H

#include <SimpleModbusMaster.h>
//#include <SoftwareSerial.h>
#include "config.h"



//extern SoftwareSerial swSer;


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
