// #include <ESP8266WiFi.h>
// #include <ESPAsyncTCP.h>

// #define LittleFS LITTLEFS

#include "FSWebServerLib.h"
#include "modbus.h"
#include "StreamString.h"
#include <ArduinoJson.h>
#include <Ticker.h>
#include "SyncClient.h"
#include "gahe1progmem.h"

// extern "C"
// {
// #include <osapi.h>
// #include <os_type.h>
// }

#define DEBUGPORT Serial

#define RELEASE

#ifndef RELEASE
#define DEBUGLOG(fmt, ...)                       \
    {                                            \
        static const char pfmt[] PROGMEM = fmt;  \
        DEBUGPORT.printf_P(pfmt, ##__VA_ARGS__); \
    }
#else
#define DEBUGLOG(...)
#endif

#define SERVER_HOST_NAME "api.thingspeak.com"

#define TCP_PORT 80

// static os_timer_t intervalTimer;

static void replyToServer(void *arg)
{
    AsyncClient *client = reinterpret_cast<AsyncClient *>(arg);

    // send reply
    if (client->space() > 32 && client->canSend())
    {
        char message[32];
        sprintf(message, "this is from %s", WiFi.localIP().toString().c_str());
        client->add(message, strlen(message));
        client->send();
    }
}

/* event callbacks */
// static void handleData(void *arg, AsyncClient *client, void *data, size_t len)
// {
//     DEBUGLOG("\r\n data received from %s \r\n", client->remoteIP().toString().c_str());

//     size_t lenData = len + 1;
//     char receivedData[lenData];

//     uint8_t *d = (uint8_t *)data;
//     for (size_t i = 0; i < lenData; i++)
//     {
//         receivedData[i] = (char)d[i];
//     }
//     receivedData[lenData] = '\0';

//     DEBUGLOG("Data: %s\r\n", receivedData);

//     if (ws.count())
//     {
//         //   ws.textAll(buf);
//         //   ws.textAll("\r\n");
//         ws.textAll(receivedData);
//         ws.textAll("\r\n");
//         // }
//     }

//     client->close(true);

//     // char temp[len + 1];

//     // uint8_t *d = (uint8_t *)data;
//     // for (size_t i = 0; i < len; i++)
//     // {
//     //     static int j = 0;
//     //     char z = (char)d[i];
//     //     temp[j] = z;
//     //     j++;
//     //     if (z == '\n')
//     //     {
//     //         temp[j + 1] = '\0';
//     //         j = 0;
//     //         char status[] = "HTTP/1.1 200 OK";
//     //         int len = strlen(status);
//     //         if (strncmp(temp, status, len) == 0)
//     //         {
//     //             if (ws.count())
//     //                 ws.textAll(status);

//     //             bClient->close(true);

//     //             // AsyncClient *client = bClient;
//     //             // bClient = NULL;
//     //             // delete c;

//     //             break;
//     //         }
//     //     }
//     // }

//     // os_timer_arm(&intervalTimer, 15000, true); // schedule for reply to server at next 2s
// }

void onConnect(void *arg, AsyncClient *client)
{
    DEBUGLOG("\r\n Client has been connected to %s on port %d \r\n", SERVER_HOST_NAME, TCP_PORT);
    replyToServer(client);
}

void onDisconnect(void *arg, AsyncClient *client)
{
    DEBUGLOG("\r\n TCP client disconnected from %s on port %d \r\n", SERVER_HOST_NAME, TCP_PORT);
    // replyToServer(client);
}

void onError(void *arg, AsyncClient *client, err_t error)
{
    DEBUGLOG("\r\n TCP client error, %s on port %d \r\n", SERVER_HOST_NAME, TCP_PORT);
    client->close(true);
    // replyToServer(client);
}

void onTimeout(void *arg, AsyncClient *client, uint32_t time)
{
    DEBUGLOG("\r\n TCP client timeout, %s on port %d \r\n", SERVER_HOST_NAME, TCP_PORT);
    client->close(true);
    // replyToServer(client);
}

void ConstructThingspeak()
{
    File file = LittleFS.open(PSTR("/thingspeak.json"), "r");
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

        // client->write(output.c_str());

        if (ws.count())
        {
            ws.textAll(PSTR("Thingspeak send request\r\n"));
            ws.textAll(output.c_str());
        }
    }
}

Ticker tickerConnectToThingspeak;

bool triggered = false;

void trigger()
{
    triggered = true;
}

bool connected = false;
bool failed = false;

char bufResponse[64];

void SyncClientThingspeak()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        tickerConnectToThingspeak.once(15, trigger);
        return;
    }

    uint32_t freeheap = ESP.getFreeHeap();
    char bufHeap[10];
    dtostrf(freeheap, 0, 0, bufHeap);

    SyncClient client;

    if (packets[PACKET2].connection == 0) return;

    if (!client.connect("api.thingspeak.com", 80))
    {
        failed = true;
        return;
    }
    client.setTimeout(2);

    if (client.printf_P(pgm_thingspeak_template,
                        "POST",
                        "/update?",
                        bufAmpere,
                        bufWatt,
                        bufVoltage,
                        bufPstkwh,
                        bufVar,
                        bufApparentPower,
                        bufPowerFactor,
                        bufHeap,
                        ESPHTTPServer._config.hostname,
                        "api.thingspeak.com",
                        "QSK88PPX9COHGEAA") > 0)
    {
        connected = true;

        while (client.connected() && client.available() == 0)
        {
            yield();
        }

        uint8_t i = 0;

        while (client.available())
        {
            // Serial.write(client.read());
            char c = (char)client.read();

            uint8_t maxLen = sizeof(bufResponse)/sizeof(bufResponse[0])-1;

            if (i < maxLen)
            {
                bufResponse[i] = c;
                bufResponse[i + 1] = '\0';
                i++;
                if (c == '\n')
                {
                    client.flush();
                    break;
                }
            }
        }

        if (client.connected())
        {
            client.stop();
        }
    }
    else
    {
        client.stop();
        // if (ws.count())
        //     ws.textAll("Send failed\r\n");

        failed = 2;

        // while (client.connected())
        //     delay(0);
    }

    tickerConnectToThingspeak.once(15, trigger);
}

void Thingspeaksetup()
{

    // AsyncClient *client = new AsyncClient;
    // client->onData(&handleData, client);
    // client->onConnect(&onConnect, client);
    // client->onDisconnect(&onDisconnect, client);
    // client->onError(&onError, client);
    // client->onTimeout(&onTimeout, client);
    // client->connect(SERVER_HOST_NAME, TCP_PORT);

    // os_timer_disarm(&intervalTimer);
    // os_timer_setfn(&intervalTimer, &replyToServer, client);

    tickerConnectToThingspeak.once(15, trigger);
}

void Thingspeakloop()
{
    if (triggered)
    {
        triggered = false;

        SyncClientThingspeak();

         if (ws.count())
            ws.textAll("Triggered\n");
    }

    if (connected)
    {
        connected = false;

        // if (ws.count())
        //     ws.textAll("Connected\r\n");

        if (ws.count())
        {
            const char *txt = bufResponse;
            ws.textAll(txt);
        }
    }

    if (failed)
    {
        if (failed == 1)
        {
            if (ws.count())
                ws.textAll("Send failed\r\n");
        }
        else if (failed == 2)
        {
            if (ws.count())
                ws.textAll("Send failed 2\r\n");
        }

        failed = false;
    }
}