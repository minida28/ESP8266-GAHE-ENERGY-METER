#include "FSWebServerLib.h"
#include <StreamString.h>
#include <Arduino.h>

#include <Arduino.h>
//#include "AsyncJson.h"
#include <ArduinoJson.h>
#include <SPIFFSEditor.h>
#include <DNSServer.h>
#include <StreamString.h>
#include <time.h>

#include "FSWebServerLib.h"
#include "gahe1progmem.h"

#include "timehelper.h"
// #include "sntphelper.h"
#include <pgmspace.h>

// #include "config.h"
#include "modbus.h"
// #include "mqtt.h"
#include "config.h"

#define RELEASE

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

// uint16_t num;
uint32_t clientID;

#define PARAMETER_XML_FILE "/parameter2.xml"

FSInfo fs_info;

DNSServer dnsServer;
AsyncWebSocket ws("/ws");
AsyncEventSource evs("/events");

bool DEBUGVIAWS = true;

uint32_t timestampReceivedFromWebGUI = 0;

bool configFileTimeUpdated = false;
bool sendFreeHeapStatusFlag = false;
bool sendDateTimeFlag = false;
bool setDateTimeFromGUIFlag = false;
bool wifiGotIpFlag = false;

// How to use the async client?
// https://github.com/me-no-dev/ESPAsyncTCP/issues/18

static AsyncClient *aClient = NULL;
static AsyncClient *bClient = NULL;

void runAsyncClientEmoncms()
{
  DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);
  if (aClient) //client already exists
  {
    // return;

    if (DEBUGVIAWS)
    {
      ws.textAll(PSTR("EmonCMS client already exists, closing the client...\r\n"));
    }

    aClient->close(true);
    AsyncClient *client = aClient;
    aClient = NULL;
    delete client;
  }

  aClient = new AsyncClient();
  if (!aClient) //could not allocate client
    return;

  aClient->onError([](void *arg, AsyncClient *client, err_t error) {
    DEBUGLOG("Connect Error\r\n");
    aClient = NULL;
    delete client;
  },
                   NULL);

  aClient->onConnect([](void *arg, AsyncClient *client) {
    DEBUGLOG("Connected\r\n");
    if (DEBUGVIAWS)
    {
      ws.textAll(PSTR("EmonCMS connected\r\n"));
    }
    aClient->onError(NULL, NULL);

    client->onDisconnect([](void *arg, AsyncClient *c) {
      DEBUGLOG("\nDisconnected\r\n");
      if (DEBUGVIAWS)
      {
        ws.textAll(PSTR("EmonCMS Disconnected\r\n"));
      }
      aClient = NULL;
      delete c;
    },
                         NULL);

    client->onData([](void *arg, AsyncClient *c, void *data, size_t len) {
      DEBUGLOG("\r\nData: ");
      DEBUGLOG("%d", len);
      // uint8_t *d = (uint8_t *)data;
      //  for (size_t i = 0; i < len; i++) {
      //    Serial.write(d[i]);
      //  }

      // if (DEBUGVIAWS)
      // {
      //   char receivedData[len + 1];

      //   uint8_t *d = (uint8_t *)data;
      //   for (size_t i = 0; i < len; i++)
      //   {
      //     receivedData[i] = (char)d[i];
      //   }
      //   receivedData[len] = '\0';

      //   char buf[32];
      //   sprintf_P(buf, PSTR("EmonCMS onData, len=%u\r\n"), len);
      //   ws.textAll(buf);
      //   ws.textAll("\r\n");
      //   ws.textAll(receivedData);
      //   ws.textAll("\r\n");
      // }

      char temp[len + 1];

      uint8_t *d = (uint8_t *)data;
      for (size_t i = 0; i < len; i++)
      {
        static int j = 0;
        char z = (char)d[i];
        temp[j] = z;
        j++;
        if (z == '\n')
        {
          temp[j + 1] = '\0';
          j = 0;
          char status[] = "HTTP/1.1 200 OK";
          int len = strlen(status);
          if (strncmp(temp, status, len) == 0)
          {
            if (DEBUGVIAWS)
              ws.textAll(status);

            aClient->close(true);

            // AsyncClient *client = aClient;
            // aClient = NULL;
            // delete c;

            break;
          }
        }
      }
    },
                   NULL);

    //construct HTTP request for EmonCMS
    File file = SPIFFS.open("/emoncms.json", "r");
    if (!file)
    {
      DEBUGLOG("Failed to open config file\r\n");
      return;
    }
    size_t size = file.size();
    char buf[size];
    file.readBytes(buf, size);
    buf[size] = '\0';
    file.close();

    // DynamicJsonDocument jsonBuffer(1024);
    StaticJsonDocument<1152> json;
    auto error = deserializeJson(json, buf);
    if (error)
    {
      DEBUGLOG("Failed to parse config file\r\n");
      return;
    }
    const char *templaterequest = json[FPSTR(pgm_templaterequest)];
    const char *method = json[FPSTR(pgm_method)];
    const char *url = json[FPSTR(pgm_url)];
    const char *node = json[FPSTR(pgm_node)];
    const char *apikey = json[FPSTR(pgm_apikey)];
    //const char* fulljson = json[FPSTR(pgm_fulljson)];
    const char *host = json[FPSTR(pgm_host)];

    StaticJsonDocument<512> jsonBuffer;
    jsonBuffer[FPSTR(pgm_voltage)] = bufVoltage;
    jsonBuffer[FPSTR(pgm_ampere)] = bufAmpere;
    jsonBuffer[FPSTR(pgm_watt)] = bufWatt;
    jsonBuffer[FPSTR(pgm_pstkwh)] = bufPstkwh;

    size_t len = measureJson(jsonBuffer);
    char bufFulljson[len + 1];
    serializeJson(jsonBuffer, bufFulljson, sizeof(bufFulljson));

    StreamString output;
    if (output.reserve(512))
    {
      output.printf(templaterequest,
                    method,
                    url,
                    node,
                    apikey,
                    bufFulljson,
                    host);
      DEBUGLOG("%s\r\n", output.c_str());
      client->write(output.c_str());
      if (DEBUGVIAWS)
      {
        ws.textAll(PSTR("EmonCMS send request\r\n"));
        ws.textAll(output.c_str());
      }
    }
  },
                     NULL);

  if (!aClient->connect("emoncms.org", 80))
  {
    DEBUGLOG("Connect Fail\r\n");
    if (DEBUGVIAWS)
    {
      ws.textAll(PSTR("EmonCMS Connect Fail\r\n"));
    }
    AsyncClient *client = aClient;
    aClient = NULL;
    delete client;
  }
}

