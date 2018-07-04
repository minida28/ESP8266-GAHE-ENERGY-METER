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

//#include "emonesp.h"
#include "config.h"

#include <Arduino.h>
#include <EEPROM.h>                   // Save config settings


// Wifi Network Strings
String esid = "";
String epass = "";
String eip_0 = "";
String eip_1 = "";
String eip_2 = "";
String eip_3 = "";
String enetmask_0 = "";
String enetmask_1 = "";
String enetmask_2 = "";
String enetmask_3 = "";
String egateway_0 = "";
String egateway_1 = "";
String egateway_2 = "";
String egateway_3 = "";
String edns_0 = "";
String edns_1 = "";
String edns_2 = "";
String edns_3 = "";
String edhcp = "";
String entp_server = "";
String entp_period = "";
String etimezone = "";
String edaylight = "";
String edevicename = "";


// Web server authentication (leave blank for none)
String www_username = "";
String www_password = "";

// EMONCMS SERVER strings
String emoncms_server = "";
String emoncms_node = "";
String emoncms_apikey = "";
String emoncms_fingerprint = "";

// MQTT Settings
String mqtt_server = "";
String mqtt_clientid = "";
String mqtt_user = "";
String mqtt_pass = "";
String mqtt_topic = "";
String mqtt_feed_prefix = "";

#define EEPROM_ESID_SIZE          32
#define EEPROM_EPASS_SIZE         64

#define EEPROM_IP_0_SIZE         3
#define EEPROM_IP_1_SIZE         3
#define EEPROM_IP_2_SIZE         3
#define EEPROM_IP_3_SIZE         3

#define EEPROM_NETMASK_0_SIZE     3
#define EEPROM_NETMASK_1_SIZE     3
#define EEPROM_NETMASK_2_SIZE     3
#define EEPROM_NETMASK_3_SIZE     3

#define EEPROM_GATEWAY_0_SIZE     3
#define EEPROM_GATEWAY_1_SIZE     3
#define EEPROM_GATEWAY_2_SIZE     3
#define EEPROM_GATEWAY_3_SIZE     3

#define EEPROM_DNS_0_SIZE     3
#define EEPROM_DNS_1_SIZE     3
#define EEPROM_DNS_2_SIZE     3
#define EEPROM_DNS_3_SIZE     3

#define EEPROM_DHCP_SIZE     5

#define EEPROM_NTP_SERVER_SIZE   45
#define EEPROM_NTP_PERIOD_SIZE   6
#define EEPROM_TIMEZONE_SIZE   6
#define EEPROM_DAYLIGHT_SIZE   2
#define EEPROM_DEVICENAME_SIZE   45

#define EEPROM_EMON_API_KEY_SIZE  32
#define EEPROM_EMON_SERVER_SIZE   45
#define EEPROM_EMON_NODE_SIZE     32
#define EEPROM_EMON_FINGERPRINT_SIZE  60

#define EEPROM_MQTT_SERVER_SIZE   45
#define EEPROM_MQTT_CLIENTID_SIZE 32
#define EEPROM_MQTT_USER_SIZE     32
#define EEPROM_MQTT_PASS_SIZE     32
#define EEPROM_MQTT_TOPIC_SIZE    32
#define EEPROM_MQTT_FEED_PREFIX_SIZE  10

#define EEPROM_WWW_USER_SIZE      16
#define EEPROM_WWW_PASS_SIZE      16

#define EEPROM_SIZE 1024

#define EEPROM_ESID_START            0
#define EEPROM_ESID_END             (EEPROM_ESID_START + EEPROM_ESID_SIZE)
#define EEPROM_EPASS_START           EEPROM_ESID_END
#define EEPROM_EPASS_END            (EEPROM_EPASS_START + EEPROM_EPASS_SIZE)

#define EEPROM_IP_0_START           EEPROM_EPASS_END
#define EEPROM_IP_0_END             (EEPROM_IP_0_START + EEPROM_IP_0_SIZE)
#define EEPROM_IP_1_START           EEPROM_IP_0_END
#define EEPROM_IP_1_END             (EEPROM_IP_1_START + EEPROM_IP_1_SIZE)
#define EEPROM_IP_2_START           EEPROM_IP_1_END
#define EEPROM_IP_2_END             (EEPROM_IP_2_START + EEPROM_IP_2_SIZE)
#define EEPROM_IP_3_START           EEPROM_IP_2_END
#define EEPROM_IP_3_END             (EEPROM_IP_3_START + EEPROM_IP_3_SIZE)

