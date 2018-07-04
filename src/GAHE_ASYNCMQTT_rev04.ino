#include <Arduino.h>
//#include <SoftwareSerial.h>

// Coba pakai D1,D2,D5,D6 on NodeMcu


#include "FSWebServerLib.h"
#include "config.h"
#include "modbus.h"
#include "mqtt.h"
#include "dhtlib.h"


// #include <ESP8266FtpServer.h>

// FtpServer ftpSrv;   //set #define FTP_DEBUG in ESP8266FtpServer.h to see ftp verbose on serial



void setup()
{
  //pinMode(LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  //pinMode(D5, INPUT_PULLUP);
  Serial.begin(115200);
  Serial.setDebugOutput(false);
  //Serial1.begin(115200);

#if defined(SOFTWARESERIAL)
  Serial.swap();
#endif



  display_srcfile_details();

  Serial.print(F("Reset reason: "));
  Serial.println(ESP.getResetReason());
  Serial.println();
  Serial.print(F("Reset info: "));
  Serial.println(ESP.getResetInfo());
  Serial.print(F("Core version: "));
  Serial.println(ESP.getCoreVersion());
  Serial.print(F("SDK version: "));
  Serial.println(ESP.getSdkVersion());
  Serial.print(F("CPU Freq: "));
  Serial.println(ESP.getCpuFreqMHz());
  Serial.println();



  // -------------------------------------------------------------------
  // Mount File system
  // -------------------------------------------------------------------
  Serial.println(F("Mounting FS..."));

  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }
  else
  {
    // ftpSrv.begin("esp8266","esp8266");
  }



  // -------------------------------------------------------------------
  // SETUP MQTT
  // -------------------------------------------------------------------
  Serial.println(F("Setup MQTT..."));

  mqtt_setup();

  // -------------------------------------------------------------------
  // Load configuration from EEPROM & Setup Async Server
  // -------------------------------------------------------------------

  config_load_settings();

  // wifi_setup();

  SPIFFS.begin(); // Not really needed, checked inside library and started if needed
  
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  ESPHTTPServer.begin(&SPIFFS);

  modbus_setup();

  //--- DHT Adafruit
  dht_setup();

  // Initialise MQTT
  // mqtt_setup();

  save_system_info();

  DEBUGLOG("Setup done!");

}

bool oldState;
time_t utcTime;
bool state500ms;
bool state1000ms;
bool tick500ms;
bool tick1000ms;

void loop()
{
  utcTime = now();

  static unsigned long prevTimer500ms = 0;
  // static unsigned long prevTimer1000ms = 0;

  static time_t prevDisplay;

  if (utcTime != prevDisplay) {
    unsigned long currMilis = millis();
    prevTimer500ms = currMilis;
    // prevTimer1000ms = currMilis;
    tick1000ms = true;
    prevDisplay = utcTime;
  }

  if (millis() < prevTimer500ms + 500) {
    state500ms = true;
  }
  else {
    state500ms = false;
  }

  if (millis() < prevTimer500ms + 1000) {
    state1000ms = true;
  }
  else {
    state1000ms = false;
  }

  //  if (digitalRead(D5) == LOW && oldState == HIGH) {
  //    wifi_restart();
  //    oldState = LOW;
  //    digitalWrite(led, HIGH);
  //  }
  //  else if (digitalRead(D5) == HIGH) {
  //    oldState = HIGH;
  //    digitalWrite(led, LOW);
  //  }

  if (tick1000ms) {

  }

  ESPHTTPServer.handle();
  // ftpSrv.handleFTP();
  modbus_update();
  modbus_loop();
  dht_loop();
  //mqtt_loop();
  //wifi_loop();
  //ESP.wdtFeed();

  tick500ms = false;
  tick1000ms = false;
}

int pgm_lastIndexOf(uint8_t c, const char * p)
{
  int last_index = -1; // -1 indicates no match
  uint8_t b;
  for (int i = 0; true; i++) {
    b = pgm_read_byte(p++);
    if (b == c)
      last_index = i;
    else if (b == 0) break;
  }
  return last_index;
}