void runAsyncClientThingspeak()
{
  DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);
  if (bClient) //client already exists
  {
    // return;

    if (DEBUGVIAWS)
    {
      ws.textAll(PSTR("Thingspeak client already exists, closing the client...\r\n"));
    }
    bClient->close(true);
    AsyncClient *client = bClient;
    bClient = NULL;
    delete client;
  }

  bClient = new AsyncClient();
  if (!bClient) //could not allocate client
  {
    if (DEBUGVIAWS)
    {
      ws.textAll(PSTR("Could not allocate client Thingspeak\r\n"));
    }
    return;
  }

  bClient->onError([](void *arg, AsyncClient *client, err_t error) {
    DEBUGLOG("Connect Error\r\n");
    if (DEBUGVIAWS)
    {
      ws.textAll(PSTR("Connect Thingspeak Error\r\n"));
    }
    bClient = NULL;
    delete client;
  },
                   NULL);

  bClient->onConnect([](void *arg, AsyncClient *client) {
    DEBUGLOG("Connected\r\n");
    if (DEBUGVIAWS)
    {
      ws.textAll(PSTR("Thingspeak connected\r\n"));
    }
    bClient->onError(NULL, NULL);

    client->onDisconnect([](void *arg, AsyncClient *c) {
      DEBUGLOG("\nDisconnected\r\n");
      if (DEBUGVIAWS)
      {
        ws.textAll(PSTR("Thingspeak Disconnected\r\n"));
      }
      bClient = NULL;
      delete c;
    },
                         NULL);

    client->onData([](void *arg, AsyncClient *c, void *data, size_t len) {
      DEBUGLOG("\r\nData: ");
      DEBUGLOG("%d", len);

      // uint8_t *d = (uint8_t *)data;
      //  for (size_t i = 0; i < len; i++) {
      //    Serial.write(d[i]);
      //  }

      // if (DEBUGVIAWS)
      // {
      //   char receivedData[len + 1];

      //   uint8_t *d = (uint8_t *)data;
      //   for (size_t i = 0; i < len; i++)
      //   {
      //     receivedData[i] = (char)d[i];
      //   }
      //   receivedData[len] = '\0';

      //   char buf[32];
      //   sprintf_P(buf, PSTR("Thingspeak onData, len=%u\r\n"), len);
      //   ws.textAll(buf);
      //   ws.textAll("\r\n");
      //   ws.textAll(receivedData);
      //   ws.textAll("\r\n");
      // }

      char temp[len + 1];

      uint8_t *d = (uint8_t *)data;
      for (size_t i = 0; i < len; i++)
      {
        static int j = 0;
        char z = (char)d[i];
        temp[j] = z;
        j++;
        if (z == '\n')
        {
          temp[j + 1] = '\0';
          j = 0;
          char status[] = "HTTP/1.1 200 OK";
          int len = strlen(status);
          if (strncmp(temp, status, len) == 0)
          {
            if (DEBUGVIAWS)
              ws.textAll(status);

            bClient->close(true);

            // AsyncClient *client = bClient;
            // bClient = NULL;
            // delete c;

            break;
          }
        }
      }
    },
                   NULL);

    //Construct HTTP request to THINGSPEAK
    //"POST /update?field4=40&status=\"Horeeee bisa lagi\" HTTP/1.1\r\nHost: api.thingspeak.com\r\nX-THINGSPEAKAPIKEY:XJP1DKEH9OGVBGSX\r\n\r\n");

    File file = SPIFFS.open(PSTR("/thingspeak.json"), "r");
    if (!file)
    {
      DEBUGLOG("Failed to open config file\r\n");
      return;
    }
    size_t size = file.size();
    char buf[size];
    file.readBytes(buf, size);
    buf[size] = '\0';
    file.close();

    StaticJsonDocument<512> json;
    auto error = deserializeJson(json, buf);
    if (error)
    {
      DEBUGLOG("Failed to parse config file\r\n");
      return;
    }

    const char *templaterequest = json[FPSTR(pgm_templaterequest)];
    const char *method = json[FPSTR(pgm_method)];
    const char *url = json[FPSTR(pgm_url)];
    const char *host = json[FPSTR(pgm_host)];
    const char *apikey = json[FPSTR(pgm_apikey)];

    uint32_t freeheap = ESP.getFreeHeap();
    char online[] = "ONLINE";
    //const char* online = FPSTR(pgm_online);

    StreamString output;
    if (output.reserve(512))
    {
      output.printf(templaterequest,
                    method,
                    url,
                    bufAmpere,
                    bufWatt,
                    bufVoltage,
                    bufPstkwh,
                    bufVar,
                    bufApparentPower,
                    bufPowerFactor,
                    freeheap,
                    online,
                    host,
                    apikey);
      DEBUGLOG("%s\r\n", output.c_str());
      client->write(output.c_str());
      if (DEBUGVIAWS)
      {
        ws.textAll(PSTR("Thingspeak send request\r\n"));
        ws.textAll(output.c_str());
      }
    }
  },
                     NULL);

  if (!bClient->connect(PSTR("api.thingspeak.com"), 80))
  {
    DEBUGLOG("Connect Fail\r\n");
    if (DEBUGVIAWS)
    {
      ws.textAll(PSTR("Thingspeak connect fail\r\n"));
    }
    AsyncClient *client = bClient;
    bClient = NULL;
    delete client;
  }
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_CONNECT)
  {
    //client connected
    //os_printf("ws[%s][%u] connect\n", server->url(), client->id());
    client->printf("Hello Client %u :)", client->id());
    client->ping();
    // num = client->id();
    clientID = client->id();
  }
  else if (type == WS_EVT_DISCONNECT)
  {
    //client disconnected
    //os_printf("ws[%s][%u] disconnect: %u\r\n", server->url(), client->id());
  }
  else if (type == WS_EVT_ERROR)
  {
    //error was received from the other end
    //os_printf("ws[%s][%u] error(%u): %s\r\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
  }
  else if (type == WS_EVT_PONG)
  {
    //pong message was received (in response to a ping request maybe)
    //os_printf("ws[%s][%u] pong[%u]: %s\r\n", server->url(), client->id(), len, (len)?(char*)data:"");
  }
  else if (type == WS_EVT_DATA)
  {
    //data packet
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len)
    {
      //the whole message is in a single frame and we got all of it's data
      //os_printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT)?"text":"binary", info->len);
      if (info->opcode == WS_TEXT)
      {
        data[len] = 0;
        os_printf("%s\r\n", (char *)data);
      }
      else
      {
        for (size_t i = 0; i < info->len; i++)
        {
          os_printf("%02x ", data[i]);
        }
        os_printf("\r\n");
      }
      if (info->opcode == WS_TEXT)
        client->text(FPSTR(pgm_got_text_message));
      else
        client->binary(FPSTR(pgm_got_binary_message));
    }
    else
    {
      //message is comprised of multiple frames or the frame is split into multiple packets
      if (info->index == 0)
      {
        if (info->num == 0)
        {
          //os_printf("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
          //os_printf("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
        }
      }

      os_printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT) ? "text" : "binary", info->index, info->index + len);
      if (info->message_opcode == WS_TEXT)
      {
        data[len] = 0;
        //os_printf("%s\r\n", (char*)data);
      }
      else
      {
        for (size_t i = 0; i < len; i++)
        {
          //os_printf("%02x ", data[i]);
        }
        //os_printf("\n");
      }

      if ((info->index + len) == info->len)
      {
        //os_printf("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
        if (info->final)
        {
          //os_printf("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
          if (info->message_opcode == WS_TEXT)
            client->text(FPSTR(pgm_got_text_message));
          else
            client->binary(FPSTR(pgm_got_binary_message));
        }
      }
    }

    // if (strncmp_P((char *)data, pgm_schedulepagesholat, strlen_P(pgm_schedulepagesholat)) == 0)
    // {
    //   // clientVisitSholatTimePage = true;
    // }
    if (strncmp_P((char *)data, pgm_settimepage, strlen_P(pgm_settimepage)) == 0)
    {
      sendDateTimeFlag = true;
    }
    else if (strncmp_P((char *)data, pgm_freeheap, strlen_P(pgm_freeheap)) == 0)
    {
      sendFreeHeapStatusFlag = true;
    }
    else if (strncmp((char *)data, "/status/datetime", strlen("/status/datetime")) == 0)
    {
      sendDateTimeFlag = true;
    }
    else if (data[0] == '{')
    {
      StaticJsonDocument<1024> root;
      DeserializationError error = deserializeJson(root, data);

      if (error)
      {
        return;
      }

      //******************************
      // handle SAVE CONFIG (not fast)
      //******************************

      if (root.containsKey(FPSTR(pgm_saveconfig)))
      {
        const char *saveconfig = root[FPSTR(pgm_saveconfig)];

        //remove json key before saving
        root.remove(FPSTR(pgm_saveconfig));

        if (false)
        {
        }
        //******************************
        // save TIME config
        //******************************
        else if (strcmp_P(saveconfig, pgm_configpagetime) == 0)
        {
          File file = SPIFFS.open(FPSTR(pgm_configfiletime), "w");

          if (!file)
          {
            DEBUGLOG("Failed to open TIME config file\r\n");
            file.close();
            return;
          }

          serializeJsonPretty(root, file);
          file.flush();
          file.close();

          configFileTimeUpdated = true;

          //beep
        }
      }
    }
    else if (data[0] == 't' && data[1] == ' ')
    {
      char *token = strtok((char *)&data[2], " ");

      timestampReceivedFromWebGUI = (unsigned long)strtol(token, '\0', 10);

      setDateTimeFromGUIFlag = true;
    }
  }
}

//char buf[100];
//void sendDataWs(AsyncWebSocketClient * client, char*  test)
//{
//    //size_t len = test.length();
//    AsyncWebSocketMessageBuffer * buffer = _ws.makeBuffer(100); //  creates a buffer (len + 1) for you.
//    if (buffer) {
//        //test.printTo((char *)buffer->get(), len + 1);
//        //sprintf ((char *)buffer->get(), "%s", test);
//        //test.toCharArray((char *)buffer->get(), len);
//        memcpy((char *)buffer->get(), test, 300);
//        if (client) {
//            client->text(buffer);
//        } else {
//            _ws.textAll(buffer);
//        }
//    }
//}

AsyncFSWebServer ESPHTTPServer(80);
//AsyncWebSocket.addHandler(&_ws);

AsyncFSWebServer::AsyncFSWebServer(uint16_t port) : AsyncWebServer(port) {}

String formatBytes(size_t bytes)
{
  if (bytes < 1024)
  {
    return String(bytes) + "B";
  }
  else if (bytes < (1024 * 1024))
  {
    return String(bytes / 1024.0) + "KB";
  }
  else if (bytes < (1024 * 1024 * 1024))
  {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  }
  else
  {
    return String(bytes / 1024.0 / 1024.0 / 1024.0) + "GB";
  }
}

void flashLED(int pin, int times, int delayTime)
{
  int oldState = digitalRead(pin);
  DEBUGLOG("---Flash LED during %d ms %d times. Old state = %d\r\n", delayTime, times, oldState);

  for (int i = 0; i < times; i++)
  {
    digitalWrite(pin, LOW); // Turn on LED
    delay(delayTime);
    digitalWrite(pin, HIGH); // Turn on LED
    delay(delayTime);
  }
  digitalWrite(pin, oldState); // Turn on LED
}

int pgm_lastIndexOf(uint8_t c, const char *p)
{
  int last_index = -1; // -1 indicates no match
  uint8_t b;
  for (int i = 0; true; i++)
  {
    b = pgm_read_byte(p++);
    if (b == c)
      last_index = i;
    else if (b == 0)
      break;
  }
  return last_index;
}

// displays at startup the Sketch running in the Arduino
void display_srcfile_details(void)
{
  const char *the_path = PSTR(__FILE__); // save RAM, use flash to hold __FILE__ instead

  int slash_loc = pgm_lastIndexOf('/', the_path); // index of last '/'
  if (slash_loc < 0)
    slash_loc = pgm_lastIndexOf('\\', the_path); // or last '\' (windows, ugh)

  int dot_loc = pgm_lastIndexOf('.', the_path); // index of last '.'
  if (dot_loc < 0)
    dot_loc = pgm_lastIndexOf(0, the_path); // if no dot, return end of string

  DEBUGLOG("\r\nSketch name: ");

  for (int i = slash_loc + 1; i < dot_loc; i++)
  {
    uint8_t b = pgm_read_byte(&the_path[i]);
    if (b != 0)
      DEBUGLOG("%c",(char)b);
    else
      break;
  }
  DEBUGLOG("\r\n");

  DEBUGLOG("Compiled on: ");
  DEBUGLOG(__DATE__);
  DEBUGLOG(" at ");
  DEBUGLOG(__TIME__);
  DEBUGLOG("\r\n\r\n");
}

bool save_system_info()
{
  DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);

  // const char* pathtofile = PSTR(pgm_filesystemoverview);

  File file;
  if (!SPIFFS.exists(FPSTR(pgm_systeminfofile)))
  {
    file = SPIFFS.open(FPSTR(pgm_systeminfofile), "w");
    if (!file)
    {
      DEBUGLOG("Failed to open config file for writing\r\n");
      file.close();
      return false;
    }
    //create blank json file
    DEBUGLOG("Creating user config file for writing\r\n");
    file.print("{}");
    file.close();
  }
  //get existing json file
  file = SPIFFS.open(FPSTR(pgm_systeminfofile), "w");
  if (!file)
  {
    DEBUGLOG("Failed to open config file");
    return false;
  }

  const char *the_path = PSTR(__FILE__);
  // const char* _compiletime = PSTR(__TIME__);

  int slash_loc = pgm_lastIndexOf('/', the_path); // index of last '/'
  if (slash_loc < 0)
    slash_loc = pgm_lastIndexOf('\\', the_path); // or last '\' (windows, ugh)

  int dot_loc = pgm_lastIndexOf('.', the_path); // index of last '.'
  if (dot_loc < 0)
    dot_loc = pgm_lastIndexOf(0, the_path); // if no dot, return end of string

  int lenPath = strlen(the_path);
  int lenFileName = (lenPath - (slash_loc + 1));

  char fileName[lenFileName + 1];
  //Serial.println(lenFileName);
  //Serial.println(sizeof(fileName));

  int j = 0;
  for (int i = slash_loc + 1; i < lenPath; i++)
  {
    uint8_t b = pgm_read_byte(&the_path[i]);
    if (b != 0)
    {
      fileName[j] = (char)b;
      //Serial.print(fileName[j]);
      j++;
      if (j >= lenFileName)
      {
        break;
      }
    }
    else
    {
      break;
    }
  }
  //Serial.println();
  //Serial.println(j);
  fileName[lenFileName] = '\0';

  //const char* _compiledate = PSTR(__DATE__);
  int lenCompileDate = strlen_P(PSTR(__DATE__));
  char compileDate[lenCompileDate + 1];
  strcpy_P(compileDate, PSTR(__DATE__));

  int lenCompileTime = strlen_P(PSTR(__TIME__));
  char compileTime[lenCompileTime + 1];
  strcpy_P(compileTime, PSTR(__TIME__));

  StaticJsonDocument<1024> root;

  SPIFFS.info(fs_info);

  root[FPSTR(pgm_totalbytes)] = fs_info.totalBytes;
  root[FPSTR(pgm_usedbytes)] = fs_info.usedBytes;
  root[FPSTR(pgm_blocksize)] = fs_info.blockSize;
  root[FPSTR(pgm_pagesize)] = fs_info.pageSize;
  root[FPSTR(pgm_maxopenfiles)] = fs_info.maxOpenFiles;
  root[FPSTR(pgm_maxpathlength)] = fs_info.maxPathLength;

  root[FPSTR(pgm_filename)] = fileName;
  root[FPSTR(pgm_compiledate)] = compileDate;
  root[FPSTR(pgm_compiletime)] = compileTime;
  // root[FPSTR(pgm_lastboot)] = getLastBootStr();
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
  root[FPSTR(pgm_corever)] = ESP.getFullVersion();
  root[FPSTR(pgm_sdkver)] = ESP.getSdkVersion();
  root[FPSTR(pgm_bootmode)] = ESP.getBootMode();
  root[FPSTR(pgm_bootversion)] = ESP.getBootVersion();
  root[FPSTR(pgm_resetreason)] = ESP.getResetReason();

  serializeJsonPretty(root, file);
  file.flush();
  file.close();
  return true;
}