#define EEPROM_NETMASK_0_START      EEPROM_IP_3_END
#define EEPROM_NETMASK_0_END        (EEPROM_NETMASK_0_START + EEPROM_NETMASK_0_SIZE)
#define EEPROM_NETMASK_1_START      EEPROM_NETMASK_0_END
#define EEPROM_NETMASK_1_END        (EEPROM_NETMASK_1_START + EEPROM_NETMASK_1_SIZE)
#define EEPROM_NETMASK_2_START      EEPROM_NETMASK_1_END
#define EEPROM_NETMASK_2_END        (EEPROM_NETMASK_2_START + EEPROM_NETMASK_2_SIZE)
#define EEPROM_NETMASK_3_START      EEPROM_NETMASK_2_END
#define EEPROM_NETMASK_3_END        (EEPROM_NETMASK_3_START + EEPROM_NETMASK_3_SIZE)

#define EEPROM_GATEWAY_0_START      EEPROM_NETMASK_3_END
#define EEPROM_GATEWAY_0_END        (EEPROM_GATEWAY_0_START + EEPROM_GATEWAY_0_SIZE)
#define EEPROM_GATEWAY_1_START      EEPROM_GATEWAY_0_END
#define EEPROM_GATEWAY_1_END        (EEPROM_GATEWAY_1_START + EEPROM_GATEWAY_1_SIZE)
#define EEPROM_GATEWAY_2_START      EEPROM_GATEWAY_1_END
#define EEPROM_GATEWAY_2_END        (EEPROM_GATEWAY_2_START + EEPROM_GATEWAY_2_SIZE)
#define EEPROM_GATEWAY_3_START      EEPROM_GATEWAY_2_END
#define EEPROM_GATEWAY_3_END        (EEPROM_GATEWAY_3_START + EEPROM_GATEWAY_3_SIZE)

#define EEPROM_DNS_0_START          EEPROM_GATEWAY_3_END
#define EEPROM_DNS_0_END            (EEPROM_DNS_0_START + EEPROM_DNS_0_SIZE)
#define EEPROM_DNS_1_START          EEPROM_DNS_0_END
#define EEPROM_DNS_1_END            (EEPROM_DNS_1_START + EEPROM_DNS_1_SIZE)
#define EEPROM_DNS_2_START          EEPROM_DNS_1_END
#define EEPROM_DNS_2_END            (EEPROM_DNS_2_START + EEPROM_DNS_2_SIZE)
#define EEPROM_DNS_3_START          EEPROM_DNS_2_END
#define EEPROM_DNS_3_END            (EEPROM_DNS_3_START + EEPROM_DNS_3_SIZE)

#define EEPROM_DHCP_START           EEPROM_DNS_3_END
#define EEPROM_DHCP_END             (EEPROM_DHCP_START + EEPROM_DHCP_SIZE)

#define EEPROM_NTP_SERVER_START     EEPROM_DHCP_END
#define EEPROM_NTP_SERVER_END       (EEPROM_NTP_SERVER_START + EEPROM_NTP_SERVER_SIZE)
#define EEPROM_NTP_PERIOD_START     EEPROM_NTP_SERVER_END
#define EEPROM_NTP_PERIOD_END       (EEPROM_NTP_PERIOD_START + EEPROM_NTP_PERIOD_SIZE)
#define EEPROM_TIMEZONE_START       EEPROM_NTP_PERIOD_END
#define EEPROM_TIMEZONE_END         (EEPROM_TIMEZONE_START + EEPROM_TIMEZONE_SIZE)
#define EEPROM_DAYLIGHT_START       EEPROM_TIMEZONE_END
#define EEPROM_DAYLIGHT_END         (EEPROM_DAYLIGHT_START + EEPROM_DAYLIGHT_SIZE)
#define EEPROM_DEVICENAME_START     EEPROM_DAYLIGHT_END
#define EEPROM_DEVICENAME_END       (EEPROM_DEVICENAME_START + EEPROM_DEVICENAME_SIZE)

