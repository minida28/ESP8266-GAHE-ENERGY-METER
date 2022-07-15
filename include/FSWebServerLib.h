// FSWebServerLib.h

#ifndef _FSWEBSERVERLIB_h
#define _FSWEBSERVERLIB_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

/// Defaulut is SPIFFS, FatFS: only on ESP32, also choose partition scheme w/ ffat.
// Comment 2 lines below or uncomment only one of them

#define USE_LittleFS
//#define USE_FatFS // Only ESP32

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
#include <SPIFFSEditor.h>




#include <Ticker.h>



//#define RELEASE  // Comment to enable debug output

// #define DBG_OUTPUT_PORT Serial1
// #define DEBUGPORT Serial1

// #ifndef RELEASE
// #define DEBUGLOG(fmt, ...) { static const char pfmt[] PROGMEM_T = fmt; DEBUGPORT.printf_P(pfmt, ## __VA_ARGS__); }
// #else
// #define DEBUGLOG(...)
// #endif


#define ESP_PIN_0 0   //D3
#define ESP_PIN_1 1   //Tx
#define ESP_PIN_2 2   //D4 -> Led on NodeMcu
#define ESP_PIN_3 3   //D9(Rx)
#define ESP_PIN_4 4   //D2
#define ESP_PIN_5 5   //D1
#define ESP_PIN_9 9   //S2
#define ESP_PIN_10 10 //S3
#define ESP_PIN_12 12 //D6
#define ESP_PIN_13 13 //D7
#define ESP_PIN_14 14 //D5
#define ESP_PIN_15 15 //D8
#define ESP_PIN_16 16 //D0 -> Led on ESP8266


#define CONNECTION_LED -1           // Connection LED pin (Built in). -1 to disable
#define AP_ENABLE_BUTTON ESP_PIN_12 //D6 wemos; Button pin to enable AP during startup for configuration. -1 to disable

#define CONFIG_FILE "/config.json"
#define SECRET_FILE "/secret.json"
#define DESCRIPTION_XML_FILE "/description.xml"

extern FSInfo fs_info;

extern AsyncWebSocket ws;
extern AsyncEventSource evs;
extern uint32_t clientID;

//extern const char Page_WaitAndReload[];

typedef struct
{
  char mode[7] = "AP";
  char hostname[32] = "ESP_XXXX";
  char ssid[32];
  char password[32];
  bool dhcp = true;
  char static_ip[16] = "192.168.10.15";
  char netmask[16] = "255.255.255.0";
  char gateway[16] = "192.168.10.1";
  char dns0[16] = "192.168.10.1";
  char dns1[16] = "8.8.8.8";

} strConfig;

// typedef struct
// {
//   int8_t timezone = 70;
//   bool dst = false;
//   bool enablertc = true;
//   uint32_t syncinterval = 600;
//   bool enablentp = true;
//   char ntpserver_0[48] = "0.id.pool.ntp.org";
//   char ntpserver_1[48] = "0.asia.pool.ntp.org";
//   char ntpserver_2[48] = "192.168.10.1";
// } strConfigTime;

typedef struct
{
  String APssid = "ESP"; // ChipID is appended to this name
  String APpassword = "";
  bool APenable = false; // AP disabled by default
} strApConfig;

typedef struct
{
  bool auth;
  String wwwUsername;
  String wwwPassword;
} strHTTPAuth;

class AsyncFSWebServer : public AsyncWebServer
{
public:
  AsyncFSWebServer(uint16_t port);
  void start(FS *fs);
  void loop();
  //AsyncWebSocket _ws = AsyncWebSocket("/ws");
  strConfig _config; // General and WiFi configuration

protected:
  // strConfig _config; // General and WiFi configuration
  // strConfigTime _configTime;
  strApConfig _apConfig; // Static AP config settings
  strHTTPAuth _httpAuth;
  FS *_fs;
  long wifiDisconnectedSince = 0;
  String _browserMD5 = "";
  uint32_t _updateSize = 0;

