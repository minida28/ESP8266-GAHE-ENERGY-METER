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

#include "modbus.h"
#include "mqtt.h"


#define RELEASE

#define DEBUGPORT Serial1

#ifndef RELEASE
#define DEBUGLOG(fmt, ...)                   \
  {                                          \
    static const char pfmt[] PROGMEM = fmt;  \
    DEBUGPORT.printf_P(pfmt, ##__VA_ARGS__); \
  }
#else
#define DEBUGLOG(...)
#endif

const char pgm_txt_subcribedTopic_0[] PROGMEM = "/rumah/sts/1s/kwh1/watt";
const char pgm_txt_subcribedTopic_1[] PROGMEM = "/rumah/sts/1s/kwh1/voltage";

const char pgm_subscribe_1[] PROGMEM = "subscribe_1";
const char pgm_subscribe_2[] PROGMEM = "subscribe_2";
const char pgm_publish_1[] PROGMEM = "publish_1";

const char pgm_mqtt_server[] PROGMEM = "mqtt_server";
const char pgm_mqtt_port[] PROGMEM = "mqtt_port";
const char pgm_mqtt_keepalive[] PROGMEM = "mqtt_keepalive";
const char pgm_mqtt_cleansession[] PROGMEM = "mqtt_cleansession";
const char pgm_mqtt_lwt[] PROGMEM = "mqtt_lwt";
const char pgm_mqtt_user[] PROGMEM = "mqtt_user";
const char pgm_mqtt_pass[] PROGMEM = "mqtt_pass";
const char pgm_mqtt_clientid[] PROGMEM = "mqtt_clientid";

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

//IPAddress mqttServer(192, 168, 10, 3);

// MQTT config struct
strConfigMqtt configMqtt;

void onWifiConnect(const WiFiEventStationModeGotIP &event)
{
  DEBUGLOG("Connected to Wi-Fi.\r\n");
  if (WiFi.status() == WL_CONNECTED)
  {
    connectToMqtt();
  }
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected &event)
{
  DEBUGLOG("Disconnected from Wi-Fi.\r\n");
  mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  //wifiReconnectTimer.once(2, connectToWifi);
}

void connectToMqtt()
{
  DEBUGLOG("Connecting to MQTT...\r\n");
  mqttClient.connect();
}

void onMqttConnect(bool sessionPresent)
{
  mqttConnectedFlag = true;
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
  mqttDisconnectedFlag = true;
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos)
{
  mqttSubscribeFlag = true;
}

void onMqttUnsubscribe(uint16_t packetId)
{
  mqttUnsubscribeFlag = true;
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
  mqttPublishFlag = true;
}

bool mqtt_setup()
{
  //register mqtt handler
  wifiConnectHandlerForMqtt = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandlerForMqtt = WiFi.onStationModeDisconnected(onWifiDisconnect);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);

  if (!mqtt_load_config())
  {
    return false;
  }

  mqttClient.setServer(configMqtt.server, configMqtt.port);
  mqttClient.setCredentials(configMqtt.user, configMqtt.pass);
  mqttClient.setClientId(configMqtt.clientid);
  mqttClient.setKeepAlive(configMqtt.keepalive);
  mqttClient.setCleanSession(configMqtt.cleansession);
  mqttClient.setWill(configMqtt.lwttopic, configMqtt.lwtqos,
                     configMqtt.lwtretain, configMqtt.lwtpayload);

  if (!mqtt_load_pubsubconfig())
  {
    return false;
  }

  return true;
}