void AsyncFSWebServer::start(FS *fs)
{
  // -------------------------------------------------------------------
  // Mount File system
  // -------------------------------------------------------------------
  DEBUGLOG("Mounting FS...\r\n");

  if (!SPIFFS.begin())
  // if (false)
  {
    DEBUGLOG("Failed to mount file system\r\n");
    return;
  }
  else
  {
    // ftpSrv.begin("esp8266","esp8266");
  }

  _fs = fs;

  if (!_fs) // If SPIFFS is not started
    _fs->begin();
#ifndef RELEASE
  { // List files
    Dir dir = _fs->openDir("/");
    while (dir.next())
    {
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();

      DEBUGLOG("FS File: %s, size: %s\r\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    DEBUGLOG("\n");
  }
#endif // RELEASE

  display_srcfile_details();

  save_system_info();

  // NTP client setup
  if (CONNECTION_LED >= 0)
  {
    pinMode(CONNECTION_LED, OUTPUT); // CONNECTION_LED pin defined as output
  }
  if (AP_ENABLE_BUTTON >= 0)
  {
    //If this pin is HIGH during startup ESP will run in AP_ONLY mode.
    //Backdoor to change WiFi settings when configured WiFi is not available.
    if (analogRead(A0) >= 750)
    {
      _apConfig.APenable = true;
      DEBUGLOG("AP Enable = %d\n", _apConfig.APenable);
    }
  }

  if (CONNECTION_LED >= 0)
  {
    digitalWrite(CONNECTION_LED, HIGH); // Turn LED off
  }

  //Set the host name
  char bufPrefix[] = "ENERGYMETER_";
  char bufChipId[11];
  itoa(ESP.getChipId(), bufChipId, 10);

  //char bufHostName[32];
  strlcpy(_config.hostname, bufPrefix, sizeof(_config.hostname) / sizeof(_config.hostname[0]));
  strncat(_config.hostname, bufChipId, sizeof(bufChipId) / sizeof(bufChipId[0]));

  if (!load_config_network())
  {
    save_config_network();
    // _configAP.APenable = true;
    _apConfig.APenable = true;
  }
  if (!load_config_time())
  {
    save_config_time();
    _apConfig.APenable = true;
  }

  loadHTTPAuth();

  //WIFI INIT
  if (_configTime.syncinterval > 0)
  { // Enable NTP sync
    // NTP.begin(_configTime.ntpserver_0, TimezoneFloat(), _configTime.dst);
    // NTP.setInterval(15, _configTime.syncinterval * 60);
  }
  // Register wifi Event to control connection LED
  onStationModeConnectedHandler = WiFi.onStationModeConnected([this](WiFiEventStationModeConnected data) {
    this->onWiFiConnected(data);
  });
  onStationModeDisconnectedHandler = WiFi.onStationModeDisconnected([this](WiFiEventStationModeDisconnected data) {
    this->onWiFiDisconnected(data);
  });
  onStationModeGotIPHandler = WiFi.onStationModeGotIP([this](WiFiEventStationModeGotIP data) {
    this->onWifiGotIP(data);
  });

  DEBUGLOG("WiFi.getListenInterval(): %d\r\n", WiFi.getListenInterval());
  DEBUGLOG("WiFi.isSleepLevelMax(): %d\r\n", WiFi.isSleepLevelMax());

  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  // WiFi.setSleepMode(WIFI_LIGHT_SLEEP);
  // WiFi.setSleepMode(WIFI_MODEM_SLEEP);

  DEBUGLOG("WiFi.getListenInterval(): %d\r\n", WiFi.getListenInterval());
  DEBUGLOG("WiFi.isSleepLevelMax(): %d\r\n", WiFi.isSleepLevelMax());

  WiFi.hostname(_config.hostname);

  // WiFi.mode(WIFI_OFF); //=========== TESTING, ORIGINAL-NYA WIFI DIMATIKAN DULU

  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);

  //WiFi.mode(WIFI_OFF);
  if (_apConfig.APenable)
  {
    WiFi.mode(WIFI_OFF);
    // configureWifiAP(); // Set AP mode if AP button was pressed
    //WiFi.mode(WIFI_AP);
    WiFi.softAP(_config.hostname, NULL);
    //WiFi.softAP("GAHE1", NULL);
    WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
  }
  else
  {
    DEBUGLOG("Starting wifi in WIFI_STA mode.\r\n");

    WiFi.mode(WIFI_STA);

    if (WiFi.getAutoReconnect())
    {
      if (WiFi.waitForConnectResult(10000) == -1) // hit timeout
      {
        DEBUGLOG("Wifi connect timeout. Re-starting connection...\r\n");
        WiFi.mode(WIFI_OFF);
        WiFi.hostname(_config.hostname);
        WiFi.begin(_config.ssid, _config.password);
        WiFi.waitForConnectResult();
      }
    }
  }

  DEBUGLOG("Open http://");
  DEBUGLOG("%s\r\n", _config.hostname);
  DEBUGLOG(".local/edit to see the file browser\r\n");
  DEBUGLOG("Flash chip size: %u\r\n", ESP.getFlashChipRealSize());
  DEBUGLOG("Scketch size: %u\r\n", ESP.getSketchSize());
  DEBUGLOG("Free flash space: %u\r\n", ESP.getFreeSketchSpace());

  // _secondTk.attach(1.0f, &AsyncFSWebServer::s_secondTick, static_cast<void *>(this)); // Task to run periodic things every second

  ConfigureOTA(_httpAuth.wwwPassword.c_str());

  // dnsServer.start(53, "*", WiFi.softAPIP());

  DEBUGLOG("Starting mDNS responder...\r\n");
  // if (!MDNS.begin(_config.hostname))
  if (!MDNS.begin(_config.hostname))
  { // Start the mDNS responder for esp8266.local
    DEBUGLOG("Error setting up mDNS responder!\r\n");
  }
  else
  {
    DEBUGLOG("mDNS responder started\r\n");
    // MDNS.addService("http", "tcp", 80);
  }

  ArduinoOTA.setHostname(_config.hostname);
  ArduinoOTA.begin();

  ESPHTTPServer.serverInit(); // Configure and start Web server

  // AsyncWebServer::begin();
  // ESPHTTPServer.begin(fs);

  DEBUGLOG("END Setup\n");
}

bool AsyncFSWebServer::load_config()
{
  DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);
  return true;
}

void AsyncFSWebServer::defaultConfig()
{
  DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);
}

bool AsyncFSWebServer::save_config()
{
  DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);
  return true;
}

bool AsyncFSWebServer::loadHTTPAuth()
{
  File configFile = _fs->open(SECRET_FILE, "r");
  if (!configFile)
  {
    DEBUGLOG("Failed to open secret file\r\n");
    _httpAuth.auth = false;
    _httpAuth.wwwUsername = "";
    _httpAuth.wwwPassword = "";
    configFile.close();
    return false;
  }

  size_t size = configFile.size();
  DEBUGLOG("JSON secret file size: %d bytes\r\n", size);
  if (size > 256)
  {
    DEBUGLOG("Secret file size is too large\r\n");
    _httpAuth.auth = false;
    _httpAuth.wwwUsername = "";
    _httpAuth.wwwPassword = "";
    configFile.close();
    return false;
  }

  StaticJsonDocument<256> json;
  auto error = deserializeJson(json, configFile);
  configFile.close();

  if (error)
  {
#ifndef RELEASE
    String temp;
    // json.prettyPrintTo(temp);
    serializeJsonPretty(json, temp);
    PRINT("%s\r\n", temp.c_str());
    PRINT("Failed to parse secret file\r\n");
#endif // RELEASE
    _httpAuth.auth = false;
    _httpAuth.wwwUsername = "";
    _httpAuth.wwwPassword = "";
    return false;
  }
#ifndef RELEASE
  // json.prettyPrintTo(DEBUGPORT);
  serializeJsonPretty(json, DEBUGPORT);
  PRINT("\r\n");
#endif // RELEASE

  _httpAuth.auth = json[FPSTR(pgm_auth)];
  _httpAuth.wwwUsername = json[FPSTR(pgm_user)].as<char *>();
  _httpAuth.wwwPassword = json[FPSTR(pgm_pass)].as<char *>();

  if (_httpAuth.auth)
  {
    DEBUGLOG("Secret initialized.\r\n");
  }
  else
  {
    DEBUGLOG("Auth disabled.\r\n");
  }
  if (_httpAuth.auth)
  {
    DEBUGLOG("User: %s\r\n", _httpAuth.wwwUsername.c_str());
    DEBUGLOG("Pass: %s\r\n", _httpAuth.wwwPassword.c_str());
  }
  DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);

  return true;
}

void AsyncFSWebServer::loop()
{
  ArduinoOTA.handle();
  ws.cleanupClients();
  dnsServer.processNextRequest();
  MDNS.update();
}

void AsyncFSWebServer::configureWifiAP()
{
  DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);
  //WiFi.disconnect();
  WiFi.mode(WIFI_AP);
  String APname = _apConfig.APssid + (String)ESP.getChipId();
  if (_httpAuth.auth)
  {
    WiFi.softAP(APname.c_str(), _httpAuth.wwwPassword.c_str());
    DEBUGLOG("AP Pass enabled: %s\r\n", _httpAuth.wwwPassword.c_str());
  }
  else
  {
    WiFi.softAP(APname.c_str());
    DEBUGLOG("AP Pass disabled\r\n");
  }
  if (CONNECTION_LED >= 0)
  {
    flashLED(CONNECTION_LED, 3, 250);
  }
}

void AsyncFSWebServer::configureWifi()
{
}

