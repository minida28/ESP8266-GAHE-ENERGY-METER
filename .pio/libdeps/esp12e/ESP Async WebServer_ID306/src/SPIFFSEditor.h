#ifndef SPIFFSEditor_H_
#define SPIFFSEditor_H_

#define USE_LittleFS
#define SPIFFS MYFS

#include <ArduinoOTA.h>
#ifdef ESP32
 #include <FS.h>
 #ifdef USE_LittleFS
  #define MYFS LITTLEFS
  #include "LITTLEFS.h"
 #elif defined(USE_FatFS)
  #define MYFS FFat
  #include "FFat.h"
 #else
  #define MYFS SPIFFS
  #include <SPIFFS.h>
 #endif
 #include <ESPmDNS.h>
 #include <WiFi.h>
 #include <AsyncTCP.h>
#elif defined(ESP8266)
 #ifdef USE_LittleFS
  #include <FS.h>
  #define MYFS LittleFS
  #include <LittleFS.h> 
 #elif defined(USE_FatFS)
  #error "FatFS only on ESP32 for now!"
 #else
  #define MYFS SPIFFS
 #endif
 #include <ESP8266WiFi.h>
 #include <ESPAsyncTCP.h>
 #include <ESP8266mDNS.h>
#endif

#include <ESPAsyncWebServer.h>

class SPIFFSEditor: public AsyncWebHandler {
  private:
    fs::FS _fs;
    String _username;
    String _password; 
    bool _authenticated;
    uint32_t _startTime;
  public:
#ifdef ESP32
    SPIFFSEditor(const fs::FS& fs, const String& username=String(), const String& password=String());
#else
    SPIFFSEditor(const String& username=String(), const String& password=String(), const fs::FS& fs=SPIFFS);
#endif
    virtual bool canHandle(AsyncWebServerRequest *request) override final;
    virtual void handleRequest(AsyncWebServerRequest *request) override final;
    virtual void handleUpload(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) override final;
    virtual bool isRequestHandlerTrivial() override final {return false;}
};

#endif