bool mqtt_load_config()
{
  SPIFFS.begin();

  File configFile = SPIFFS.open(MQTT_CONFIG_FILE, "r");
  if (!configFile)
  {
    configFile.close();
    DEBUGLOG("Failed to open config file\r\n");
    return false;
  }

  size_t size = configFile.size();
  DEBUGLOG("JSON file size: %d bytes\r\n", size);

  if (size > 1024)
  {
    DEBUGLOG("Config file size is too large");
    configFile.close();
    return false;
  }

  StaticJsonDocument<1024> json;
  auto error = deserializeJson(json, configFile);

  if (error)
  {
    DEBUGLOG("Failed to parse MQTT config file\r\n");
    return false;
  }

  // #ifndef RELEASEMQTT
  serializeJsonPretty(json, DEBUGPORT);
  DEBUGLOG("\r\n");
  // #endif

  strlcpy(configMqtt.server, json[FPSTR(pgm_mqtt_server)], sizeof(configMqtt.server) / sizeof(configMqtt.server[0]));
  configMqtt.port = json[FPSTR(pgm_mqtt_port)];
  strlcpy(configMqtt.user, json[FPSTR(pgm_mqtt_user)], sizeof(configMqtt.user) / sizeof(configMqtt.user[0]));
  strlcpy(configMqtt.pass, json[FPSTR(pgm_mqtt_pass)], sizeof(configMqtt.pass) / sizeof(configMqtt.pass[0]));
  strlcpy(configMqtt.clientid, json[FPSTR(pgm_mqtt_clientid)], sizeof(configMqtt.clientid) / sizeof(configMqtt.clientid[0]));
  configMqtt.keepalive = json[FPSTR(pgm_mqtt_keepalive)];
  configMqtt.cleansession = json[FPSTR(pgm_mqtt_cleansession)];
  strlcpy(configMqtt.lwttopic, json[FPSTR(pgm_mqtt_lwt)][0], sizeof(configMqtt.lwttopic) / sizeof(configMqtt.lwttopic[0]));
  configMqtt.lwtqos = json[FPSTR(pgm_mqtt_lwt)][1];
  configMqtt.lwtretain = json[FPSTR(pgm_mqtt_lwt)][2];
  strlcpy(configMqtt.lwtpayload, json[FPSTR(pgm_mqtt_lwt)][3], sizeof(configMqtt.lwtpayload) / sizeof(configMqtt.lwtpayload[0]));

  return true;
}

