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


#ifndef _mqtthelper_h
#define _mqtthelper_h

// -------------------------------------------------------------------
// MQTT support
// -------------------------------------------------------------------

#include <Arduino.h>
//#include <WiFiClient.h>
#include <AsyncMqttClient.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>

extern AsyncMqttClient mqttClient;

// MQTT config
typedef struct {
  char server[64] = "10.0.2.2";
  uint16_t port = 1883;
  char user[32] = "test";
  char pass[64] = "test";
  char clientid[16] = "kwh1";
  uint16_t keepalive = 5;
  bool cleansession = true;
  char lwttopic[64] = "kwh1/mqttstatus";
  uint8_t lwtqos = 2;
  bool lwtretain = true;
  char lwtpayload[64] = "DISCONNECTED";

  char publish_1_topic[64] = "kwh1/mqttstatus";
  uint8_t publish_1_qos = 2;
  bool publish_1_retain = true;
  char publish_1_payload[64] = "CONNECTED";
  char publish_2_topic[64] = "/rumah/sts/kwh1/wattthreshold";
  uint8_t publish_2_qos = 2;
  bool publish_2_retain = true;
  char publish_2_payload[64] = "";
  char subscribe_1_topic[64] = "/rumah/cmd/kwh1/wattthreshold";
  uint8_t subscribe_1_qos = 2;
  char subscribe_2_topic[64] = "/rumah/cmd/kwh1/currentthreshold";
  uint8_t subscribe_2_qos = 2;

  char pub_param[11][18] = {"voltage","ampere","watt","var","frequency","pstkwh","pstkvarh","ngtkvarh","powerfactor","apparentpower","unk2"};
  char pub_default_basetopic[32] = "/rumah/sts/kwh1/";
  char pub_basetopic[32] = "rumah/kwh1/";
  char pub_1s_basetopic[32] = "/rumah/sts/1s/kwh1/";
  char pub_10s_basetopic[32] = "/rumah/sts/10s/kwh1/";
  char pub_15s_basetopic[32] = "/rumah/sts/15s/kwh1/";
  char pub_prefix_1s[5] = "1s";
  char pub_prefix_10s[5] = "10s";
  char pub_prefix_15s[5] = "15s";
  char powerthreshold_param[24] = "wattthreshold";
  char currentthreshold_param[24] = "currentthreshold";
} strConfigMqtt;

extern  strConfigMqtt configMqtt;


extern void mqtt_loop();

extern bool mqtt_setup();

// extern bool mqtt_reconnect();




#endif //