void AsyncFSWebServer::ConfigureOTA(String password)
{
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(_config.hostname);

  // No authentication by default
  if (password != "")
  {
    ArduinoOTA.setPassword(password.c_str());
    DEBUGLOG("OTA password set %s\r\n", password.c_str());
  }

#ifndef RELEASE
  ArduinoOTA.onStart([]() {
    DEBUGLOG("StartOTA\r\n");
  });
  ArduinoOTA.onEnd(std::bind([](FS *fs) {
    fs->end();
    DEBUGLOG("\r\nEnd OTA\r\n");
  },
                             _fs));
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    DEBUGLOG("OTA Progress: %u%%\r\n", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    DEBUGLOG("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)
    {
      DEBUGLOG("Auth Failed\r\n");
    }
    else if (error == OTA_BEGIN_ERROR)
    {
      DEBUGLOG("Begin Failed\r\n");
    }
    else if (error == OTA_CONNECT_ERROR)
    {
      DEBUGLOG("Connect Failed\r\n");
    }
    else if (error == OTA_RECEIVE_ERROR)
    {
      DEBUGLOG("Receive Failed\r\n");
    }
    else if (error == OTA_END_ERROR)
    {
      DEBUGLOG("End Failed\r\n");
    }
  });
  DEBUGLOG("\r\nOTA Ready\r\n");
#endif // RELEASE
  ArduinoOTA.begin();
}

void AsyncFSWebServer::onWiFiConnected(WiFiEventStationModeConnected data)
{
  if (CONNECTION_LED >= 0)
  {
    digitalWrite(CONNECTION_LED, LOW); // Turn LED on
  }
  DEBUGLOG("Led %d on\n", CONNECTION_LED);
  //turnLedOn();
  wifiDisconnectedSince = 0;
}

void AsyncFSWebServer::onWiFiDisconnected(WiFiEventStationModeDisconnected data)
{
  DEBUGLOG("case STA_DISCONNECTED");

  if (CONNECTION_LED >= 0)
  {
    digitalWrite(CONNECTION_LED, HIGH); // Turn LED off
  }
  //DBG_OUTPUT_PORT.printf("Led %s off\r\n", CONNECTION_LED);
  //flashLED(config.connectionLed, 2, 100);
  if (wifiDisconnectedSince == 0)
  {
    wifiDisconnectedSince = millis();
  }
  DEBUGLOG("\r\nDisconnected for %d seconds\r\n", (int)((millis() - wifiDisconnectedSince) / 1000));
}

void AsyncFSWebServer::onWifiGotIP(WiFiEventStationModeGotIP data)
{
  wifiGotIpFlag = true;
  WiFi.setAutoReconnect(true);

  //Serial.printf_P(PSTR("\r\nWifi Got IP: %s\r\n"), data.ip.toString().c_str ());
  DEBUGLOG("\r\nWifi Got IP\r\n");
  // DEBUGLOG("IP Address:\t%s\r\n", WiFi.localIP().toString().c_str());
  DEBUGLOG("IP Address:\t%s\r\n", data.ip.toString().c_str());
}

void AsyncFSWebServer::handleFileList(AsyncWebServerRequest *request)
{
  if (!request->hasArg("dir"))
  {
    request->send(500, "text/plain", "BAD ARGS");
    return;
  }

  String path = request->arg("dir");
  DEBUGLOG("handleFileList: %s\r\n", path.c_str());
  Dir dir = _fs->openDir(path);
  path = String();

  String output = "[";
  while (dir.next())
  {
    File entry = dir.openFile("r");
    if (true) //entry.name()!="secret.json") // Do not show secrets
    {
      if (output != "[")
        output += ',';
      bool isDir = false;
      output += "{\"type\":\"";
      output += (isDir) ? "dir" : "file";
      output += "\",\"name\":\"";
      output += String(entry.name()).substring(1);
      output += "\"}";
    }
    entry.close();
  }

  output += "]";
  DEBUGLOG("%s\r\n", output.c_str());
  request->send(200, "text/json", output);
}

void AsyncFSWebServer::send_general_configuration_values_html(AsyncWebServerRequest *request)
{
  String values = "";
  values += "devicename|" + (String)_config.hostname + "|input\n";
  request->send(200, "text/plain", values);
  DEBUGLOG("%s\r\n", __FUNCTION__);
}

void AsyncFSWebServer::send_network_configuration_values_html(AsyncWebServerRequest *request)
{
  DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);

  File configFile = _fs->open(CONFIG_FILE, "r");
  if (!configFile)
  {
    DEBUGLOG("Failed to open config file");
    return;
  }

  size_t size = configFile.size();
  DEBUGLOG("JSON file size: %d bytes\r\n", size);
  char buf[size];
  configFile.readBytes(buf, size);
  buf[size] = '\0';
  configFile.close();

  request->send(200, "text/plain", buf);
}

//*************************
// LOAD CONFIG NETWORK
//*************************
bool AsyncFSWebServer::load_config_network()
{
  DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);

  File file = _fs->open(FPSTR(pgm_configfilenetwork), "r");
  if (!file)
  {
    DEBUGLOG("Failed to open config file\n");
    file.close();
    return false;
  }

  size_t size = file.size();
  DEBUGLOG("JSON file size: %d bytes\r\n", size);

  // Allocate a buffer to store contents of the file
  char buf[size];

  //copy file to buffer
  file.readBytes(buf, size);

  //add termination character at the end
  buf[size] = '\0';

  //close the file, save your memory, keep healthy :-)
  file.close();

  StaticJsonDocument<512> root;
  auto error = deserializeJson(root, buf);

  if (error)
  {
    DEBUGLOG("Failed to parse config NETWORK file\r\n");
    return false;
  }

#ifndef RELEASE
  // root.prettyPrintTo(Serial);
  serializeJsonPretty(root, Serial);
#endif

  // strlcpy(_config.hostname, root[FPSTR(pgm_hostname)], sizeof(_config.hostname)/sizeof(_config.hostname[0]));
  strlcpy(_config.ssid, root[FPSTR(pgm_ssid)], sizeof(_config.ssid) / sizeof(_config.ssid[0]));
  strlcpy(_config.password, root[FPSTR(pgm_password)], sizeof(_config.password) / sizeof(_config.password[0]));
  _config.dhcp = root[FPSTR(pgm_dhcp)];
  strlcpy(_config.static_ip, root[FPSTR(pgm_static_ip)], sizeof(_config.static_ip) / sizeof(_config.static_ip[0]));
  strlcpy(_config.netmask, root[FPSTR(pgm_netmask)], sizeof(_config.netmask) / sizeof(_config.netmask[0]));
  strlcpy(_config.gateway, root[FPSTR(pgm_gateway)], sizeof(_config.gateway) / sizeof(_config.gateway[0]));
  strlcpy(_config.dns0, root[FPSTR(pgm_dns0)], sizeof(_config.dns0) / sizeof(_config.dns0[0]));
  strlcpy(_config.dns1, root[FPSTR(pgm_dns1)], sizeof(_config.dns1) / sizeof(_config.dns1[0]));

  return true;
}

//*************************
// LOAD CONFIG TIME
//*************************
bool AsyncFSWebServer::load_config_time()
{
  DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);

  File file = _fs->open(FPSTR(pgm_configfiletime), "r");
  if (!file)
  {
    DEBUGLOG("Failed to open config file\n");
    file.close();
    return false;
  }

  size_t size = file.size();
  DEBUGLOG("JSON file size: %d bytes\r\n", size);

  // Allocate a buffer to store contents of the file
  char buf[size];

  //copy file to buffer
  file.readBytes(buf, size);

  //add termination character at the end
  buf[size] = '\0';

  //close the file, save your memory, keep healthy :-)
  file.close();

  StaticJsonDocument<1024> root;
  auto error = deserializeJson(root, buf);

  if (error)
  {
    DEBUGLOG("Failed to parse config NETWORK file\r\n");
    return false;
  }

#ifndef RELEASE
  // root.prettyPrintTo(DEBUGPORT);
  serializeJsonPretty(root, DEBUGPORT);
#endif

  // _configTime.timezone = root[FPSTR(pgm_timezone)];
  _configTime.dst = root[FPSTR(pgm_dst)];
  _configTime.enablertc = root[FPSTR(pgm_enablertc)];
  _configTime.syncinterval = root[FPSTR(pgm_syncinterval)];
  _configTime.enablentp = root[FPSTR(pgm_enablentp)];
  strlcpy(_configTime.ntpserver_0, root[FPSTR(pgm_ntpserver_0)], sizeof(_configTime.ntpserver_0) / sizeof(_configTime.ntpserver_0[0]));
  strlcpy(_configTime.ntpserver_1, root[FPSTR(pgm_ntpserver_1)], sizeof(_configTime.ntpserver_1) / sizeof(_configTime.ntpserver_1[0]));
  strlcpy(_configTime.ntpserver_2, root[FPSTR(pgm_ntpserver_2)], sizeof(_configTime.ntpserver_2) / sizeof(_configTime.ntpserver_2[0]));

  return true;
}

//*************************
// SAVE NETWORK CONFIG
//*************************
bool AsyncFSWebServer::save_config_network()
{
  DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);

  DynamicJsonDocument json(1024);

  // json[FPSTR(pgm_hostname)] = _config.hostname;
  json[FPSTR(pgm_ssid)] = _config.ssid;
  json[FPSTR(pgm_password)] = _config.password;
  json[FPSTR(pgm_dhcp)] = _config.dhcp;
  json[FPSTR(pgm_static_ip)] = _config.static_ip;
  json[FPSTR(pgm_netmask)] = _config.netmask;
  json[FPSTR(pgm_gateway)] = _config.gateway;
  json[FPSTR(pgm_dns0)] = _config.dns0;
  json[FPSTR(pgm_dns1)] = _config.dns1;

  File file = _fs->open(FPSTR(pgm_configfilenetwork), "w");

  // if (!file)
  // {
  //   DEBUGLOG("Failed to open config file");
  //   return false;
  // }

#ifndef RELEASE
  serializeJsonPretty(json, DEBUGPORT);
#endif

  serializeJsonPretty(json, file);
  file.flush();
  file.close();
  return true;
}

//*************************
// SAVE CONFIG TIME
//*************************
bool AsyncFSWebServer::save_config_time()
{
  DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);

  DynamicJsonDocument json(1024);

  json[FPSTR(pgm_timezone)] = TimezoneFloat();
  json[FPSTR(pgm_dst)] = _configTime.dst;
  json[FPSTR(pgm_enablertc)] = _configTime.enablertc;
  json[FPSTR(pgm_syncinterval)] = _configTime.syncinterval;
  json[FPSTR(pgm_enablentp)] = _configTime.enablentp;
  json[FPSTR(pgm_ntpserver_0)] = _configTime.ntpserver_0;
  json[FPSTR(pgm_ntpserver_1)] = _configTime.ntpserver_1;
  json[FPSTR(pgm_ntpserver_2)] = _configTime.ntpserver_2;

  File file = _fs->open(FPSTR(pgm_configfiletime), "w");

  if (!file)
  {
    DEBUGLOG("Failed to open config file");
    return false;
  }

#ifndef RELEASE
  serializeJsonPretty(json, DEBUGPORT);
#endif

  serializeJsonPretty(json, file);
  file.flush();
  file.close();
  return true;
}

