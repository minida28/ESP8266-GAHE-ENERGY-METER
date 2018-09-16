/*
 * -------------------------------------------------------------------
 * EmonESP Serial to Emoncms gateway
 * -------------------------------------------------------------------
 * Adaptation of Chris Howells OpenEVSE ESP Wifi
 * by Trystan Lea, Glyn Hudson, OpenEnergyMonitor
 * All adaptation GNU General Public License as below.
 *
 * -------------------------------------------------------------------
 *
 * This file is part of OpenEnergyMonitor.org project.
 * EmonESP is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * EmonESP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with EmonESP; see the file COPYING.  If not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef _EMONESP_CONFIG_H
#define _EMONESP_CONFIG_H

#include <Arduino.h>
//#include "AsyncJson.h"
#include <ArduinoJson.h>
#include <SPIFFSEditor.h>
#include <DNSServer.h>
#include <ESP8266SSDP.h>
#include <ESP8266NetBIOS.h>
#include <StreamString.h>
#include <time.h>

#include "FSWebServerLib.h"
#include "gahe1progmem.h"

#include "timehelper.h"
#include "sntphelper.h"
#include "PingAlive.h"
#include <pgmspace.h>

#define PRINTPORT Serial1
#define DEBUGPORT Serial1

#define RELEASE
#define RELEASEMQTT
#define RELEASEMODBUS

#define PRINT(fmt, ...)                      \
  {                                          \
    static const char pfmt[] PROGMEM = fmt;  \
    PRINTPORT.printf_P(pfmt, ##__VA_ARGS__); \
  }

#ifndef RELEASE
#define DEBUGLOG(fmt, ...)                   \
  {                                          \
    static const char pfmt[] PROGMEM = fmt;  \
    DEBUGPORT.printf_P(pfmt, ##__VA_ARGS__); \
  }
#else
#define DEBUGLOG(...)
#endif

#ifndef RELEASEMQTT
#define DEBUGMQTT(fmt, ...)                  \
  {                                          \
    static const char pfmt[] PROGMEM = fmt;  \
    DEBUGPORT.printf_P(pfmt, ##__VA_ARGS__); \
  }
#else
#define DEBUGMQTT(...)
#endif

#ifndef RELEASEMODBUS
#define DEBUGMODBUS(fmt, ...)                \
  {                                          \
    static const char pfmt[] PROGMEM = fmt;  \
    DEBUGPORT.printf_P(pfmt, ##__VA_ARGS__); \
  }
#else
#define DEBUGMODBUS(...)
#endif

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

// -------------------------------------------------------------------
// Load and save the EmonESP config.
//
// This initial implementation saves the config to the EEPROM area of flash
// -------------------------------------------------------------------

// Global config varables

// Wifi Network Strings
extern String esid;
extern String epass;

extern String eip_0;
extern String eip_1;
extern String eip_2;
extern String eip_3;
extern String enetmask_0;
extern String enetmask_1;
extern String enetmask_2;
extern String enetmask_3;
extern String egateway_0;
extern String egateway_1;
extern String egateway_2;
extern String egateway_3;
extern String edns_0;
extern String edns_1;
extern String edns_2;
extern String edns_3;
extern String edhcp;
extern String entp_server;
extern String entp_period;
extern String etimezone;
extern String edaylight;
extern String edevicename;

// MQTT Settings
extern String mqtt_server;
extern String mqtt_clientid;
extern String mqtt_pass;
extern String mqtt_feed_prefix;
extern String mqtt_topic;
extern String mqtt_user;

// Web server authentication (leave blank for none)
extern String www_username;
extern String www_password;

// EMONCMS SERVER strings
extern String emoncms_server;
extern String emoncms_node;
extern String emoncms_apikey;
extern String emoncms_fingerprint;

// -------------------------------------------------------------------
// Load saved settings
// -------------------------------------------------------------------
extern void config_load_settings();

// -------------------------------------------------------------------
// Save the EmonCMS server details
// -------------------------------------------------------------------
extern void config_save_emoncms(String server, String node, String apikey, String fingerprint);

// -------------------------------------------------------------------
// Save the MQTT broker details
// -------------------------------------------------------------------
extern void config_save_mqtt(String server, String clientid, String user, String pass, String topic, String prefix);

// -------------------------------------------------------------------
// Save the admin/web interface details
// -------------------------------------------------------------------
extern void config_save_admin(String user, String pass);

// -------------------------------------------------------------------
// Save the Wifi details
// -------------------------------------------------------------------
extern void config_save_wifi(String qsid, String qpass);

// -------------------------------------------------------------------
// Save ALL CONFIG
// -------------------------------------------------------------------
extern void config_save_all(
    String qsid,
    String qpass,
    String qip_0,
    String qip_1,
    String qip_2,
    String qip_3,
    String qnetmask_0,
    String qnetmask_1,
    String qnetmask_2,
    String qnetmask_3,
    String qgateway_0,
    String qgateway_1,
    String qgateway_2,
    String qgateway_3,
    String qdns_0,
    String qdns_1,
    String qdns_2,
    String qdns_3,
    String qdhcp,
    String qntp_server,
    String qntp_period,
    String qtimezone,
    String qdaylight,
    String qdevicename);

// -------------------------------------------------------------------
// Reset the config back to defaults
// -------------------------------------------------------------------
extern void config_reset();

// -------------------------------------------------------------------
// Added by me
// -------------------------------------------------------------------

#endif // _EMONESP_CONFIG_H
