// #include <Arduino.h>
//#include <SoftwareSerial.h>

// Coba pakai D1,D2,D5,D6 on NodeMcu



// #include <ESPAsyncWebServer.h>
// #include <SPIFFSEditor.h>
#include "FSWebServerLib.h"
#include "timehelper.h"
// #include "config.h"
#include "modbus.h"
#include "mqtt.h"
// #include "dhtlib.h"
// #include "thingspeakhelper.h"
#include "asyncpinghelper.h"

// #include <ESP8266FtpServer.h>


//#define RELEASE

#define DEBUGPORT Serial

#ifndef RELEASE
#define DEBUGLOG(fmt, ...)                   \
  {                                          \
    static const char pfmt[] PROGMEM = fmt;  \
    DEBUGPORT.printf_P(pfmt, ##__VA_ARGS__); \
  }
#else
#define DEBUGLOG(...)
#endif

// FtpServer ftpSrv;   //set #define FTP_DEBUG in ESP8266FtpServer.h to see ftp verbose on serial



void setup()
{
  
  //pinMode(D5, INPUT_PULLUP);
  DEBUGPORT.begin(115200);

#ifndef RELEASE
  DEBUGPORT.setDebugOutput(true);
#endif

  DEBUGLOG("Mounting FS...\r\n");
  if (!MYFS.begin())
  {
    DEBUGLOG("Failed to mount file system\r\n");
    return;
  } 

  DEBUGLOG("Setup MODBUS...\r\n");
  modbus_setup();

  DEBUGLOG("RTC Time...\r\n");
  RtcSetup();

  DEBUGLOG("Setup Time...\r\n");
  TimeSetup();

  DEBUGLOG("Setup MQTT...\r\n");
  mqtt_setup();

  DEBUGLOG("Setup AsyncPING...\r\n");
  PingSetup();

  DEBUGLOG("Starting ESPHTTPServer...\r\n");
  ESPHTTPServer.start(&MYFS);

  // DEBUGLOG("RTC Time...\r\n");
  // RtcSetup();
  // DEBUGLOG("Setup Time...\r\n");
  // TimeSetup();

  waitingForInternetConnectedTimer.once(30, FlipWaitingForInternet);

  nextSync = 30;

  // -------------------------------------------------------------------
  // Load configuration from EEPROM & Setup Async Server
  // -------------------------------------------------------------------

  // config_load_settings();

  // WiFi.setSleepMode(WIFI_NONE_SLEEP);

  // DEBUGLOG("Starting ESPHTTPServer...\r\n");
  // ESPHTTPServer.start(&SPIFFS);

  // modbus_setup();

  // Thingspeaksetup();

  pinMode(2, OUTPUT); // Initialize the BUILTIN_LED pin as an output

  DEBUGLOG("Setup completed!\r\n\r\n");
}

bool oldState;
// time_t utcTime;
// bool state500ms;
// bool state1000ms;
bool tick500ms;
// bool tick1000ms;


void loop()
{
  TimeLoop();

  modbus_update();
  modbus_loop(); 
  ESPHTTPServer.loop();

  mqtt_loop();
}