void AsyncFSWebServer::send_config_network(AsyncWebServerRequest *request)
{
  DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);

  AsyncResponseStream *response = request->beginResponseStream("application/json");
  DynamicJsonDocument root(1024);

  root[FPSTR(pgm_hostname)] = _config.hostname;
  root[FPSTR(pgm_ssid)] = _config.ssid;
  root[FPSTR(pgm_password)] = _config.password;
  root[FPSTR(pgm_dhcp)] = _config.dhcp;
  root[FPSTR(pgm_static_ip)] = _config.static_ip;
  root[FPSTR(pgm_netmask)] = _config.netmask;
  root[FPSTR(pgm_gateway)] = _config.gateway;
  root[FPSTR(pgm_dns0)] = _config.dns0;
  root[FPSTR(pgm_dns1)] = _config.dns1;

  serializeJsonPretty(root, *response);
  request->send(response);
}

void AsyncFSWebServer::send_connection_state_values_html(AsyncWebServerRequest *request)
{
  DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);
  String state = "N/A";
  String Networks = "";
  if (WiFi.status() == 0)
    state = "Idle";
  else if (WiFi.status() == 1)
    state = "NO SSID AVAILBLE";
  else if (WiFi.status() == 2)
    state = "SCAN COMPLETED";
  else if (WiFi.status() == 3)
    state = "CONNECTED";
  else if (WiFi.status() == 4)
    state = "CONNECT FAILED";
  else if (WiFi.status() == 5)
    state = "CONNECTION LOST";
  else if (WiFi.status() == 6)
    state = "DISCONNECTED";

  WiFi.scanNetworks(true);

  String values = "";
  values += "connectionstate|" + state + "|div\n";
  //values += "networks|Scanning networks ...|div\n";
  request->send(200, "text/plain", values);
  state = "";
  values = "";
  Networks = "";
}

void AsyncFSWebServer::send_information_values_html(AsyncWebServerRequest *request)
{
  DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);

  AsyncResponseStream *response = request->beginResponseStream("application/json");
  DynamicJsonDocument root(1024);

  root[FPSTR(pgm_chipid)] = ESP.getChipId();
  root[FPSTR(pgm_hostname)] = _config.hostname;
  uint8_t mode = WiFi.getMode();
  if (mode == WIFI_AP)
  {
    root[FPSTR(pgm_mode)] = FPSTR(pgm_WIFI_AP);
  }
  else if (mode == WIFI_STA)
  {
    root[FPSTR(pgm_mode)] = FPSTR(pgm_WIFI_STA);
  }
  else if (mode == WIFI_AP_STA)
  {
    root[FPSTR(pgm_mode)] = FPSTR(pgm_WIFI_AP_STA);
  }
  else if (mode == WIFI_OFF)
  {
    root[FPSTR(pgm_mode)] = FPSTR(pgm_WIFI_OFF);
  }
  else
  {
    root[FPSTR(pgm_mode)] = FPSTR(pgm_NA);
  }
  root[FPSTR(pgm_ssid)] = WiFi.SSID();
  root[FPSTR(pgm_sta_ip)] = WiFi.localIP().toString();
  root[FPSTR(pgm_sta_mac)] = WiFi.macAddress();
  root[FPSTR(pgm_ap_ip)] = WiFi.softAPIP().toString();
  root[FPSTR(pgm_ap_mac)] = WiFi.softAPmacAddress();
  root[FPSTR(pgm_gateway)] = WiFi.gatewayIP().toString();
  root[FPSTR(pgm_netmask)] = WiFi.subnetMask().toString();
  root[FPSTR(pgm_dns)] = WiFi.dnsIP().toString();
  root[FPSTR(pgm_lastsync)] = getLastSyncStr();
  root[FPSTR(pgm_time)] = getTimeStr();
  root[FPSTR(pgm_date)] = getDateStr();
  root[FPSTR(pgm_uptime)] = getUptimeStr();
  root[FPSTR(pgm_lastboot)] = getLastBootStr();

  serializeJson(root, *response);

  request->send(response);
}

String AsyncFSWebServer::getMacAddress()
{
  uint8_t mac[6];
  char macStr[18] = {0};
  WiFi.macAddress(mac);
  sprintf_P(macStr, PSTR("%02X:%02X:%02X:%02X:%02X:%02X"), mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(macStr);
}

void AsyncFSWebServer::send_NTP_configuration_values_html(AsyncWebServerRequest *request)
{
  DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);

  AsyncResponseStream *response = request->beginResponseStream("text/json");
  DynamicJsonDocument root(1024);

  root[FPSTR(pgm_timezone)] = TimezoneFloat();
  root[FPSTR(pgm_dst)] = _configTime.dst;
  root[FPSTR(pgm_enablertc)] = _configTime.enablertc;
  root[FPSTR(pgm_syncinterval)] = _configTime.syncinterval;
  root[FPSTR(pgm_enablentp)] = _configTime.enablentp;
  root[FPSTR(pgm_ntpserver_0)] = _configTime.ntpserver_0;
  root[FPSTR(pgm_ntpserver_1)] = _configTime.ntpserver_1;
  root[FPSTR(pgm_ntpserver_2)] = _configTime.ntpserver_2;

  serializeJson(root, *response);
  request->send(response);
}

// convert a single hex digit character to its integer value (from https://code.google.com/p/avr-netino/)
unsigned char AsyncFSWebServer::h2int(char c)
{
  if (c >= '0' && c <= '9')
  {
    return ((unsigned char)c - '0');
  }
  if (c >= 'a' && c <= 'f')
  {
    return ((unsigned char)c - 'a' + 10);
  }
  if (c >= 'A' && c <= 'F')
  {
    return ((unsigned char)c - 'A' + 10);
  }
  return (0);
}

String AsyncFSWebServer::urldecode(String input) // (based on https://code.google.com/p/avr-netino/)
{
  char c;
  String ret = "";

  for (byte t = 0; t < input.length(); t++)
  {
    c = input[t];
    if (c == '+')
      c = ' ';
    if (c == '%')
    {

      t++;
      c = input[t];
      t++;
      c = (h2int(c) << 4) | h2int(input[t]);
    }

    ret.concat(c);
  }
  return ret;
}

//
// Check the Values is between 0-255
//
boolean AsyncFSWebServer::checkRange(String Value)
{
  if (Value.toInt() < 0 || Value.toInt() > 255)
  {
    return false;
  }
  else
  {
    return true;
  }
}

void AsyncFSWebServer::send_network_configuration_html(AsyncWebServerRequest *request)
{
  DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);

  //List all parameters
  int params = request->params();
  if (params)
  {
    _config.dhcp = false;

    for (int i = 0; i < params; i++)
    {
      AsyncWebParameter *p = request->getParam(i);

      DEBUGLOG("POST[%s]: %s\r\n", p->name().c_str(), p->value().c_str());

      const char *name = p->name().c_str();
      uint8_t len = strlen(name);

      if (strncmp_P(name, pgm_ssid, len) == 0)
      {
        strlcpy(_config.ssid, p->value().c_str(), sizeof(_config.ssid) / sizeof(_config.ssid[0]));
      }
      if (strncmp_P(name, pgm_password, len) == 0)
      {
        strlcpy(_config.password, p->value().c_str(), sizeof(_config.password) / sizeof(_config.password[0]));
      }
      if (strncmp_P(name, pgm_dhcp, len) == 0)
      {
        _config.dhcp = p->value();
      }
      if (strncmp_P(name, pgm_static_ip, len) == 0)
      {
        strlcpy(_config.static_ip, p->value().c_str(), sizeof(_config.static_ip) / sizeof(_config.static_ip[0]));
      }
      if (strncmp_P(name, pgm_netmask, len) == 0)
      {
        strlcpy(_config.netmask, p->value().c_str(), sizeof(_config.netmask) / sizeof(_config.netmask[0]));
      }
      if (strncmp_P(name, pgm_gateway, len) == 0)
      {
        strlcpy(_config.gateway, p->value().c_str(), sizeof(_config.gateway) / sizeof(_config.gateway[0]));
      }
      if (strncmp_P(name, pgm_dns0, len) == 0)
      {
        strlcpy(_config.dns0, p->value().c_str(), sizeof(_config.dns0) / sizeof(_config.dns0[0]));
      }
      if (strncmp_P(name, pgm_dns1, len) == 0)
      {
        strlcpy(_config.dns1, p->value().c_str(), sizeof(_config.dns1) / sizeof(_config.dns1[0]));
      }
    }
    //save settings
    save_config_network();
    request->send(200, PSTR("text/plain"), PSTR("OK"));
    return;
  }
  request->send(SPIFFS, request->url());
}

void AsyncFSWebServer::send_classic_xml_page(AsyncWebServerRequest *request)
{
  uint32_t heap = ESP.getFreeHeap();

  File paramXmlFile = SPIFFS.open(PARAMETER_XML_FILE, "r");
  if (!paramXmlFile)
  {
    DEBUGLOG("Failed to open config file\r\n");
    return;
  }

  size_t size = paramXmlFile.size();
  DEBUGLOG("PARAMETER_XML_FILE file size: %d bytes\r\n", size);
  if (size > 1024)
  {
    DEBUGLOG("PARAMETER_XML_FILE file size is too large\r\n");
    return;
  }

  // Allocate a buffer to store contents of the file
  char buf[size];

  //copy file to buffer
  paramXmlFile.readBytes(buf, size);

  //    for (int i = 0; i < size; i++) {
  //      //DEBUGMQTT("%c", (char)payload[i]);
  //      buf[i] = &paramXmlFile[i];
  //    }

  //add termination character at the end
  buf[size] = '\0';

  //close the file, save your memory, keep healthy :-)
  paramXmlFile.close();

  //PRINT("%s\r\n", buf);

  StreamString output;

  if (output.reserve(1024))
  {
    //convert IP address to char array
    size_t len = strlen(WiFi.localIP().toString().c_str());
    char ipAddress[len + 1];
    strlcpy(ipAddress, WiFi.localIP().toString().c_str(), sizeof(ipAddress) / sizeof(ipAddress[0]));

    char bufRequestsPACKET3[10];
    char bufSuccessful_requestsPACKET3[10];
    char bufFailed_requestsPACKET3[10];
    char bufException_errorsPACKET3[10];
    char bufConnectionPACKET3[10];

    dtostrf(packets[PACKET3].requests, 0, 0, bufRequestsPACKET3);
    dtostrf(packets[PACKET3].successful_requests, 0, 0, bufSuccessful_requestsPACKET3);
    dtostrf(packets[PACKET3].failed_requests, 0, 0, bufFailed_requestsPACKET3);
    dtostrf(packets[PACKET3].exception_errors, 0, 0, bufException_errorsPACKET3);
    dtostrf(packets[PACKET3].connection, 0, 0, bufConnectionPACKET3);

    char bufHeap[10];
    dtostrf(heap, 0, 0, bufHeap);

    output.printf(buf,
                  getUptimeStr(),
                  ipAddress,
                  bufVoltage,
                  bufAmpere,
                  bufWatt,
                  bufVar,
                  bufApparentPower,
                  bufPowerFactor,
                  bufFrequency,
                  bufPstkwh,
                  bufPstkvarh,
                  bufNgtkvarh,
                  bufwattThreshold,
                  bufCurrentThreshold,
                  bufRequestsPACKET3,
                  bufSuccessful_requestsPACKET3,
                  bufFailed_requestsPACKET3,
                  bufException_errorsPACKET3,
                  bufConnectionPACKET3,
                  bufHeap);

    request->send(200, "text/xml", output);
  }
  else
  {
    request->send(500);
  }
}