#define EEPROM_MQTT_SERVER_START        EEPROM_DEVICENAME_END
#define EEPROM_MQTT_SERVER_END          (EEPROM_MQTT_SERVER_START + EEPROM_MQTT_SERVER_SIZE)
#define EEPROM_MQTT_CLIENTID_START     EEPROM_MQTT_SERVER_END
#define EEPROM_MQTT_CLIENTID_END       (EEPROM_MQTT_CLIENTID_START + EEPROM_MQTT_CLIENTID_SIZE)
#define EEPROM_MQTT_USER_START          EEPROM_MQTT_CLIENTID_END
#define EEPROM_MQTT_USER_END            (EEPROM_MQTT_USER_START + EEPROM_MQTT_USER_SIZE)
#define EEPROM_MQTT_PASS_START          EEPROM_MQTT_USER_END
#define EEPROM_MQTT_PASS_END            (EEPROM_MQTT_PASS_START + EEPROM_MQTT_PASS_SIZE)
#define EEPROM_MQTT_TOPIC_START         EEPROM_MQTT_PASS_END
#define EEPROM_MQTT_TOPIC_END           (EEPROM_MQTT_TOPIC_START + EEPROM_MQTT_TOPIC_SIZE)
#define EEPROM_MQTT_FEED_PREFIX_START   EEPROM_MQTT_TOPIC_END
#define EEPROM_MQTT_FEED_PREFIX_END    (EEPROM_MQTT_FEED_PREFIX_START + EEPROM_MQTT_FEED_PREFIX_SIZE)

#define EEPROM_WWW_USER_START           EEPROM_MQTT_FEED_PREFIX_END
#define EEPROM_WWW_USER_END             (EEPROM_WWW_USER_START + EEPROM_WWW_USER_SIZE)
#define EEPROM_WWW_PASS_START           EEPROM_WWW_USER_END
#define EEPROM_WWW_PASS_END             (EEPROM_WWW_PASS_START + EEPROM_WWW_PASS_SIZE)

#define EEPROM_EMON_API_KEY_START       EEPROM_WWW_PASS_END
#define EEPROM_EMON_API_KEY_END         (EEPROM_EMON_API_KEY_START + EEPROM_EMON_API_KEY_SIZE)
#define EEPROM_EMON_SERVER_START        EEPROM_EMON_API_KEY_END
#define EEPROM_EMON_SERVER_END          (EEPROM_EMON_SERVER_START + EEPROM_EMON_SERVER_SIZE)
#define EEPROM_EMON_NODE_START          EEPROM_EMON_SERVER_END
#define EEPROM_EMON_NODE_END            (EEPROM_EMON_NODE_START + EEPROM_EMON_NODE_SIZE)
#define EEPROM_EMON_FINGERPRINT_START   EEPROM_EMON_NODE_END
#define EEPROM_EMON_FINGERPRINT_END    (EEPROM_EMON_FINGERPRINT_START + EEPROM_EMON_FINGERPRINT_SIZE)



// -------------------------------------------------------------------
// Reset EEPROM, wipes all settings
// -------------------------------------------------------------------
void ResetEEPROM() {
  //DEBUG.println("Erasing EEPROM");
  for (int i = 0; i < EEPROM_SIZE; ++i) {
    EEPROM.write(i, 0);
    //DEBUG.print("#");
  }
}

void EEPROM_read_string(int start, int count, String& val) {
  for (int i = 0; i < count; ++i) {
    byte c = EEPROM.read(start + i);
    if (c != 0 && c != 255) val += (char) c;
  }
}

void EEPROM_write_string(int start, int count, String val) {
  for (int i = 0; i < count; ++i) {
    if (i < val.length()) {
      EEPROM.write(start + i, val[i]);
    } else {
      EEPROM.write(start + i, 0);
    }
  }
}

