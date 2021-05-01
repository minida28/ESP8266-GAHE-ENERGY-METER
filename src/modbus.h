#ifndef _MYMODBUS_H
#define _MYMODBUS_H

#include "SimpleModbusMaster.h"

// This is the easiest way to create new packets
// Add as many as you want. TOTAL_NO_OF_PACKETS
// is automatically updated.
enum
{
  PACKET1,
  PACKET2,
  PACKET3,
  PACKET4,
  // PACKET5,
  // PACKET6,
  // PACKET7,
  // PACKET8,
  // PACKET9,
  // PACKET10,
  // PACKET11,
  // PACKET12,
  // PACKET13,
  // PACKET14,
  TOTAL_NO_OF_PACKETS // leave this last entry
};

extern Packet packets[];

//extern SoftwareSerial swSer;

extern char bufwattThreshold[10];
extern char bufCurrentThreshold[10];
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


extern uint16_t wattThreshold;
extern float currentThreshold;


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
extern void modbus_loop_1();


#endif