void AsyncFSWebServer::send_NTP_configuration_html(AsyncWebServerRequest *request)
{
  DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);

  if (!checkAuth(request))
  {
    return request->requestAuthentication();
  }

  // Save Settings
  //List all parameters
  int params = request->params();
  if (params)
  {
    _configTime.dst = false;
    _configTime.enablertc = false;
    _configTime.enablentp = false;

    for (int i = 0; i < params; i++)
    {
      AsyncWebParameter *p = request->getParam(i);

      DEBUGLOG("POST[%s]: %s\r\n", p->name().c_str(), p->value().c_str());

      const char *name = p->name().c_str();
      uint8_t len = strlen(name);

      if (strncmp_P(name, pgm_timezone, len) == 0)
      {
        // _configTime.timezone = atoi(p->value().c_str());
      }
      if (strncmp_P(name, pgm_dst, len) == 0)
      {
        _configTime.dst = p->value();
      }
      if (strncmp_P(name, pgm_enablertc, len) == 0)
      {
        _configTime.enablertc = p->value();
      }
      if (strncmp_P(name, pgm_syncinterval, len) == 0)
      {
        _configTime.syncinterval = atoll(p->value().c_str());
      }
      if (strncmp_P(name, pgm_enablentp, len) == 0)
      {
        _configTime.enablentp = p->value();
      }
      if (strncmp_P(name, pgm_ntpserver_0, len) == 0)
      {
        strlcpy(_configTime.ntpserver_0, p->value().c_str(), sizeof(_configTime.ntpserver_0) / sizeof(_configTime.ntpserver_0[0]));
      }
      if (strncmp_P(name, pgm_ntpserver_1, len) == 0)
      {
        strlcpy(_configTime.ntpserver_1, p->value().c_str(), sizeof(_configTime.ntpserver_1) / sizeof(_configTime.ntpserver_1[0]));
      }
      if (strncmp_P(name, pgm_ntpserver_2, len) == 0)
      {
        strlcpy(_configTime.ntpserver_2, p->value().c_str(), sizeof(_configTime.ntpserver_2) / sizeof(_configTime.ntpserver_2[0]));
      }
    }

    request->send(200, "text/plain", "OK");

    save_config_time();

    // NTP.setTimeZone(_configTime.timezone / 10);
    // NTP.setDayLight(_configTime.dst);
    // NTP.setInterval(_configTime.syncinterval * 60);
    // setTime(NTP.getTime()); //set time

    return;
  }

  request->send(SPIFFS, request->url());
}

void AsyncFSWebServer::restart_esp(AsyncWebServerRequest *request)
{
  request->send_P(200, PSTR("text/html"), Page_Restart);
  DEBUGLOG("%s\r\n", __FUNCTION__);
  // mqttClient.disconnect();
  evs.close();
  ws.closeAll();
  _fs->end(); // SPIFFS.end();
  // ESP.restart();

  espRestartTimer.once(3, esp_restart);
}

void esp_restart()
{
  ESP.restart();
}

void AsyncFSWebServer::send_wwwauth_configuration_values_html(AsyncWebServerRequest *request)
{
  String values = "";

  values += "wwwauth|" + (String)(_httpAuth.auth ? "checked" : "") + "|chk\n";
  values += "wwwuser|" + (String)_httpAuth.wwwUsername + "|input\n";
  values += "wwwpass|" + (String)_httpAuth.wwwPassword + "|input\n";

  request->send(200, "text/plain", values);

  DEBUGLOG("%s\r\n", __FUNCTION__);
}

void AsyncFSWebServer::send_wwwauth_configuration_html(AsyncWebServerRequest *request)
{
  DEBUGLOG("%s %d\n", __FUNCTION__, request->args());
  if (request->args() > 0) // Save Settings
  {
    _httpAuth.auth = false;
    //String temp = "";
    for (uint8_t i = 0; i < request->args(); i++)
    {
      if (request->argName(i) == "wwwuser")
      {
        _httpAuth.wwwUsername = urldecode(request->arg(i));
        DEBUGLOG("User: %s\r\n", _httpAuth.wwwUsername.c_str());
        continue;
      }
      if (request->argName(i) == "wwwpass")
      {
        _httpAuth.wwwPassword = urldecode(request->arg(i));
        DEBUGLOG("Pass: %s\r\n", _httpAuth.wwwPassword.c_str());
        continue;
      }
      if (request->argName(i) == "wwwauth")
      {
        _httpAuth.auth = true;
        DEBUGLOG("HTTP Auth enabled\r\n");
        continue;
      }
    }

    saveHTTPAuth();
  }
  request->send(SPIFFS, request->url());

  DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);
}

bool AsyncFSWebServer::saveHTTPAuth()
{
  //flag_config = false;
  DEBUGLOG("Save secret\r\n");
  //DynamicJsonDocument jsonBuffer(1024)(256);
  DynamicJsonDocument json(1024);
  //StaticJsonDocument<256> jsonBuffer;
  json["auth"] = _httpAuth.auth;
  json["user"] = _httpAuth.wwwUsername;
  json["pass"] = _httpAuth.wwwPassword;

  //TODO add AP data to html
  File configFile = _fs->open(SECRET_FILE, "w");
  if (!configFile)
  {
    DEBUGLOG("Failed to open secret file for writing\r\n");
    configFile.close();
    return false;
  }

#ifndef RELEASE
  String temp;
  serializeJsonPretty(json, temp);
  DEBUGLOG("%s\r\n", temp);
#endif // RELEASE

  serializeJson(json, configFile);
  configFile.flush();
  configFile.close();
  return true;
}

void AsyncFSWebServer::send_update_firmware_values_html(AsyncWebServerRequest *request)
{
  String values = "";
  uint32_t maxSketchSpace = (ESP.getSketchSize() - 0x1000) & 0xFFFFF000;
  //bool updateOK = Update.begin(maxSketchSpace);
  bool updateOK = maxSketchSpace < ESP.getFreeSketchSpace();
  StreamString result;
  Update.printError(result);
  DEBUGLOG("--MaxSketchSpace: %d\r\n", maxSketchSpace);
  DEBUGLOG("--Update error = %s\r\n", result.c_str());
  values += "remupd|" + (String)((updateOK) ? "OK" : "ERROR") + "|div\n";

  if (Update.hasError())
  {
    result.trim();
    values += "remupdResult|" + result + "|div\n";
  }
  else
  {
    values += "remupdResult||div\n";
  }

  request->send(200, "text/plain", values);
  DEBUGLOG("%s\r\n", __FUNCTION__);
}

void AsyncFSWebServer::setUpdateMD5(AsyncWebServerRequest *request)
{
  _browserMD5 = "";
  DEBUGLOG("Arg number: %d\r\n", request->args());
  if (request->args() > 0) // Read hash
  {
    for (uint8_t i = 0; i < request->args(); i++)
    {
      DEBUGLOG("Arg %s: %s\r\n", request->argName(i).c_str(), request->arg(i).c_str());
      if (request->argName(i) == "md5")
      {
        _browserMD5 = urldecode(request->arg(i));
        Update.setMD5(_browserMD5.c_str());
        continue;
      }
      if (request->argName(i) == "size")
      {
        _updateSize = request->arg(i).toInt();
        DEBUGLOG("Update size: %i\r\n", _updateSize);
        continue;
      }
    }
    request->send(200, "text/html", "OK --> MD5: " + _browserMD5);
  }
}

void AsyncFSWebServer::updateFirmware(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
  // handler for the file upload, get's the sketch bytes, and writes
  // them through the Update object
  static long totalSize = 0;
  if (!index)
  { //UPLOAD_FILE_START
    SPIFFS.end();
    Update.runAsync(true);
    DEBUGLOG("Update start: %s\r\n", filename.c_str());
    // uint32_t maxSketchSpace = ESP.getSketchSize();
    DEBUGLOG("Max free scketch space: %u\r\n", ESP.getSketchSize());
    DEBUGLOG("New scketch size: %u\r\n", _updateSize);
    if (_browserMD5 != NULL && _browserMD5 != "")
    {
      Update.setMD5(_browserMD5.c_str());
      DEBUGLOG("Hash from client: %s\r\n", _browserMD5.c_str());
    }
    if (!Update.begin(_updateSize))
    { //start with max available size
      Update.printError(DEBUGPORT);
    }
  }

  // Get upload file, continue if not start
  totalSize += len;
  DEBUGLOG(".");
  size_t written = Update.write(data, len);
  if (written != len)
  {
    DEBUGLOG("len = %d written = %u totalSize = %li\r\n", len, written, totalSize);
    //Update.printError(DBG_OUTPUT_PORT);
    //return;
  }
  if (final)
  { // UPLOAD_FILE_END
    String updateHash;
    DEBUGLOG("Applying update...\r\n");
    if (Update.end(true))
    { //true to set the size to the current progress
      updateHash = Update.md5String();
      DEBUGLOG("Upload finished. Calculated MD5: %s\r\n", updateHash.c_str());
      DEBUGLOG("Update Success: %u\r\nRebooting...\r\n", request->contentLength());
    }
    else
    {
      updateHash = Update.md5String();
      DEBUGLOG("Upload failed. Calculated MD5: %s\r\n", updateHash.c_str());
      Update.printError(DEBUGPORT);
    }
  }

  //delay(2);
}

void AsyncFSWebServer::send_ssdp_xml_page(AsyncWebServerRequest *request)
{
  DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);

  File configFile = SPIFFS.open(DESCRIPTION_XML_FILE, "r");
  if (!configFile)
  {
    DEBUGLOG("Failed to open config file\r\n");
    return;
  }

  size_t size = configFile.size();
  DEBUGLOG("SSDP_XML_FILE file size: %d bytes\r\n", size);
  if (size > 1024)
  {
    DEBUGLOG("SSDP_XML_FILE file size maybe too large\r\n");
    return;
  }

  // Allocate a buffer to store contents of the file
  char buf[size];

  //copy file to buffer
  configFile.readBytes(buf, size);

  //add termination character at the end
  buf[size] = '\0';

  //close the file, save your memory, keep healthy :-)
  configFile.close();

  DEBUGLOG("%s\r\n", buf);

  StreamString output;

  if (output.reserve(1024))
  {
    //convert IP address to char array
    size_t len = strlen(WiFi.localIP().toString().c_str());
    char URLBase[len + 1];
    strlcpy(URLBase, WiFi.localIP().toString().c_str(), sizeof(URLBase) / sizeof(URLBase[0]));

    // const char *friendlyName = WiFi.hostname().toString().c_str();
    len = strlen(WiFi.hostname().c_str());
    char friendlyName[len + 1];
    strlcpy(friendlyName, WiFi.hostname().c_str(), sizeof(friendlyName) / sizeof(friendlyName[0]));

    char presentationURL[] = "/";
    uint32_t serialNumber = ESP.getChipId();
    char modelName[] = "GHDS100E";
    const char *modelNumber = friendlyName;
    //output.printf(ssdpTemplate,
    output.printf(buf,
                  URLBase,
                  friendlyName,
                  presentationURL,
                  serialNumber,
                  modelName,
                  modelNumber, //modelNumber
                  (uint8_t)((serialNumber >> 16) & 0xff),
                  (uint8_t)((serialNumber >> 8) & 0xff),
                  (uint8_t)serialNumber & 0xff);
    request->send(200, "text/xml", output);
  }
  else
  {
    request->send(500);
  }
}