// -------------------------------------------------------------------
// Load saved settings from EEPROM
// -------------------------------------------------------------------
void config_load_settings()
{
  EEPROM.begin(EEPROM_SIZE);

  // EmonCMS settings
  EEPROM_read_string(EEPROM_EMON_API_KEY_START, EEPROM_EMON_API_KEY_SIZE, emoncms_apikey);
  EEPROM_read_string(EEPROM_EMON_SERVER_START, EEPROM_EMON_SERVER_SIZE, emoncms_server);
  EEPROM_read_string(EEPROM_EMON_NODE_START, EEPROM_EMON_NODE_SIZE, emoncms_node);
  EEPROM_read_string(EEPROM_EMON_FINGERPRINT_START, EEPROM_EMON_FINGERPRINT_SIZE, emoncms_fingerprint);

  // MQTT settings
  EEPROM_read_string(EEPROM_MQTT_SERVER_START, EEPROM_MQTT_SERVER_SIZE, mqtt_server);
  EEPROM_read_string(EEPROM_MQTT_CLIENTID_START, EEPROM_MQTT_CLIENTID_SIZE, mqtt_clientid);
  EEPROM_read_string(EEPROM_MQTT_USER_START, EEPROM_MQTT_USER_SIZE, mqtt_user);
  EEPROM_read_string(EEPROM_MQTT_PASS_START, EEPROM_MQTT_PASS_SIZE, mqtt_pass);
  EEPROM_read_string(EEPROM_MQTT_TOPIC_START, EEPROM_MQTT_TOPIC_SIZE, mqtt_topic);
  EEPROM_read_string(EEPROM_MQTT_FEED_PREFIX_START, EEPROM_MQTT_FEED_PREFIX_SIZE, mqtt_feed_prefix);


  // Web server credentials
  EEPROM_read_string(EEPROM_WWW_USER_START, EEPROM_WWW_USER_SIZE, www_username);
  EEPROM_read_string(EEPROM_WWW_PASS_START, EEPROM_WWW_PASS_SIZE, www_password);

  // Load WiFi values
  EEPROM_read_string(EEPROM_ESID_START, EEPROM_ESID_SIZE, esid);
  EEPROM_read_string(EEPROM_EPASS_START, EEPROM_EPASS_SIZE, epass);

  // Load ALL values
  EEPROM_read_string(EEPROM_IP_0_START, EEPROM_IP_0_SIZE, eip_0);
  EEPROM_read_string(EEPROM_IP_1_START, EEPROM_IP_1_SIZE, eip_1);
  EEPROM_read_string(EEPROM_IP_2_START, EEPROM_IP_2_SIZE, eip_2);
  EEPROM_read_string(EEPROM_IP_3_START, EEPROM_IP_3_SIZE, eip_3);
  EEPROM_read_string(EEPROM_NETMASK_0_START, EEPROM_NETMASK_0_SIZE, enetmask_0);
  EEPROM_read_string(EEPROM_NETMASK_1_START, EEPROM_NETMASK_1_SIZE, enetmask_1);
  EEPROM_read_string(EEPROM_NETMASK_2_START, EEPROM_NETMASK_2_SIZE, enetmask_2);
  EEPROM_read_string(EEPROM_NETMASK_3_START, EEPROM_NETMASK_3_SIZE, enetmask_3);
  EEPROM_read_string(EEPROM_GATEWAY_0_START, EEPROM_GATEWAY_0_SIZE, egateway_0);
  EEPROM_read_string(EEPROM_GATEWAY_1_START, EEPROM_GATEWAY_1_SIZE, egateway_1);
  EEPROM_read_string(EEPROM_GATEWAY_2_START, EEPROM_GATEWAY_2_SIZE, egateway_2);
  EEPROM_read_string(EEPROM_GATEWAY_3_START, EEPROM_GATEWAY_3_SIZE, egateway_3);
  EEPROM_read_string(EEPROM_DNS_0_START, EEPROM_DNS_0_SIZE, edns_0);
  EEPROM_read_string(EEPROM_DNS_1_START, EEPROM_DNS_1_SIZE, edns_1);
  EEPROM_read_string(EEPROM_DNS_2_START, EEPROM_DNS_2_SIZE, edns_2);
  EEPROM_read_string(EEPROM_DNS_3_START, EEPROM_DNS_3_SIZE, edns_3);
  EEPROM_read_string(EEPROM_DHCP_START, EEPROM_DHCP_SIZE, edhcp);
  EEPROM_read_string(EEPROM_NTP_SERVER_START, EEPROM_NTP_SERVER_SIZE, entp_server);
  EEPROM_read_string(EEPROM_NTP_PERIOD_START, EEPROM_NTP_PERIOD_SIZE, entp_period);
  EEPROM_read_string(EEPROM_TIMEZONE_START, EEPROM_TIMEZONE_SIZE, etimezone);
  EEPROM_read_string(EEPROM_DAYLIGHT_START, EEPROM_DAYLIGHT_SIZE, edaylight);
  EEPROM_read_string(EEPROM_DEVICENAME_START, EEPROM_DEVICENAME_SIZE, edevicename);

  EEPROM.end();
}

