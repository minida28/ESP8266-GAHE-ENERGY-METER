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

#include "mqtt.h"

long reconnectInterval = 30000;
int clientTimeout = 0;
int i = 0;

AsyncMqttClient mqttClient;

Ticker mqttReconnectTimer;

WiFiEventHandler wifiConnectHandlerForMqtt;
WiFiEventHandler wifiDisconnectHandlerForMqtt;


//IPAddress mqttServer(192, 168, 10, 3);

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

// MQTT config struct
strConfigMqtt configMqtt;

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  DEBUGMQTT("Connected to Wi-Fi.\r\n");
  if (WiFi.status() == WL_CONNECTED) {
    connectToMqtt();
  }
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  DEBUGMQTT("Disconnected from Wi-Fi.\r\n");
  mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  //wifiReconnectTimer.once(2, connectToWifi);
}

void connectToMqtt() {
  DEBUGMQTT("Connecting to MQTT...\r\n");
  mqttClient.connect();
}

void onMqttConnect(bool sessionPresent) {
  DEBUGMQTT("Connected to MQTT.\n  Session present: %d\r\n");

  File configFile = SPIFFS.open(PUBSUBJSON_FILE, "r");
  if (!configFile) {
    DEBUGMQTT("Failed to open config file\r\n");
    return;
  }

  size_t size = configFile.size();
  DEBUGMQTT("CONFIG_FILE_MQTT_PUBSUB file size: %d bytes\r\n", size);
  if (size > 1024) {
    DEBUGMQTT("WARNING, file size maybe too large\r\n");

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
  StaticJsonBuffer<1024> jsonBuffer;

  JsonObject& json = jsonBuffer.parseObject(buf.get());

  //  JsonVariant json;
  //  json = jsonBuffer.parse(buf.get());

  if (!json.success()) {
    DEBUGMQTT("Failed to parse MQTT config file\r\n");
    return;
  }

#ifndef RELEASEMQTT
  String temp;
  json.prettyPrintTo(temp);
  Serial.println(temp);
#endif

  // publish 1
  const char* publish_1_topic = json[FPSTR(pgm_publish_1)][0];
  uint8_t publish_1_qos = json[FPSTR(pgm_publish_1)][1];
  bool publish_1_retain = json[FPSTR(pgm_publish_1)][2];
  const char* publish_1_payload = json[FPSTR(pgm_publish_1)][0];
  uint16_t packetIdPub1 = mqttClient.publish(
                            json[FPSTR(pgm_publish_1)][0],  //topic
                            json[FPSTR(pgm_publish_1)][1],  //qos
                            json[FPSTR(pgm_publish_1)][2],  //retain
                            json[FPSTR(pgm_publish_1)][3]   //payload
                          );
  DEBUGMQTT(
    "Publishing packetId: %d\n  topic:  %s\n QoS:  %d\n retain:  %d\n payload:  %s\r\n",
    packetIdPub1, publish_1_topic, publish_1_qos, publish_1_retain, publish_1_topic
  );

  // subscribe 1
  const char* subscribe_1_topic = json[FPSTR(pgm_subscribe_1)][0];
  uint8_t subscribe_1_qos = json[FPSTR(pgm_subscribe_1)][1];
  uint16_t packetIdSub1 = mqttClient.subscribe(subscribe_1_topic, subscribe_1_qos);
  DEBUGMQTT("Subscribing packetId: %d\n  topic: %s\n  QoS: %d\r\n", packetIdSub1, subscribe_1_topic, subscribe_1_qos);

  //  //subscribe 2
  //  const char* subscribe_2_topic = json[FPSTR(pgm_subscribe_2)][0];
  //  uint8_t subscribe_2_qos = json[FPSTR(pgm_subscribe_2)][1];
  //  uint16_t packetIdSub2 = mqttClient.subscribe(subscribe_2_topic, subscribe_2_qos);
  //  DEBUGMQTT("Subscribing packetId: %d\n  topic: %s\n  QoS: %d\r\n"), packetIdSub2, subscribe_2_topic, subscribe_2_qos);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  DEBUGMQTT("Disconnected from MQTT.\r\n");

  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(5, connectToMqtt);
  }
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  DEBUGMQTT("Subscribe acknowledged.\n  packetId: %d\n  qos: %d\r\n", packetId, qos);
}