void AsyncFSWebServer::serverInit()
{
  //SERVER INIT

  evs.onConnect([](AsyncEventSourceClient *client) {
    DEBUGLOG("Event source client connected from %s\r\n", client->client()->remoteIP().toString().c_str());
    //client->send("hello!",NULL,millis(),1000);
    //sendTimeData();
  });
  addHandler(&evs);

  ws.onEvent(onWsEvent);
  addHandler(&ws);

  //  //meter_reading
  //  on("/meter", [this](AsyncWebServerRequest * request) {
  //    if (!this->checkAuth(request))
  //      return request->requestAuthentication();
  //    this->send_meter_reading(request);
  //  });
  on(PSTR("/xml"), [this](AsyncWebServerRequest *request) {
    if (!this->checkAuth(request))
      return request->requestAuthentication();
    this->send_classic_xml_page(request);
  });
  //list directory
  on(PSTR("/list"), HTTP_GET, [this](AsyncWebServerRequest *request) {
    if (!this->checkAuth(request))
      return request->requestAuthentication();
    this->handleFileList(request);
  });

  on(PSTR("/admin/generalvalues"), HTTP_GET, [this](AsyncWebServerRequest *request) {
    if (!this->checkAuth(request))
      return request->requestAuthentication();
    this->send_general_configuration_values_html(request);
  });
  on(PSTR("/admin/values"), [this](AsyncWebServerRequest *request) {
    if (!this->checkAuth(request))
      return request->requestAuthentication();
    this->send_network_configuration_values_html(request);
  });
  on(PSTR("/admin/connectionstate"), [this](AsyncWebServerRequest *request) {
    if (!this->checkAuth(request))
      return request->requestAuthentication();
    this->send_connection_state_values_html(request);
  });
  on(PSTR("/admin/infovalues"), [this](AsyncWebServerRequest *request) {
    if (!this->checkAuth(request))
      return request->requestAuthentication();
    this->send_information_values_html(request);
  });
  on(PSTR("/admin/ntpvalues"), [this](AsyncWebServerRequest *request) {
    if (!this->checkAuth(request))
      return request->requestAuthentication();
    this->send_NTP_configuration_values_html(request);
  });
  on(PSTR("/config.html"), [this](AsyncWebServerRequest *request) {
    if (!this->checkAuth(request))
      return request->requestAuthentication();
    this->send_network_configuration_html(request);
  });
  on(PSTR("/scan"), HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "[";
    int n = WiFi.scanComplete();
    if (n == WIFI_SCAN_FAILED)
    {
      WiFi.scanNetworks(true);
    }
    else if (n)
    {
      for (int i = 0; i < n; ++i)
      {
        if (i)
          json += ",";
        json += "{";
        json += "\"rssi\":" + String(WiFi.RSSI(i));
        json += ",\"ssid\":\"" + WiFi.SSID(i) + "\"";
        json += ",\"bssid\":\"" + WiFi.BSSIDstr(i) + "\"";
        json += ",\"channel\":" + String(WiFi.channel(i));
        json += ",\"secure\":" + String(WiFi.encryptionType(i));
        json += ",\"hidden\":" + String(WiFi.isHidden(i) ? "true" : "false");
        json += "}";
      }
      WiFi.scanDelete();
      if (WiFi.scanComplete() == WIFI_SCAN_FAILED)
      {
        WiFi.scanNetworks(true);
      }
    }
    json += "]";
    request->send(200, "text/json", json);
    json = "";
  });
  //  on("/scan2", HTTP_GET, [](AsyncWebServerRequest * request) {
  //    wifi_scan();
  //    request->send(200, "text/plain", "[" + st + "],[" + rssi + "]");
  //  });
  //  on("/wifirestart", [](AsyncWebServerRequest * request) {
  //    request->send(200, "text/plain", "restart wifi");
  //    wifi_restart();
  //  });
  on(PSTR("/ntp.html"), [this](AsyncWebServerRequest *request) {
    if (!this->checkAuth(request))
      return request->requestAuthentication();
    this->send_NTP_configuration_html(request);
  });
  on(PSTR("/admin/restart"), [this](AsyncWebServerRequest *request) {
    DEBUGPORT.println(request->url());
    if (!this->checkAuth(request))
    {
      return request->requestAuthentication();
    }
    this->restart_esp(request);
  });
  on(PSTR("/admin/wwwauth"), [this](AsyncWebServerRequest *request) {
    if (!this->checkAuth(request))
      return request->requestAuthentication();
    this->send_wwwauth_configuration_values_html(request);
  });
  on(PSTR("/admin"), [this](AsyncWebServerRequest *request) {
    if (!this->checkAuth(request))
      return request->requestAuthentication();
    request->send(SPIFFS, PSTR("/admin.html"));
  });
  on(PSTR("/system.html"), [this](AsyncWebServerRequest *request) {
    if (!this->checkAuth(request))
      return request->requestAuthentication();
    this->send_wwwauth_configuration_html(request);
  });
  on(PSTR("/update/updatepossible"), [this](AsyncWebServerRequest *request) {
    if (!this->checkAuth(request))
      return request->requestAuthentication();
    this->send_update_firmware_values_html(request);
  });
  on(PSTR("/setmd5"), [this](AsyncWebServerRequest *request) {
    if (!this->checkAuth(request))
      return request->requestAuthentication();
    //DBG_OUTPUT_PORT.println("md5?");
    this->setUpdateMD5(request);
  });
  on(PSTR("/update"), HTTP_GET, [this](AsyncWebServerRequest *request) {
    if (!this->checkAuth(request))
      return request->requestAuthentication();
    request->send(SPIFFS, PSTR("/update.html"));
  });
  on(PSTR("/update"), HTTP_POST, [this](AsyncWebServerRequest *request) {
    if (!this->checkAuth(request))
      return request->requestAuthentication();
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", (Update.hasError()) ? "FAIL" : "<META http-equiv=\"refresh\" content=\"15;URL=/update\">Update correct. Restarting...");
    response->addHeader(PSTR("Connection"), PSTR("close"));
    response->addHeader(PSTR("Access-Control-Allow-Origin"), PSTR("*"));
    request->send(response);
    this->_fs->end();
    ESP.restart(); }, [this](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) { this->updateFirmware(request, filename, index, data, len, final); });

  //  on("/savemqtt", [this](AsyncWebServerRequest * request) {
  //    if (!this->checkAuth(request))
  //      return request->requestAuthentication();
  //    this->handleSaveMqtt(request);
  //  });

  //  on("/status", [this](AsyncWebServerRequest * request) {
  //    if (!this->checkAuth(request))
  //      return request->requestAuthentication();
  //    this->handleStatus(request);
  //  });

  on(PSTR("/reset"), [this](AsyncWebServerRequest *request) {
    if (!this->checkAuth(request))
      return request->requestAuthentication();
    this->handleRst(request);
  });

  on(PSTR("/savenetwork"), [this](AsyncWebServerRequest *request) {
    if (!this->checkAuth(request))
      return request->requestAuthentication();
    this->handleSaveNetwork(request);
  });

  on(PSTR("/config/network"), [this](AsyncWebServerRequest *request) {
    if (!this->checkAuth(request))
      return request->requestAuthentication();
    this->send_config_network(request);
  });

#define HIDE_SECRET
#ifdef HIDE_SECRET
  on(SECRET_FILE, HTTP_GET, [this](AsyncWebServerRequest *request) {
    if (!this->checkAuth(request))
      return request->requestAuthentication();
    AsyncWebServerResponse *response = request->beginResponse(403, "text/plain", "Forbidden");
    response->addHeader(PSTR("Connection"), PSTR("close"));
    response->addHeader(PSTR("Access-Control-Allow-Origin"), PSTR("*"));
    request->send(response);
  });
#endif // HIDE_SECRET

#ifdef HIDE_CONFIG
  on(CONFIG_FILE, HTTP_GET, [this](AsyncWebServerRequest *request) {
    if (!this->checkAuth(request))
      return request->requestAuthentication();
    AsyncWebServerResponse *response = request->beginResponse(403, "text/plain", "Forbidden");
    response->addHeader("Connection", "close");
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });
#endif // HIDE_CONFIG

  on("/ssdpxml", [this](AsyncWebServerRequest *request) {
    if (!this->checkAuth(request))
      return request->requestAuthentication();
    this->send_ssdp_xml_page(request);
  });

  //called when the url is not defined here
  //use it to load content from SPIFFS
  onNotFound([this](AsyncWebServerRequest *request) {
    DEBUGLOG("Not found: %s\r\n", request->url().c_str());

    if (!this->checkAuth(request))
      return request->requestAuthentication();

    if (request->method() == HTTP_GET)
    {
      DEBUGLOG("GET");
    }
    else if (request->method() == HTTP_POST)
    {
      DEBUGLOG("POST");
    }
    else if (request->method() == HTTP_DELETE)
    {
      DEBUGLOG("DELETE");
    }
    else if (request->method() == HTTP_PUT)
    {
      DEBUGLOG("PUT");
    }
    else if (request->method() == HTTP_PATCH)
    {
      DEBUGLOG("PATCH");
    }
    else if (request->method() == HTTP_HEAD)
    {
      DEBUGLOG("HEAD");
    }
    else if (request->method() == HTTP_OPTIONS)
    {
      DEBUGLOG("OPTIONS");
    }
    else
    {
      DEBUGLOG("UNKNOWN");
    }
    DEBUGLOG(" http://%s%s\r\n", request->host().c_str(), request->url().c_str());

    if (request->contentLength())
    {
      DEBUGLOG("_CONTENT_TYPE: %s\r\n", request->contentType().c_str());
      DEBUGLOG("_CONTENT_LENGTH: %u\r\n", request->contentLength());
    }

    int i;

#ifndef RELEASE
    int headers = request->headers();
    for (i = 0; i < headers; i++)
    {
      AsyncWebHeader *h = request->getHeader(i);
      DEBUGLOG("_HEADER[%s]: %s\r\n", h->name().c_str(), h->value().c_str());
    }
#endif

    int params = request->params();
    for (i = 0; i < params; i++)
    {
      AsyncWebParameter *p = request->getParam(i);
      if (p->isFile())
      {
        DEBUGLOG("_FILE[%s]: %s, size: %u\r\n", p->name().c_str(), p->value().c_str(), p->size());
      }
      else if (p->isPost())
      {
        DEBUGLOG("_POST[%s]: %s\r\n", p->name().c_str(), p->value().c_str());
      }
      else
      {
        DEBUGLOG("_GET[%s]: %s\r\n", p->name().c_str(), p->value().c_str());
      }
    }

    request->send(404, "text/plain", "File Not Found");

    //    if (ws.hasClient(num)) {
    //      ws.text(num, __PRETTY_FUNCTION__);
    //    }
  });

  addHandler(new SPIFFSEditor());

  //index file
  serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

  onFileUpload([this](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final) {
    if (!index)
      DEBUGLOG("UploadStart: %s\r\n", filename.c_str());
    DEBUGLOG("%s", (const char *)data);
    if (final)
      DEBUGLOG("UploadEnd: %s (%u)\r\n", filename.c_str(), index + len);
  });

  onRequestBody([this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (!index)
      DEBUGLOG("BodyStart: %u\r\n", total);
    DEBUGLOG("%s", (const char *)data);
    if (index + len == total)
      DEBUGLOG("BodyEnd: %u\r\n", total);
  });

  begin(); //--> Not here
  DEBUGLOG("HTTP server started\r\n");
}