// displays at startup the Sketch running in the Arduino
void display_srcfile_details(void) {
  const char *the_path = PSTR(__FILE__);           // save RAM, use flash to hold __FILE__ instead

  int slash_loc = pgm_lastIndexOf('/', the_path); // index of last '/'
  if (slash_loc < 0) slash_loc = pgm_lastIndexOf('\\', the_path); // or last '\' (windows, ugh)

  int dot_loc = pgm_lastIndexOf('.', the_path);  // index of last '.'
  if (dot_loc < 0) dot_loc = pgm_lastIndexOf(0, the_path); // if no dot, return end of string

  Serial.print(F("\nSketch name: "));

  for (int i = slash_loc + 1; i < dot_loc; i++) {
    uint8_t b = pgm_read_byte(&the_path[i]);
    if (b != 0) Serial.print((char) b);
    else break;
  }
  Serial.println();

  Serial.print(F("Compiled on: "));
  Serial.print(__DATE__);
  Serial.print(F(" at "));
  Serial.println(__TIME__);
  Serial.println();
}

bool save_system_info() {
  PRINT("%s\r\n", __PRETTY_FUNCTION__);

  // const char* pathtofile = PSTR(pgm_filesystemoverview);

  File file;
  if (!SPIFFS.exists(FPSTR(pgm_systeminfofile)))
  {
    file = SPIFFS.open(FPSTR(pgm_systeminfofile), "w");
    if (!file) {
      PRINT("Failed to open config file for writing\r\n");
      file.close();
      return false;
    }
    //create blank json file
    PRINT("Creating user config file for writing\r\n");
    file.print("{}");
    file.close();
  }
  //get existing json file
  file = SPIFFS.open(FPSTR(pgm_systeminfofile), "w");
  if (!file) {
    PRINT("Failed to open config file");
    return false;
  }

  const char* the_path = PSTR(__FILE__);
  // const char* _compiletime = PSTR(__TIME__);

  int slash_loc = pgm_lastIndexOf('/', the_path); // index of last '/'
  if (slash_loc < 0) slash_loc = pgm_lastIndexOf('\\', the_path); // or last '\' (windows, ugh)

  int dot_loc = pgm_lastIndexOf('.', the_path);  // index of last '.'
  if (dot_loc < 0) dot_loc = pgm_lastIndexOf(0, the_path); // if no dot, return end of string

  int lenPath = strlen(the_path);
  int lenFileName = (lenPath - (slash_loc + 1));

  char fileName[lenFileName];
  //Serial.println(lenFileName);
  //Serial.println(sizeof(fileName));

  int j = 0;
  for (int i = slash_loc + 1; i < lenPath; i++) {
    uint8_t b = pgm_read_byte(&the_path[i]);
    if (b != 0) {
      fileName[j] = (char) b;
      //Serial.print(fileName[j]);
      j++;
      if (j >= lenFileName) {
        break;
      }
    }
    else {
      break;
    }
  }
  //Serial.println();
  //Serial.println(j);
  fileName[lenFileName] = '\0';

  //const char* _compiledate = PSTR(__DATE__);
  int lenCompileDate = strlen_P(PSTR(__DATE__));
  char compileDate[lenCompileDate];
  strcpy_P(compileDate, PSTR(__DATE__));

  int lenCompileTime = strlen_P(PSTR(__TIME__));
  char compileTime[lenCompileTime];
  strcpy_P(compileTime, PSTR(__TIME__));

  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();

  root[FPSTR(pgm_filename)] = fileName;
  root[FPSTR(pgm_compiledate)] = compileDate;
  root[FPSTR(pgm_compiletime)] = compileTime;
  root[FPSTR(pgm_lastboot)] = NTP.getTimeDateString(NTP.getLastBootTime());
  root[FPSTR(pgm_chipid)] = ESP.getChipId();
  root[FPSTR(pgm_cpufreq)] = ESP.getCpuFreqMHz();
  root[FPSTR(pgm_sketchsize)] = ESP.getSketchSize();
  root[FPSTR(pgm_freesketchspace)] = ESP.getFreeSketchSpace();
  root[FPSTR(pgm_flashchipid)] = ESP.getFlashChipId();
  root[FPSTR(pgm_flashchipmode)] = ESP.getFlashChipMode();
  root[FPSTR(pgm_flashchipsize)] = ESP.getFlashChipSize();
  root[FPSTR(pgm_flashchiprealsize)] = ESP.getFlashChipRealSize();
  root[FPSTR(pgm_flashchipspeed)] = ESP.getFlashChipSpeed();
  root[FPSTR(pgm_cyclecount)] = ESP.getCycleCount();
  root[FPSTR(pgm_corever)] = ESP.getCoreVersion();
  root[FPSTR(pgm_sdkver)] = ESP.getSdkVersion();
  root[FPSTR(pgm_bootmode)] = ESP.getBootMode();
  root[FPSTR(pgm_bootversion)] = ESP.getBootVersion();
  root[FPSTR(pgm_resetreason)] = ESP.getResetReason();

  root.prettyPrintTo(file);
  file.flush();
  file.close();
  return true;
}