void config_save_emoncms(String server, String node, String apikey, String fingerprint)
{
  EEPROM.begin(EEPROM_SIZE);

  emoncms_server = server;
  emoncms_node = node;
  emoncms_apikey = apikey;
  emoncms_fingerprint = fingerprint;

  // save apikey to EEPROM
  EEPROM_write_string(EEPROM_EMON_API_KEY_START, EEPROM_EMON_API_KEY_SIZE, emoncms_apikey);

  // save emoncms server to EEPROM max 45 characters
  EEPROM_write_string(EEPROM_EMON_SERVER_START, EEPROM_EMON_SERVER_SIZE, emoncms_server);

  // save emoncms node to EEPROM max 32 characters
  EEPROM_write_string(EEPROM_EMON_NODE_START, EEPROM_EMON_NODE_SIZE, emoncms_node);

  // save emoncms HTTPS fingerprint to EEPROM max 60 characters
  EEPROM_write_string(EEPROM_EMON_FINGERPRINT_START, EEPROM_EMON_FINGERPRINT_SIZE, emoncms_fingerprint);

  EEPROM.commit();
  EEPROM.end();
}

void config_save_mqtt(String server, String clientid, String user, String pass, String topic, String prefix)
{
  EEPROM.begin(EEPROM_SIZE);

  mqtt_server = server;
  mqtt_clientid = clientid;
  mqtt_user = user;
  mqtt_pass = pass;
  mqtt_topic = topic;
  mqtt_feed_prefix = prefix;


  // Save MQTT server max 45 characters
  EEPROM_write_string(EEPROM_MQTT_SERVER_START, EEPROM_MQTT_SERVER_SIZE, mqtt_server);

  // Save MQTT client id max 32 characters
  EEPROM_write_string(EEPROM_MQTT_CLIENTID_START, EEPROM_MQTT_CLIENTID_SIZE, mqtt_clientid);

  // Save MQTT username max 32 characters
  EEPROM_write_string(EEPROM_MQTT_USER_START, EEPROM_MQTT_USER_SIZE, mqtt_user);

  // Save MQTT pass max 32 characters
  EEPROM_write_string(EEPROM_MQTT_PASS_START, EEPROM_MQTT_PASS_SIZE, mqtt_pass);

  // Save MQTT topic max 32 characters
  EEPROM_write_string(EEPROM_MQTT_TOPIC_START, EEPROM_MQTT_TOPIC_SIZE, mqtt_topic);

  // Save MQTT topic separator max 10 characters
  EEPROM_write_string(EEPROM_MQTT_FEED_PREFIX_START, EEPROM_MQTT_FEED_PREFIX_SIZE, mqtt_feed_prefix);

  EEPROM.commit();
  EEPROM.end();
}

void config_save_admin(String user, String pass)
{
  EEPROM.begin(EEPROM_SIZE);

  www_username = user;
  www_password = pass;

  EEPROM_write_string(EEPROM_WWW_USER_START, EEPROM_WWW_USER_SIZE, user);
  EEPROM_write_string(EEPROM_WWW_PASS_START, EEPROM_WWW_PASS_SIZE, pass);

  EEPROM.commit();
  EEPROM.end();
}

void config_save_wifi(String qsid, String qpass)
{
  EEPROM.begin(EEPROM_SIZE);

  esid = qsid;
  epass = qpass;

  EEPROM_write_string(EEPROM_ESID_START, EEPROM_ESID_SIZE, qsid);
  EEPROM_write_string(EEPROM_EPASS_START, EEPROM_EPASS_SIZE, qpass);

  EEPROM.commit();
  EEPROM.end();
}