bool AsyncFSWebServer::checkAuth(AsyncWebServerRequest *request)
{
  if (!_httpAuth.auth)
  {
    return true;
  }
  else
  {
    return request->authenticate(_httpAuth.wwwUsername.c_str(), _httpAuth.wwwPassword.c_str());
  }
}

//// -------------------------------------------------------------------
//// Save MQTT Config
//// url: /savemqtt
//// -------------------------------------------------------------------
//void AsyncFSWebServer::handleSaveMqtt(AsyncWebServerRequest * request) {
//  config_save_mqtt(request->arg("server"),
//                   request->arg("clientid"),
//                   request->arg("user"),
//                   request->arg("pass"),
//                   request->arg("topic"),
//                   request->arg("prefix"));
//
//  char tmpStr[250];
//  // BUG: Potential buffer overflow issue the values mqtt_xxx come from user
//  //      input so could overflow the buffer no matter the length
//  sprintf(tmpStr, "Saved: %s %s %s %s %s %s", mqtt_server.c_str(), mqtt_clientid.c_str(), mqtt_user.c_str(), mqtt_pass.c_str(), mqtt_topic.c_str(), mqtt_feed_prefix.c_str());
//  DEBUGLOG("%s\r\n", tmpStr);
//  request->send(200, "text/html", tmpStr);
//  //server.send(200, "text/html", tmpStr);
//
//  // If connected disconnect MQTT to trigger re-connect with new details
//  mqtt_restart();
//}

//// -------------------------------------------------------------------
//// Returns status json
//// url: /status
//// -------------------------------------------------------------------
//void AsyncFSWebServer::handleStatus(AsyncWebServerRequest * request) {
//
//  String s = "{";
//  if (wifi_mode == WIFI_MODE_STA) {
//    s += "\"mode\":\"STA\",";
//  } else if (wifi_mode == WIFI_MODE_AP_ONLY) {
//    s += "\"mode\":\"AP\",";
//  } else if (wifi_mode == WIFI_MODE_AP_STA_RETRY) {
//    s += "\"mode\":\"AP-RETRY\",";
//  } else if (wifi_mode == WIFI_MODE_AP_AND_STA) {
//    s += "\"mode\":\"STA+AP\",";
//  }
//  s += "\"networks\":[" + st + "],";
//  s += "\"rssi\":[" + rssi + "],";
//
//  s += "\"ssid\":\"" + esid + "\",";
//  s += "\"pass\":\"" + epass + "\",";
//  s += "\"srssi\":\"" + String(WiFi.RSSI()) + "\",";
//  s += "\"ipaddress\":\"" + sta_ip + "\",";
//  // s += "\"ap_ip\":\"" + String(WiFi.softAPIP()) + "\",";
//  // s += "\"ap_ssid\":\"" + softAP_ssid_ID + "\",";
//  s += "\"ap_ip\":\"" + ap_ip + "\",";
//  s += "\"ap_ssid\":\"" + softAP_ssid_ID + "\",";
//  // s += "\"ipaddress\":\"" + WiFi.localIP().toString() + "\",";
//  //s += "\"emoncms_server\":\"" + emoncms_server + "\",";
//  //s += "\"emoncms_node\":\"" + emoncms_node + "\",";
//  //s += "\"emoncms_apikey\":\"" + emoncms_apikey + "\",";
//  //s += "\"emoncms_fingerprint\":\"" + emoncms_fingerprint + "\",";
//  //s += "\"emoncms_connected\":\"" + String(emoncms_connected) + "\",";
//  //s += "\"packets_sent\":\"" + String(packets_sent) + "\",";
//  //s += "\"packets_success\":\"" + String(packets_success) + "\",";
//
//  s += "\"mqtt_server\":\"" + mqtt_server + "\",";
//  s += "\"mqtt_clientid\":\"" + mqtt_clientid + "\",";
//  s += "\"mqtt_user\":\"" + mqtt_user + "\",";
//  s += "\"mqtt_pass\":\"" + mqtt_pass + "\",";
//  s += "\"mqtt_topic\":\"" + mqtt_topic + "\",";
//  s += "\"mqtt_feed_prefix\":\"" + mqtt_feed_prefix + "\",";
//  s += "\"mqtt_connected\":\"" + String(mqtt_connected()) + "\",";
//
//  s += "\"www_username\":\"" + www_username + "\",";
//  //s += "\"www_password\":\""+www_password+"\",";
//
//  s += "\"free_heap\":\"" + String(ESP.getFreeHeap()) + "\",";
//  s += "\"version\":\"" + String("currentfirmware") + "\",";
//  s += "\"ip\":\"[" + eip_0 + "," + eip_1 + "," + eip_2 + "," + eip_3 + "]\",";
//  s += "\"netmask\":\"[" + enetmask_0 + "," + enetmask_1 + "," + enetmask_2 + "," + enetmask_3 + "]\",";
//  s += "\"gateway\":\"[" + egateway_0 + "," + egateway_1 + "," + egateway_2 + "," + egateway_3 + "]\",";
//  s += "\"dns\":\"[" + edns_0 + "," + edns_1 + "," + edns_2 + "," + edns_3 + "]\",";
//  s += "\"dhcp\":\"" + edhcp + "\",";
//  s += "\"ntp\":\"" + entp_server + "\",";
//  s += "\"NTPperiod\":\"" + entp_period + "\",";
//  s += "\"timeZone\":\"" + etimezone + "\",";
//  s += "\"daylight\":\"" + edaylight + "\",";
//  s += "\"deviceName\":\"" + edevicename + "\"";
//
//  s += "}";
//  request->send(200, "text/html", s);
//}

void AsyncFSWebServer::handleRst(AsyncWebServerRequest *request)
{
  // config_reset();
  // server.send(200, "text/html", "1");
  request->send(200, "text/html", "1");
  WiFi.disconnect();
  // delay(1000);
  ESP.restart();
}

// -------------------------------------------------------------------
// Save selected network to EEPROM and attempt connection
// url: /savenetwork
// -------------------------------------------------------------------
void AsyncFSWebServer::handleSaveNetwork(AsyncWebServerRequest *request)
{
  String qsid = request->arg(PSTR("ssid"));
  String qpass = request->arg(PSTR("password"));

  //decodeURI(qsid);
  //decodeURI(qpass);

  request->send(200, PSTR("text/plain"), PSTR("saved"));

  // if (qsid != 0)
  // {
  //   config_save_wifi(qsid, qpass);

  //   request->send(200, PSTR("text/plain"), PSTR("saved"));
  // }
  // else
  // {
  //   request->send(400, PSTR("text/plain"), PSTR("No SSID"));
  // }

  // String s;
  // String qsid = server.arg("ssid");
  // String qpass = server.arg("pass");
  //
  // decodeURI(qsid);
  // decodeURI(qpass);
  //
  // if (qsid != 0)
  // {
  //   config_save_wifi(qsid, qpass);
  //
  //   server.send(200, "text/plain", "saved");
  //   delay(2000);
  //
  //   wifi_restart();
  // } else {
  //   server.send(400, "text/plain", "No SSID");
  // }

  // config_save_mqtt(request->arg("server"),
  //                  request->arg("clientid"),
  //                  request->arg("user"),
  //                  request->arg("pass"),
  //                  request->arg("topic"),
  //                  request->arg("prefix"));

  //
  //
  // if (request->argName(i) == "password") { _config.pass = urldecode(request->arg(i)); continue;44444 }
  // if (request->argName(i) == "ip_0") { if (checkRange(request->arg(i)))   _config.ip[0] = request->arg(i).toInt(); continue; }
  // if (request->argName(i) == "ip_1") { if (checkRange(request->arg(i)))   _config.ip[1] = request->arg(i).toInt(); continue; }
  // if (request->argName(i) == "ip_2") { if (checkRange(request->arg(i)))   _config.ip[2] = request->arg(i).toInt(); continue; }
  // if (request->argName(i) == "ip_3") { if (checkRange(request->arg(i)))   _config.ip[3] = request->arg(i).toInt(); continue; }
  // if (request->argName(i) == "nm_0") { if (checkRange(request->arg(i)))   _config.netmask[0] = request->arg(i).toInt(); continue; }
  // if (request->argName(i) == "nm_1") { if (checkRange(request->arg(i)))   _config.netmask[1] = request->arg(i).toInt(); continue; }
  // if (request->argName(i) == "nm_2") { if (checkRange(request->arg(i)))   _config.netmask[2] = request->arg(i).toInt(); continue; }
  // if (request->argName(i) == "nm_3") { if (checkRange(request->arg(i)))   _config.netmask[3] = request->arg(i).toInt(); continue; }
  // if (request->argName(i) == "gw_0") { if (checkRange(request->arg(i)))   _config.gateway[0] = request->arg(i).toInt(); continue; }
  // if (request->argName(i) == "gw_1") { if (checkRange(request->arg(i)))   _config.gateway[1] = request->arg(i).toInt(); continue; }
  // if (request->argName(i) == "gw_2") { if (checkRange(request->arg(i)))   _config.gateway[2] = request->arg(i).toInt(); continue; }
  // if (request->argName(i) == "gw_3") { if (checkRange(request->arg(i)))   _config.gateway[3] = request->arg(i).toInt(); continue; }
  // if (request->argName(i) == "dns_0") { if (checkRange(request->arg(i)))  _config.dns[0] = request->arg(i).toInt(); continue; }
  // if (request->argName(i) == "dns_1") { if (checkRange(request->arg(i)))  _config.dns[1] = request->arg(i).toInt(); continue; }
  // if (request->argName(i) == "dns_2") { if (checkRange(request->arg(i)))  _config.dns[2] = request->arg(i).toInt(); continue; }
  // if (request->argName(i) == "dns_3") { if (checkRange(request->arg(i)))  _config.dns[3] = request->arg(i).toInt(); continue; }
  // if (request->argName(i) == "dhcp") { _config.dhcp = true; continue; }

  //request->send(200, "text/html", Page_WaitAndReload);
  // config_save_wifi(_config.ssid, _config.pass);
  //   request->send(200, "text/html", "saved");
  //   // wifi_restart();
  //   save_config();
  //   Serial.println("Config saved");
  //   // config_save_wifi(_config.ssid, _config.pass);
  //   //yield();
  //   // delay(1000);
  //   wifi_restart();
  // }
}