void onMqttUnsubscribe(uint16_t packetId) {
  DEBUGMQTT("Unsubscribe acknowledged.\n  packetId: %d\r\n", packetId);
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  //  Serial.println("Payload received.");
  //  Serial.print("  topic: ");
  //  Serial.println(topic);
  //  Serial.print("  qos: ");
  //  Serial.println(properties.qos);
  //  Serial.print("  dup: ");
  //  Serial.println(properties.dup);
  //  Serial.print("  retain: ");
  //  Serial.println(properties.retain);
  //  Serial.print("  len: ");
  //  Serial.println(len);
  //  Serial.print("  index: ");
  //  Serial.println(index);
  //  Serial.print("  total: ");
  //  Serial.println(total);

  DEBUGMQTT("MQTT received [%s] ", topic);
  for (int i = 0; i < len; i++) {
    DEBUGMQTT("%c", (char)payload[i]);
  }
  DEBUGMQTT("\r\n");

  char t[64];
  strcpy(t, topic);

  File configFile = SPIFFS.open(PUBSUBJSON_FILE, "r");
  if (!configFile) {
    DEBUGMQTT("Failed to open config file\r\n");
    return;
  }

  size_t size = configFile.size();
  //DEBUGMQTT("CONFIG_FILE_MQTT_PUBSUB file size: %d bytes\r\n", size);
  if (size > 1024) {
    DEBUGMQTT("WARNING, file size maybe too large\r\n");

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
  StaticJsonBuffer<1024> jsonBuffer;

  JsonObject& json = jsonBuffer.parseObject(buf.get());

  //  JsonVariant json;
  //  json = jsonBuffer.parse(buf.get());

  if (!json.success()) {
    DEBUGMQTT("Failed to parse MQTT config file\r\n");
    return;
  }

  //#ifndef RELEASEMQTT
  //  String temp;
  //  json.prettyPrintTo(temp);
  //  Serial.println(temp);
  //#endif

  const char* subscribe_1_topic = json["subscribe_1"][0];
  uint8_t subscribe_1_qos =  json["subscribe_1"][1];

  //handle wattThreshold payload
  if (strncmp(const_cast<char*>(topic), subscribe_1_topic, json[FPSTR(pgm_subscribe_1)][0].measureLength()) == 0) {

    //copy payload
    strlcpy(bufwattThreshold, payload, sizeof(bufwattThreshold));
    wattThreshold = atof(bufwattThreshold);

    //roundup value
    dtostrf(atof(bufwattThreshold), 0, 1, bufwattThreshold);

    // acknowledge the receipt by sending back status
    mqttClient.publish("/rumah/sts/kwh1/wattthreshold", 2, true, bufwattThreshold);
  }
}

void onMqttPublish(uint16_t packetId) {
  DEBUGMQTT("Publish acknowledged.\r\n");
  DEBUGMQTT("  packetId: %d\r\n", packetId);
}

boolean mqtt_setup() {

  File configFile = SPIFFS.open(MQTT_CONFIG_FILE, "r");
  if (!configFile) {
    DEBUGMQTT("Failed to open config file\r\n");
    return false;
  }

  size_t size = configFile.size();
  /*if (size > 1024) {
      DEBUGMQTT("Config file size is too large");
      configFile.close();
      return false;
    }*/

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  // We don't use String here because ArduinoJson library requires the input
  // buffer to be mutable. If you don't use ArduinoJson, you may as well
  // use configFile.readString instead.
  configFile.readBytes(buf.get(), size);
  configFile.close();
  DEBUGMQTT("JSON file size: %d bytes\r\n", size);
  //DynamicJsonBuffer jsonBuffer(1024);
  StaticJsonBuffer<1024> jsonBuffer;

  JsonObject& json = jsonBuffer.parseObject(buf.get());

  //  JsonVariant json;
  //  json = jsonBuffer.parse(buf.get());

  if (!json.success()) {
    DEBUGMQTT("Failed to parse MQTT config file\r\n");
    return false;
  }

#ifndef RELEASEMQTT
  String temp;
  json.prettyPrintTo(temp);
  Serial.println(temp);
#endif

  //uint16_t len;
  auto mqtt_server = json[FPSTR(pgm_mqtt_server)].as<const char*>();
  //strcpy(configMqtt.server, mqtt_server);
  strlcpy(configMqtt.server, json[FPSTR(pgm_mqtt_server)], sizeof(configMqtt.server));

  auto mqtt_port = (uint16_t)json[FPSTR(pgm_mqtt_port)];
  //configMqtt.port = mqtt_port;
  configMqtt.port = json[FPSTR(pgm_mqtt_port)];

  auto mqtt_user = json[FPSTR(pgm_mqtt_user)].as<const char*>();
  //len = json[FPSTR(pgm_mqtt_user)].measureLength();
  //strcpy(configMqtt.user, mqtt_user);
  strlcpy(configMqtt.user, json[FPSTR(pgm_mqtt_user)], sizeof(configMqtt.user));

  auto mqtt_pass = json[FPSTR(pgm_mqtt_pass)].as<const char*>();
  //strcpy(configMqtt.pass, mqtt_pass);
  strlcpy(configMqtt.pass, json[FPSTR(pgm_mqtt_pass)], sizeof(configMqtt.pass));

  auto mqtt_clientid = json[FPSTR(pgm_mqtt_clientid)].as<const char*>();
  //strcpy(configMqtt.clientid, mqtt_clientid);
  strlcpy(configMqtt.clientid, json[FPSTR(pgm_mqtt_clientid)], sizeof(configMqtt.clientid));

  auto mqtt_keepalive = (uint16_t)json[FPSTR(pgm_mqtt_keepalive)];
  //configMqtt.keepalive = mqtt_keepalive;
  configMqtt.keepalive = json[FPSTR(pgm_mqtt_keepalive)];

  auto mqtt_cleansession = (bool)json[FPSTR(pgm_mqtt_cleansession)];
  //configMqtt.cleansession = mqtt_cleanSession;
  configMqtt.cleansession = json[FPSTR(pgm_mqtt_cleansession)];

  //lwt
  auto mqtt_lwttopic = json[FPSTR(pgm_mqtt_lwt)][0].as<const char*>();
  //strcpy(configMqtt.lwttopic, mqtt_lwtlwttopic);
  strlcpy(configMqtt.lwttopic, json[FPSTR(pgm_mqtt_lwt)][0], sizeof(configMqtt.lwttopic));

  auto mqtt_lwtqos =  (uint8_t)json[FPSTR(pgm_mqtt_lwt)][1];
  //configMqtt.lwtqos = mqttlwt_qos;
  configMqtt.lwtqos = json[FPSTR(pgm_mqtt_lwt)][1];

  auto mqtt_lwtretain =  (bool)json[FPSTR(pgm_mqtt_lwt)][2];
  //configMqtt.lwtretain = mqtt_lwt_retain;
  configMqtt.lwtretain = json[FPSTR(pgm_mqtt_lwt)][2];

  auto mqtt_lwtpayload = json[FPSTR(pgm_mqtt_lwt)][3].as<const char*>();
  //strcpy(configMqtt.lwtpayload, mqtt_lwtpayload);
  strlcpy(configMqtt.lwtpayload, json[FPSTR(pgm_mqtt_lwt)][3], sizeof(configMqtt.lwtpayload));


  wifiConnectHandlerForMqtt = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandlerForMqtt = WiFi.onStationModeDisconnected(onWifiDisconnect);

  //register mqtt handler
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);

  //setup
  mqttClient.setServer(configMqtt.server, configMqtt.port);
  mqttClient.setCredentials(configMqtt.user, configMqtt.pass);
  mqttClient.setClientId(configMqtt.clientid);
  mqttClient.setKeepAlive(configMqtt.keepalive);
  mqttClient.setCleanSession(configMqtt.cleansession);
  mqttClient.setWill(configMqtt.lwttopic, configMqtt.lwtqos,
                     configMqtt.lwtretain, configMqtt.lwtpayload
                    );

  return true;
}

