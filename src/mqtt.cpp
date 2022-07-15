/*
   -------------------------------------------------------------------
   EmonESP Serial to Emoncms gateway
   -------------------------------------------------------------------
   Adaptation of Chris Howells OpenEVSE ESP Wifi
   by Trystan Lea, Glyn Hudson, OpenEnergyMonitor
   All adaptation GNU General Public License as below.

   -------------------------------------------------------------------

   This file is part of OpenEnergyMonitor.org project.
   EmonESP is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.
   EmonESP is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with EmonESP; see the file COPYING.  If not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <Arduino.h>

#include <ArduinoJson.h>
#include "modbus.h"
#include "mqtt.h"
#include "gahe1progmem.h"
#include "FSWebServerLib.h"
#include "config.h"

// #define RELEASE

#define DEBUGPORT Serial

#ifndef RELEASE
#define DEBUGLOG(fmt, ...)                       \
    {                                            \
        static const char pfmt[] PROGMEM = fmt;  \
        DEBUGPORT.printf_P(pfmt, ##__VA_ARGS__); \
    }
#else
#define DEBUGLOG(...)
#endif

#define MQTT_CONFIG_FILE "/configmqtt.json"
#define PUBSUBJSON_FILE "/pubsub.json"

const char pgm_txt_subcribedTopic_0[] PROGMEM = "/rumah/sts/1s/kwh1/watt";
const char pgm_txt_subcribedTopic_1[] PROGMEM = "/rumah/sts/1s/kwh1/voltage";

const char pgm_subscribe_1[] PROGMEM = "subscribe_1";
const char pgm_subscribe_2[] PROGMEM = "subscribe_2";
const char pgm_publish_1[] PROGMEM = "publish_1";

// const char pgm_mqtt_server[] PROGMEM = "mqtt_server";
// const char pgm_mqtt_port[] PROGMEM = "mqtt_port";
// const char pgm_mqtt_keepalive[] PROGMEM = "mqtt_keepalive";
// const char pgm_mqtt_cleansession[] PROGMEM = "mqtt_cleansession";
// const char pgm_mqtt_lwt[] PROGMEM = "mqtt_lwt";
// const char pgm_mqtt_user[] PROGMEM = "mqtt_user";
// const char pgm_mqtt_pass[] PROGMEM = "mqtt_pass";
// const char pgm_mqtt_clientid[] PROGMEM = "mqtt_clientid";

// const char pgm_mqtt_enabled[] PROGMEM = "enabled";
// const char pgm_mqtt_server[] PROGMEM = "server";
// const char pgm_mqtt_port[] PROGMEM = "port";
// const char pgm_mqtt_user[] PROGMEM = "user";
// const char pgm_mqtt_pass[] PROGMEM = "pass";
// const char pgm_mqtt_clientid[] PROGMEM = "clientid";
// const char pgm_mqtt_keepalive[] PROGMEM = "keepalive";
// const char pgm_mqtt_cleansession[] PROGMEM = "cleansession";
// const char pgm_mqtt_lwttopicprefix[] PROGMEM = "lwttopicprefix";
// const char pgm_mqtt_lwtqos[] PROGMEM = "lwtqos";
// const char pgm_mqtt_lwtretain[] PROGMEM = "lwtretain";
// const char pgm_mqtt_lwtpayload[] PROGMEM = "lwtpayload";

bool wifiConnectedFlag = false;
bool mqttConnectedFlag = false;
bool mqttDisconnectedFlag = false;
bool mqttPublishFlag = false;
bool mqttSubscribeFlag = false;
bool mqttUnsubscribeFlag = false;
bool mqttOnMessageFlag = false;

uint32_t reconnectInterval = 30000;
int clientTimeout = 0;
int i = 0;

char bufTopic[64];
char bufPayload[256];

AsyncMqttClient mqttClient;

Ticker mqttReconnectTimer;

WiFiEventHandler wifiConnectHandlerForMqtt;
WiFiEventHandler wifiDisconnectHandlerForMqtt;

// IPAddress mqttServer(192, 168, 10, 3);

// MQTT config struct
strConfigMqtt configMqtt;

void connectToMqtt()
{
    DEBUGLOG("Connecting to MQTT...\r\n");
    mqttClient.connect();
}

void onWifiGotIp(const WiFiEventStationModeGotIP &event)
{
    DEBUGLOG("Connected to Wi-Fi.\r\n");
    
    if (WiFi.isConnected())
    {
        mqttReconnectTimer.once(5, connectToMqtt);
    }
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected &event)
{
    DEBUGLOG("Disconnected from Wi-Fi.\r\n");
    mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
                                 // wifiReconnectTimer.once(2, connectToWifi);
}



void MqttDisconnectedCb()
{
    DEBUGLOG("Disconnected from MQTT.\r\n");

    mqttConnectedFlag = false;

    if (WiFi.isConnected())
    {
        mqttReconnectTimer.once(5, connectToMqtt);
    }
}

void onMqttConnect(bool sessionPresent)
{
    DEBUGLOG("MQTT Connected.\r\n");
    mqttConnectedFlag = true;

    char bufFullTopic[64];
    strlcpy(bufFullTopic, ESPHTTPServer._config.hostname, sizeof(bufFullTopic) / sizeof(bufFullTopic[0]));
    strncat(bufFullTopic, "/mqttstatus", sizeof(bufFullTopic) / sizeof(bufFullTopic[0]));

    // publish 1
    mqttClient.publish(
        bufFullTopic,                // topic
        configMqtt.publish_1_qos,    // qos
        configMqtt.publish_1_retain, // retain
        configMqtt.publish_1_payload // payload
    );

    // publish 2
    dtostrf(currentThreshold, 0, 1, bufCurrentThreshold);
    mqttClient.publish("/rumah/sts/kwh1/currentthreshold", 2, true, bufCurrentThreshold);

    // subscribe 1
    mqttClient.subscribe(
        configMqtt.subscribe_1_topic,
        configMqtt.subscribe_1_qos);
    DEBUGLOG(
        "Subscribing packet  topic: %s\n  QoS: %d\r\n",
        configMqtt.subscribe_1_topic, configMqtt.subscribe_1_qos);

    // subscribe 2
    mqttClient.subscribe(
        configMqtt.subscribe_2_topic,
        configMqtt.subscribe_2_qos);
    DEBUGLOG(
        "Subscribing packet  topic: %s\n  QoS: %d\r\n",
        configMqtt.subscribe_2_topic, configMqtt.subscribe_2_qos);

    //  //subscribe 2
    //  const char* subscribe_2_topic = json[FPSTR(pgm_subscribe_2)][0];
    //  uint8_t subscribe_2_qos = json[FPSTR(pgm_subscribe_2)][1];
    //  uint16_t packetIdSub2 = mqttClient.subscribe(subscribe_2_topic, subscribe_2_qos);
    //  DEBUGLOG("Subscribing packetId: %d\r\n  topic: %s\r\n  QoS: %d\r\n"), packetIdSub2, subscribe_2_topic, subscribe_2_qos);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
    // mqttDisconnectedFlag = true;
    DEBUGLOG("Disconnected from MQTT.\r\n");

    if (WiFi.isConnected())
    {
        mqttReconnectTimer.once(5, connectToMqtt);
    }
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos)
{
  Serial.println("Subscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  Serial.print("  qos: ");
  Serial.println(qos);
}

void onMqttUnsubscribe(uint16_t packetId)
{
  Serial.println("Unsubscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
    mqttOnMessageFlag = true;

    DEBUGLOG("Payload received\r\n  topic: %s\r\n  qos: %d\r\n  dup: %d\r\n  retain: %d\r\n  len: %d\r\n  index: %d\r\n  total: %d\r\n",
             topic, properties.qos, properties.dup, properties.retain, len, index, total);

    size_t lenTopic = strlen(topic) + 1;
    DEBUGLOG("topic len:%d, payload len:%d", lenTopic, len);

    if (lenTopic > sizeof(bufTopic) - 1)
    {
        DEBUGLOG("Topic length is too large!");
        return;
    }

    if (len > sizeof(bufPayload) - 1)
    {
        DEBUGLOG("Payload length is too large!");
        return;
    }

    for (size_t i = 0; i < lenTopic; i++)
    {
        bufTopic[i] = (char)topic[i];
    }
    bufTopic[lenTopic] = '\0';

    for (size_t i = 0; i < len; i++)
    {
        bufPayload[i] = (char)payload[i];
    }
    bufPayload[len] = '\0';
}

void onMqttPublish(uint16_t packetId)
{
  Serial.println("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

// bool mqtt_load_config()
// {
//   // MYFS.begin();

//   File configFile = MYFS.open(MQTT_CONFIG_FILE, "r");
//   if (!configFile)
//   {
//     configFile.close();
//     DEBUGLOG("Failed to open config file\r\n");
//     return false;
//   }

//   size_t size = configFile.size();
//   DEBUGLOG("JSON file size: %d bytes\r\n", size);

//   if (size > 1024)
//   {
//     DEBUGLOG("Config file size is too large");
//     configFile.close();
//     return false;
//   }

//   StaticJsonDocument<1024> json;
//   auto error = deserializeJson(json, configFile);

//   if (error)
//   {
//     DEBUGLOG("Failed to parse MQTT config file\r\n");
//     return false;
//   }

//   #ifndef RELEASE
//   serializeJsonPretty(json, DEBUGPORT);
//   DEBUGLOG("\r\n");
//   #endif

//   strlcpy(configMqtt.server, json[FPSTR(pgm_mqtt_server)], sizeof(configMqtt.server) / sizeof(configMqtt.server[0]));
//   configMqtt.port = json[FPSTR(pgm_mqtt_port)];
//   strlcpy(configMqtt.user, json[FPSTR(pgm_mqtt_user)], sizeof(configMqtt.user) / sizeof(configMqtt.user[0]));
//   strlcpy(configMqtt.pass, json[FPSTR(pgm_mqtt_pass)], sizeof(configMqtt.pass) / sizeof(configMqtt.pass[0]));
//   strlcpy(configMqtt.clientid, json[FPSTR(pgm_mqtt_clientid)], sizeof(configMqtt.clientid) / sizeof(configMqtt.clientid[0]));
//   configMqtt.keepalive = json[FPSTR(pgm_mqtt_keepalive)];
//   configMqtt.cleansession = json[FPSTR(pgm_mqtt_cleansession)];
//   strlcpy(configMqtt.lwttopic, json[FPSTR(pgm_mqtt_lwt)][0], sizeof(configMqtt.lwttopic) / sizeof(configMqtt.lwttopic[0]));
//   configMqtt.lwtqos = json[FPSTR(pgm_mqtt_lwt)][1];
//   configMqtt.lwtretain = json[FPSTR(pgm_mqtt_lwt)][2];
//   strlcpy(configMqtt.lwtpayload, json[FPSTR(pgm_mqtt_lwt)][3], sizeof(configMqtt.lwtpayload) / sizeof(configMqtt.lwtpayload[0]));

//   return true;
// }

bool load_config_mqtt()
{
    DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);

    // size_t fileLen = strlen_P(pgm_configfilemqtt);
    // char filename[fileLen + 1];
    // strcpy_P(filename, pgm_configfilemqtt);

    // File file = MYFS.open(filename, "r");
    DEBUGLOG("Opening file %s...\r\n", MQTT_CONFIG_FILE);
    File file = MYFS.open(MQTT_CONFIG_FILE, "r");

    if (!file)
    {
        DEBUGLOG("Failed to open config file\r\n");
        file.close();
        return false;
    }

    const uint16_t allocSize = 512;

    if (file.size() > allocSize)
    {
        DEBUGLOG("File size %d bytes is larger than allocSize %d bytes!\r\n", file.size(), allocSize);
        file.close();
        return false;
    }

    StaticJsonDocument<allocSize> root;
    DeserializationError error = deserializeJson(root, file);

    file.close();

    if (error)
    {
        DEBUGLOG("Failed parsing config MQTT file\r\n");
        return false;
    }

#ifndef RELEASE
    String temp;
    serializeJsonPretty(root, temp);
    DEBUGLOG("%s\r\n", temp.c_str());
#endif

    configMqtt.enabled = root[FPSTR(pgm_mqtt_enabled)];
    strlcpy(configMqtt.server, root[FPSTR(pgm_mqtt_server)], sizeof(configMqtt.server) / sizeof(configMqtt.server[0]));
    configMqtt.port = root[FPSTR(pgm_mqtt_port)];
    strlcpy(configMqtt.user, root[FPSTR(pgm_mqtt_user)], sizeof(configMqtt.user) / sizeof(configMqtt.user[0]));
    strlcpy(configMqtt.pass, root[FPSTR(pgm_mqtt_pass)], sizeof(configMqtt.pass) / sizeof(configMqtt.pass[0]));
    strlcpy(configMqtt.clientid, ESPHTTPServer._config.hostname, sizeof(configMqtt.clientid) / sizeof(configMqtt.clientid[0]));
    configMqtt.keepalive = root[FPSTR(pgm_mqtt_keepalive)];
    configMqtt.cleansession = root[FPSTR(pgm_mqtt_cleansession)];

    // lwt
    strlcpy(configMqtt.lwttopicprefix, root[FPSTR(pgm_mqtt_lwttopicprefix)], sizeof(configMqtt.lwttopicprefix) / sizeof(configMqtt.lwttopicprefix[0]));
    configMqtt.lwtqos = root[FPSTR(pgm_mqtt_lwtqos)];
    configMqtt.lwtretain = root[FPSTR(pgm_mqtt_lwtretain)];
    strlcpy(configMqtt.lwtpayload, root[FPSTR(pgm_mqtt_lwtpayload)], sizeof(configMqtt.lwtpayload) / sizeof(configMqtt.lwtpayload[0]));

    // construct full lwt topic
    char lwtbasetopic[sizeof(ESPHTTPServer._config.hostname)];
    strlcpy(lwtbasetopic, ESPHTTPServer._config.hostname, sizeof(lwtbasetopic));

    char lwttopicprefix[sizeof(configMqtt.lwttopicprefix)];
    strlcpy(lwttopicprefix, configMqtt.lwttopicprefix, sizeof(lwttopicprefix));

    char buflwttopic[64];
    strlcpy(buflwttopic, lwtbasetopic, sizeof(buflwttopic) / sizeof(buflwttopic[0]));

    // check if lwt topic prefix is available or not
    // if yes, add "/" as needed
    if (strncmp(lwttopicprefix, "", 1) == 0)
    {
        // do nothing
        DEBUGLOG("lwt topic prefix is not available. Do nothing...\r\n");
    }
    else
    {
        char bufSlash[] = "/";
        strncat(buflwttopic, bufSlash, sizeof(buflwttopic) / sizeof(buflwttopic[0]));
        strncat(buflwttopic, lwttopicprefix, sizeof(buflwttopic) / sizeof(buflwttopic[0]));
    }

    // store full topic
    strlcpy(configMqtt.lwtfulltopic, buflwttopic, sizeof(configMqtt.lwtfulltopic));

    DEBUGLOG("\r\nConfig MQTT loaded successfully.\r\n");
    DEBUGLOG("enabled: %d\r\n", configMqtt.enabled);
    DEBUGLOG("server: %s\r\n", configMqtt.server);
    DEBUGLOG("port: %d\r\n", configMqtt.port);
    DEBUGLOG("user: %s\r\n", configMqtt.user);
    DEBUGLOG("pass: %s\r\n", configMqtt.pass);
    DEBUGLOG("clientid: %s\r\n", configMqtt.clientid);
    DEBUGLOG("keepalive: %d\r\n", configMqtt.keepalive);
    DEBUGLOG("cleansession: %d\r\n", configMqtt.cleansession);
    DEBUGLOG("lwttopicprefix: %s\r\n", configMqtt.lwttopicprefix);
    DEBUGLOG("full lwt topic: %s\r\n", buflwttopic);
    DEBUGLOG("lwtqos: %d\r\n", configMqtt.lwtqos);
    DEBUGLOG("lwtretain: %d\r\n", configMqtt.lwtretain);
    DEBUGLOG("lwtpayload: %s\r\n", configMqtt.lwtpayload);

    mqttClient.setServer(configMqtt.server, configMqtt.port);

    if (strcmp(configMqtt.user, "") == 0 || strcmp(configMqtt.pass, "") == 0)
    {
        // do nothing
        DEBUGLOG("configMqtt.user or configMqtt.pass is empty. Do nothing...\r\n");
    }
    else
    {
        mqttClient.setCredentials(configMqtt.user, configMqtt.pass);
    }

    mqttClient.setClientId(configMqtt.clientid);
    mqttClient.setKeepAlive(configMqtt.keepalive);
    mqttClient.setCleanSession(configMqtt.cleansession);
    mqttClient.setWill(configMqtt.lwtfulltopic,
                       configMqtt.lwtqos,
                       configMqtt.lwtretain,
                       configMqtt.lwtpayload);

    return true;
}

bool mqtt_load_pubsubconfig()
{
    File pubSubJsonFile = MYFS.open(PUBSUBJSON_FILE, "r");
    if (!pubSubJsonFile)
    {
        DEBUGLOG("Failed to open PUBSUBJSON_FILE file\r\n");
        pubSubJsonFile.close();
        return false;
    }

    const uint16_t allocatedJsonBufferSize = 1500;
    DEBUGLOG("Allocated JsonBufferSize size: %d bytes\r\n", allocatedJsonBufferSize);

    uint16_t size = pubSubJsonFile.size();
    DEBUGLOG("PUBSUBJSON_FILE file size: %d bytes\r\n", size);

    if (size > allocatedJsonBufferSize)
    {
        DEBUGLOG("WARNING!! PUBSUBJSON_FILE file size is too large! \r\n");
        pubSubJsonFile.close();
        // return false;
    }

    StaticJsonDocument<allocatedJsonBufferSize> root;
    auto error = deserializeJson(root, pubSubJsonFile);

    // close the file, save your memory, keep healthy :-)
    pubSubJsonFile.close();

    if (error)
    {
        DEBUGLOG("Failed to parse PUBSUBJSON_FILE file\r\n");
        return false;
    }

    strlcpy(configMqtt.pub_default_basetopic, root[FPSTR(pgm_pub_default_basetopic)], sizeof(configMqtt.pub_default_basetopic) / sizeof(configMqtt.pub_default_basetopic[0]));

    JsonArray pub_param = root[FPSTR(pgm_pub_param)];

    uint16_t jsonArraySize = pub_param.size();
    uint16_t pub_param_numrows = sizeof(configMqtt.pub_param) / sizeof(configMqtt.pub_param[0]);
    uint16_t pub_param_numcols = sizeof(configMqtt.pub_param[0]) / sizeof(char);

    if (jsonArraySize > pub_param_numrows)
    {
        DEBUGLOG("Json param array size is too large!\r\n");
        return false;
    }

    if (jsonArraySize < pub_param_numrows)
    {
        DEBUGLOG("WARNING, Json param array size is smaller!\r\n");
        // return false;
    }

    for (uint16_t i = 0; i < jsonArraySize; i++)
    {
        const char *pr = pub_param[i];
        strlcpy(configMqtt.pub_param[i], pr, pub_param_numcols);
    }

    return true;
}

bool mqtt_setup()
{
    // register mqtt handler
    wifiConnectHandlerForMqtt = WiFi.onStationModeGotIP(onWifiGotIp);
    wifiDisconnectHandlerForMqtt = WiFi.onStationModeDisconnected(onWifiDisconnect);

    mqttClient.onConnect(onMqttConnect);
    mqttClient.onDisconnect(onMqttDisconnect);
    mqttClient.onSubscribe(onMqttSubscribe);
    mqttClient.onUnsubscribe(onMqttUnsubscribe);
    mqttClient.onMessage(onMqttMessage);
    mqttClient.onPublish(onMqttPublish);

    if (!load_config_mqtt())
    {
        return false;
    }

    // mqttClient.setServer(configMqtt.server, configMqtt.port);
    // mqttClient.setCredentials(configMqtt.user, configMqtt.pass);
    // mqttClient.setClientId(configMqtt.clientid);
    // mqttClient.setKeepAlive(configMqtt.keepalive);
    // mqttClient.setCleanSession(configMqtt.cleansession);
    // mqttClient.setWill(configMqtt.lwttopic, configMqtt.lwtqos,
    //                    configMqtt.lwtretain, configMqtt.lwtpayload);

    if (!mqtt_load_pubsubconfig())
    {
        return false;
    }

    // ESPHTTPServer.on("/status/mqtt", [](AsyncWebServerRequest *request) {
    //   DEBUGLOG("%s\r\n", request->url().c_str());

    //   AsyncResponseStream *response = request->beginResponseStream("application/json");
    //   StaticJsonDocument<256> root;
    //   root["connected"] = mqttClient.connected();
    //   serializeJson(root, *response);
    //   request->send(response);
    // });

    // ESPHTTPServer.begin();

    return true;
}

void mqtt_loop()
{


    // if (mqttConnectedFlag)
    // {
    //     mqttConnectedFlag = false;
    // }

    // if (mqttDisconnectedFlag)
    // {
    //   mqttDisconnectedFlag = false;
    //   MqttDisconnectedCb();
    // }


    if (mqttOnMessageFlag)
    {
        mqttOnMessageFlag = false;

        const char *subscribe_1_topic = configMqtt.subscribe_1_topic;

        // handle energy meter payload
        //  if (strncmp(const_cast<char *>(topic), sub1_topic, json[FPSTR(sub1_topic)].measureLength()) == 0)
        if (strncmp(bufTopic, subscribe_1_topic, strlen(bufTopic)) == 0)
        {
            DEBUGLOG("MQTT received [%s]\r\n", bufTopic);
            DEBUGLOG("Payload: %s\r\n", bufPayload);

            // copy payload to buffer
            //  strlcpy(bufwattThreshold, bufPayload, sizeof(bufwattThreshold) / sizeof(bufwattThreshold[0]));

            wattThreshold = atoi(bufPayload);

            // roundup value
            dtostrf(wattThreshold, 0, 0, bufwattThreshold);

            // acknowledge the receipt by sending back status
            mqttClient.publish(PSTR("/rumah/sts/kwh1/wattthreshold"), 2, true, bufwattThreshold);
        }

        const char *subscribe_2_topic = configMqtt.subscribe_2_topic;
        // const char *subscribe_2_topic = "/rumah/cmd/kwh1/currentthreshold";

        // handle energy meter payload
        //  if (strncmp(const_cast<char *>(topic), sub1_topic, json[FPSTR(sub1_topic)].measureLength()) == 0)
        if (strncmp(bufTopic, subscribe_2_topic, strlen(bufTopic)) == 0)
        {
            DEBUGLOG("MQTT received [%s]\r\n", bufTopic);
            DEBUGLOG("Payload: %s\r\n", bufPayload);

            // copy payload
            strlcpy(bufCurrentThreshold, bufPayload, sizeof(bufCurrentThreshold) / sizeof(bufCurrentThreshold[0]));

            currentThreshold = atof(bufPayload);

            // roundup value
            dtostrf(currentThreshold, 0, 1, bufCurrentThreshold);

            // acknowledge the receipt by sending back status
            mqttClient.publish(PSTR("/rumah/sts/kwh1/currentthreshold"), 2, true, bufCurrentThreshold);
        }
    }
}