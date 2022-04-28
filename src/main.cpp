// #include <Arduino.h>
//#include <SoftwareSerial.h>

// Coba pakai D1,D2,D5,D6 on NodeMcu



#include <ESPAsyncWebServer.h>
#include <SPIFFSEditor.h>
#include "FSWebServerLib.h"
#include "timehelper.h"
// #include "config.h"
#include "modbus.h"
#include "mqtt.h"
// #include "dhtlib.h"
#include "thingspeakhelper.h"
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
  pinMode(2, OUTPUT); // Initialize the BUILTIN_LED pin as an output
  //pinMode(D5, INPUT_PULLUP);
  DEBUGPORT.begin(115200);

#ifndef RELEASE
  DEBUGPORT.setDebugOutput(true);
#endif

  DEBUGLOG("Mounting FS...\r\n");
  if (!LittleFS.begin())
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

  // DEBUGLOG("Setup MQTT...\r\n");
  // mqtt_setup();

  DEBUGLOG("Starting ESPHTTPServer...\r\n");
  ESPHTTPServer.start(&LittleFS);

  // Timesetup();

// #if defined(SOFTWARESERIAL)
//   Serial.swap();
// #endif
  DEBUGLOG("Setup AsyncPING...\r\n");
  PingSetup();
  DEBUGLOG("Setup MQTT...\r\n");
  mqtt_setup();

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

  Thingspeaksetup();

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

  // utcTime = now;

  // static unsigned long prevTimer500ms = 0;
  // // static unsigned long prevTimer1000ms = 0;

  // static time_t prevDisplay;

  // if (utcTime != prevDisplay)
  // {
  //   // unsigned long currMilis = millis();
  //   // prevTimer500ms = currMilis;
  //   // prevTimer1000ms = currMilis;
  //   tick1000ms = true;
  //   prevDisplay = utcTime;
  // }

  // if (millis() < prevTimer500ms + 500)
  // {
  //   state500ms = true;
  // }
  // else
  // {
  //   state500ms = false;
  // }

  // if (millis() < prevTimer500ms + 1000)
  // {
  //   state1000ms = true;
  // }
  // else
  // {
  //   state1000ms = false;
  // }

  //  if (digitalRead(D5) == LOW && oldState == HIGH) {
  //    wifi_restart();
  //    oldState = LOW;
  //    digitalWrite(led, HIGH);
  //  }
  //  else if (digitalRead(D5) == HIGH) {
  //    oldState = HIGH;
  //    digitalWrite(led, LOW);
  //  }

  // if (tick1000ms)
  // {
  // }
  // mqtt_loop();
  modbus_update();
  modbus_loop(); 
  ESPHTTPServer.loop();
  // ftpSrv.handleFTP();

  // modbus_loop();
  

  
  // modbus_update();

  // dht_loop();
  mqtt_loop();

  // TimeLoop();

  Thingspeakloop();

  //wifi_loop();
  //ESP.wdtFeed();

  // tick500ms = false;
  // tick1000ms = false;
}