  WiFiEventHandler onStationModeConnectedHandler;
  WiFiEventHandler onStationModeDisconnectedHandler;
  WiFiEventHandler onStationModeGotIPHandler;

  //uint currentWifiStatus;

  Ticker _secondTk;
  Ticker espRestartTimer;
  bool _secondFlag;

  //AsyncWebSocket _ws("/ws"); // access at ws://[esp ip]/ws
  //AsyncWebSocket _ws = AsyncWebSocket("/ws");

  // AsyncEventSource _evs = AsyncEventSource("/events");

  // void sendTimeData();
  bool load_config();
  void defaultConfig();
  bool save_config();
  bool loadHTTPAuth();
  bool saveHTTPAuth();
  void configureWifiAP();
  void configureWifi();
  void configureWifi2();
  void ConfigureOTA(String password);
  void serverInit();

  void onWiFiConnected(WiFiEventStationModeConnected data);
  void onWiFiDisconnected(WiFiEventStationModeDisconnected data);
  void onWifiGotIP(WiFiEventStationModeGotIP data);

  // static void s_secondTick(void *arg);

  String getMacAddress();

  bool checkAuth(AsyncWebServerRequest *request);
  void handleFileList(AsyncWebServerRequest *request);
  //void handleFileRead_edit_html(AsyncWebServerRequest *request);
  //bool handleFileRead(String path, AsyncWebServerRequest *request);
  //void handleFileCreate(AsyncWebServerRequest *request);
  //void handleFileDelete(AsyncWebServerRequest *request);
  //void handleFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
  void send_general_configuration_values_html(AsyncWebServerRequest *request);
  void send_network_configuration_values_html(AsyncWebServerRequest *request);
  void send_connection_state_values_html(AsyncWebServerRequest *request);
  void send_information_values_html(AsyncWebServerRequest *request);
  void send_NTP_configuration_values_html(AsyncWebServerRequest *request);
  void send_network_configuration_html(AsyncWebServerRequest *request);
  void send_NTP_configuration_html(AsyncWebServerRequest *request);
  void restart_esp(AsyncWebServerRequest *request);
  void send_wwwauth_configuration_values_html(AsyncWebServerRequest *request);
  void send_wwwauth_configuration_html(AsyncWebServerRequest *request);
  void send_update_firmware_values_html(AsyncWebServerRequest *request);
  void setUpdateMD5(AsyncWebServerRequest *request);
  void updateFirmware(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);

  void send_config_network(AsyncWebServerRequest *request);
  void send_config_mqtt(AsyncWebServerRequest *request);
  bool load_config_network();
  bool load_config_time();
  bool save_config_network();
  bool save_config_time();

  

  static String urldecode(String input); // (based on https://code.google.com/p/avr-netino/)
  static unsigned char h2int(char c);
  static boolean checkRange(String Value);

  //void send_meter_reading(AsyncWebServerRequest *request);
  //void send_classic_page(AsyncWebServerRequest *request);
  void send_classic_xml_page(AsyncWebServerRequest *request);
  // void send_ssdp_xml_page(AsyncWebServerRequest *request);
  //void send_test_page(AsyncWebServerRequest *request);

  void handleSaveMqtt(AsyncWebServerRequest *request);
  //void handleStatus(AsyncWebServerRequest *request);
  void handleRst(AsyncWebServerRequest *request);
  void handleSaveNetwork(AsyncWebServerRequest *request);

  void sendHeap(uint8_t mode);
};

extern AsyncFSWebServer ESPHTTPServer;
void esp_restart();

extern bool sendDateTimeFlag;
extern bool wifiGotIpFlag;
extern bool saveConfigTimeFlag;


void runAsyncClientEmoncms();
void runAsyncClientThingspeak();

bool save_config_time();

// extern uint16_t num;

//extern char bufXML[1000];
//extern void handleXML();
extern String millis2time();


#endif // _FSWEBSERVERLIB_h