bool mqtt_load_pubsubconfig()
{
  File pubSubJsonFile = SPIFFS.open(PUBSUBJSON_FILE, "r");
  if (!pubSubJsonFile)
  {
    DEBUGLOG("Failed to open PUBSUBJSON_FILE file\r\n");
    pubSubJsonFile.close();
    return false;
  }

  const uint16_t allocatedJsonBufferSize = 1280;
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

  //close the file, save your memory, keep healthy :-)
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

void MqttConnectedCb()
{
  // DEBUGLOG("Connected to MQTT.\r\n  Session present: %d\r\n");
  DEBUGLOG("Connected to MQTT.\r\n");

  File configFile = SPIFFS.open(PUBSUBJSON_FILE, "r");
  if (!configFile)
  {
    DEBUGLOG("Failed to open config file\r\n");
    return;
  }

  size_t size = configFile.size();
  DEBUGLOG("PUBSUBJSON_FILE file size: %d bytes\r\n", size);
  if (size > 1024)
  {
    DEBUGLOG("WARNING, file size maybe too large\r\n");

    //configFile.close();

    //return false;
  }

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  // We don't use String here because ArduinoJson library requires the input
  // buffer to be mutable. If you don't use ArduinoJson, you may as well
  // use configFile.readString instead.
  configFile.readBytes(buf.get(), size);
  configFile.close();
  //DynamicJsonBuffer jsonBuffer(1024);
  StaticJsonDocument<1536> json;

  auto error = deserializeJson(json, buf.get());

  //  JsonVariant json;
  //  json = jsonBuffer.parse(buf.get());

  if (error)
  {
    DEBUGLOG("Failed to parse MQTT config file\r\n");
    return;
  }

#ifndef RELEASE
  String temp;
  serializeJsonPretty(json, temp);
  Serial.println(temp);
#endif

  // publish 1
  mqttClient.publish(
      configMqtt.publish_1_topic,  //topic
      configMqtt.publish_1_qos,    //qos
      configMqtt.publish_1_retain, //retain
      configMqtt.publish_1_payload //payload
  );
  DEBUGLOG(
      "Publishing packet topic:  %s\r\n QoS:  %d\r\n retain:  %d\r\n payload:  %s\r\n",
      configMqtt.publish_1_topic, configMqtt.publish_1_qos, configMqtt.publish_1_retain, configMqtt.publish_1_payload);

  // publish 2
  dtostrf(currentThreshold, 0, 1, bufCurrentThreshold);
  mqttClient.publish(PSTR("/rumah/sts/kwh1/currentthreshold"), 2, true, bufCurrentThreshold);

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

void MqttDisconnectedCb()
{
  DEBUGLOG("Disconnected from MQTT.\r\n");

  if (WiFi.isConnected())
  {
    mqttReconnectTimer.once(5, connectToMqtt);
  }
}

void mqtt_loop()
{
  if (mqttConnectedFlag)
  {
    mqttConnectedFlag = false;
    MqttConnectedCb();
  }

  if (mqttDisconnectedFlag)
  {
    mqttDisconnectedFlag = false;
    MqttDisconnectedCb();
  }

  if (mqttPublishFlag)
  {
    mqttPublishFlag = false;
    DEBUGLOG("Publish acknowledged.\r\n");
    // DEBUGLOG("  packetId: %d\r\n", packetId);
  }

  if (mqttSubscribeFlag)
  {
    mqttSubscribeFlag = false;
    // DEBUGLOG("Subscribe acknowledged.\n  packetId: %d\n  qos: %d\r\n", packetId, qos);
    DEBUGLOG("Subscribe acknowledged.\r\n");
  }

  if (mqttUnsubscribeFlag)
  {
    mqttUnsubscribeFlag = false;
    // DEBUGLOG("Unsubscribe acknowledged.\n  packetId: %d\r\n", packetId);
    DEBUGLOG("Unsubscribe acknowledged.\r\n");
  }

  if (mqttOnMessageFlag)
  {
    mqttOnMessageFlag = false;

    const char *subscribe_1_topic = configMqtt.subscribe_1_topic;

    //handle energy meter payload
    // if (strncmp(const_cast<char *>(topic), sub1_topic, json[FPSTR(sub1_topic)].measureLength()) == 0)
    if (strncmp(bufTopic, subscribe_1_topic, strlen(bufTopic)) == 0)
    {
      DEBUGLOG("MQTT received [%s]\r\n", bufTopic);
      DEBUGLOG("Payload: %s\r\n", bufPayload);

      //copy payload to buffer
      // strlcpy(bufwattThreshold, bufPayload, sizeof(bufwattThreshold) / sizeof(bufwattThreshold[0]));

      wattThreshold = atoi(bufPayload);

      //roundup value
      dtostrf(wattThreshold, 0, 0, bufwattThreshold);

      // acknowledge the receipt by sending back status
      mqttClient.publish(PSTR("/rumah/sts/kwh1/wattthreshold"), 2, true, bufwattThreshold);
    }

    const char *subscribe_2_topic = configMqtt.subscribe_2_topic;
    // const char *subscribe_2_topic = "/rumah/cmd/kwh1/currentthreshold";

    //handle energy meter payload
    // if (strncmp(const_cast<char *>(topic), sub1_topic, json[FPSTR(sub1_topic)].measureLength()) == 0)
    if (strncmp(bufTopic, subscribe_2_topic, strlen(bufTopic)) == 0)
    {
      DEBUGLOG("MQTT received [%s]\r\n", bufTopic);
      DEBUGLOG("Payload: %s\r\n", bufPayload);

      //copy payload
      // strlcpy(bufCurrentThreshold, bufPayload, sizeof(bufCurrentThreshold) / sizeof(bufCurrentThreshold[0]));

      currentThreshold = atof(bufPayload);

      //roundup value
      dtostrf(currentThreshold, 0, 1, bufCurrentThreshold);

      // acknowledge the receipt by sending back status
      mqttClient.publish(PSTR("/rumah/sts/kwh1/currentthreshold"), 2, true, bufCurrentThreshold);
    }
  }
}