void config_save_all(
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
  String qdevicename
)
{
  EEPROM.begin(EEPROM_SIZE);

  esid = qsid;
  epass = qpass;
  eip_0 = qip_0;
  eip_1 = qip_1;
  eip_2 = qip_2;
  eip_3 = qip_3;
  enetmask_0 = qnetmask_0;
  enetmask_1 = qnetmask_1;
  enetmask_2 = qnetmask_2;
  enetmask_3 = qnetmask_3;
  egateway_0 = qgateway_0;
  egateway_1 = qgateway_1;
  egateway_2 = qgateway_2;
  egateway_3 = qgateway_3;
  edns_0 = qdns_0;
  edns_1 = qdns_1;
  edns_2 = qdns_2;
  edns_3 = qdns_3;
  edhcp = qdhcp;
  entp_server = qntp_server;
  entp_period = qntp_period;
  etimezone = qtimezone;
  edaylight = qdaylight;
  edevicename = qdevicename;

  EEPROM_write_string(EEPROM_ESID_START, EEPROM_ESID_SIZE, qsid);
  EEPROM_write_string(EEPROM_EPASS_START, EEPROM_EPASS_SIZE, qpass);
  EEPROM_write_string(EEPROM_IP_0_START, EEPROM_IP_0_SIZE, qip_0);
  EEPROM_write_string(EEPROM_IP_1_START, EEPROM_IP_1_SIZE, qip_1);
  EEPROM_write_string(EEPROM_IP_2_START, EEPROM_IP_2_SIZE, qip_2);
  EEPROM_write_string(EEPROM_IP_3_START, EEPROM_IP_3_SIZE, qip_3);
  EEPROM_write_string(EEPROM_NETMASK_0_START, EEPROM_NETMASK_0_SIZE, qnetmask_0);
  EEPROM_write_string(EEPROM_NETMASK_1_START, EEPROM_NETMASK_1_SIZE, qnetmask_1);
  EEPROM_write_string(EEPROM_NETMASK_2_START, EEPROM_NETMASK_2_SIZE, qnetmask_2);
  EEPROM_write_string(EEPROM_NETMASK_3_START, EEPROM_NETMASK_3_SIZE, qnetmask_3);
  EEPROM_write_string(EEPROM_GATEWAY_0_START, EEPROM_GATEWAY_0_SIZE, qgateway_0);
  EEPROM_write_string(EEPROM_GATEWAY_1_START, EEPROM_GATEWAY_1_SIZE, qgateway_1);
  EEPROM_write_string(EEPROM_GATEWAY_2_START, EEPROM_GATEWAY_2_SIZE, qgateway_2);
  EEPROM_write_string(EEPROM_GATEWAY_3_START, EEPROM_GATEWAY_3_SIZE, qgateway_3);
  EEPROM_write_string(EEPROM_DNS_0_START, EEPROM_DNS_0_SIZE, qdns_0);
  EEPROM_write_string(EEPROM_DNS_1_START, EEPROM_DNS_1_SIZE, qdns_1);
  EEPROM_write_string(EEPROM_DNS_2_START, EEPROM_DNS_2_SIZE, qdns_2);
  EEPROM_write_string(EEPROM_DNS_3_START, EEPROM_DNS_3_SIZE, qdns_3);
  EEPROM_write_string(EEPROM_DHCP_START, EEPROM_DHCP_SIZE, qdhcp);
  EEPROM_write_string(EEPROM_NTP_SERVER_START, EEPROM_NTP_SERVER_SIZE, qntp_server);
  EEPROM_write_string(EEPROM_NTP_PERIOD_START, EEPROM_NTP_PERIOD_SIZE, qntp_period);
  EEPROM_write_string(EEPROM_TIMEZONE_START, EEPROM_TIMEZONE_SIZE, qtimezone);
  EEPROM_write_string(EEPROM_DAYLIGHT_START, EEPROM_DAYLIGHT_SIZE, qdaylight);
  EEPROM_write_string(EEPROM_DEVICENAME_START, EEPROM_DEVICENAME_SIZE, qdevicename);

  EEPROM.commit();
  EEPROM.end();
}

void config_reset()
{
  EEPROM.begin(EEPROM_SIZE);

  ResetEEPROM();

  EEPROM.commit();
  EEPROM.end();
}


