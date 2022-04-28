
// #include <ESP8266WiFi.h>
#include "FSWebServerLib.h"
#include <ArduinoJson.h>
#include "modbus.h"
#include "mqtt.h"
#include "timehelper.h"
#include "gahe1progmem.h"
#include "config.h"
#include <Ticker.h>

// #define RELEASE

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

//////////////////// Port information ///////////////////
#define baud 9600

#define polling 50 // the scan rate; default value is 40
#define timeout 100
#define retry_count 30

// used to toggle the receive/transmit pin on the driver
#define TxEnablePin 5 // WEMOS_PIN_D1

#define PIN_LED 2 // WEMOS_PIN_D4

// The total amount of available memory on the master to store data
#define TOTAL_NO_OF_REGISTERS 10 * TOTAL_NO_OF_PACKETS

#define SOFTWARESERIAL

#if defined(SOFTWARESERIAL)
#include "SoftwareSerial.h"
uint8_t Rx = 14; // WEMOS_PIN_D5;
uint8_t Tx = 12; // WEMOS_PIN_D6;
#endif

// // This is the easiest way to create new packets
// // Add as many as you want. TOTAL_NO_OF_PACKETS
// // is automatically updated.
// enum
// {
//   PACKET1,
//   PACKET2,
//   PACKET3,
//   PACKET4,
//   PACKET5,
//   PACKET6,
//   PACKET7,
//   PACKET8,
//   PACKET9,
//   PACKET10,
//   PACKET11,
//   PACKET12,
//   PACKET13,
//   PACKET14,
//   TOTAL_NO_OF_PACKETS // leave this last entry
// };

// Create an array of Packets to be configured
Packet packets[TOTAL_NO_OF_PACKETS];
// packetPointer packet3 = &packets[PACKET3];

// Masters register array
unsigned int regs[TOTAL_NO_OF_REGISTERS];

// unsigned int readRegs[2];

// unsigned long currentMillis;
// long previousMillis = 0;
// long interval = 1000;

uint32_t oldSecond;

uint32_t oldrequestPACKET2;
uint32_t oldrequestPACKET3;
uint32_t oldrequestPACKET4;

// float oldAmpere;
// float oldWatt;

// unsigned long lastRequest = 0;
unsigned long lastmillisoldPacketConnection;

// char bufRequestsPACKET3[10];
// char bufSuccessful_requestsPACKET3[10];
// char bufFailed_requestsPACKET3[10];
// char bufException_errorsPACKET3[10];
// char bufConnectionPACKET3[10];

char bufVoltage[10];
char bufAmpere[10];
char bufWatt[10];
char bufVar[10];
char bufFrequency[10];
char bufPstkwh[10];
char bufPstkvarh[10];
char bufNgtkvarh[10];
char bufPowerFactor[10];
char bufApparentPower[10];
char bufUnk2[10];

// Set initial max Watt Threshold value.
// For 10 Amps MCB, suggested value is 2400 watt (around 11A) as the initial value.
uint16_t wattThreshold = 2400;
float currentThreshold = 11.4;

// Variable to store updated Watt Threshold value
// and set initial value equal to initial value of max watt Threshold.
uint16_t lastwattThreshold = wattThreshold;
float currentThreshold_old = currentThreshold;

// Buffer for Watt Threshold
char bufwattThreshold[10];
char bufCurrentThreshold[10];

unsigned long lastMsg = 0;
unsigned long lastEmonCMSMsg;
unsigned long lastThingSpeakMsg;
unsigned long lastMsg1s = 0;
unsigned long lastMsg10s = 0;
unsigned long lastTimer16s = 0;

#if defined(SOFTWARESERIAL)
SoftwareSerial swSer;
#endif

Ticker ticker1000msModbus;
Ticker ticker60sModbus;

bool tick1000msModbus;
bool tick60sModbus;
uint32_t numModbusSuccessReq = 0;

void Ticking1000ms()
{
  tick1000msModbus = true;
}

void Ticking60s()
{
  tick60sModbus = true;
}

void modbus_setup()
{

  ticker1000msModbus.attach(1, Ticking1000ms);
  ticker60sModbus.attach(60, Ticking60s);

#if defined(SOFTWARESERIAL)
  swSer.begin(baud, SWSERIAL_8N1, Rx, Tx, false, 95, 11);
  // swSer.begin(baud, SWSERIAL_8N1, Rx, Tx, false);
  // begin(baud, config, rxPin, txPin, m_invert);
  // swSer.begin(baud, SWSERIAL_8N1, 12, 12, false, 256);
#endif

  // Initialize each packet

  /*
    Read here if you want to know more about electricity / glossary term below
    https://www.progress-energy.com/assets/www/docs/business/power-factor-how-effects-bill.pdf
  */

  // modbus_construct(&packets[PACKET1], 4, READ_HOLDING_REGISTERS, 0, 1, 0);   // DMM

  modbus_construct(&packets[PACKET1], 3, READ_INPUT_REGISTERS, 10, 2, 0); // read Present Voltage (Volt)
  modbus_construct(&packets[PACKET2], 3, READ_INPUT_REGISTERS, 22, 2, 2); // read Present Current (Ampere)
  modbus_construct(&packets[PACKET3], 3, READ_INPUT_REGISTERS, 28, 2, 4); // read Present True Power (Watt or kW)
  modbus_construct(&packets[PACKET4], 3, READ_INPUT_REGISTERS, 50, 2, 6); // read Accumulative Positive kWh

  // modbus_construct(&packets[PACKET1], 3, READ_INPUT_REGISTERS, 10, 2, 0);    // read Present Voltage (Volt)
  // modbus_construct(&packets[PACKET2], 3, READ_INPUT_REGISTERS, 22, 2, 2);    // read Present Current (Ampere)
  // modbus_construct(&packets[PACKET3], 3, READ_INPUT_REGISTERS, 28, 2, 4);    // read Present True Power (Watt or kW)
  // modbus_construct(&packets[PACKET4], 3, READ_INPUT_REGISTERS, 36, 2, 6);    // read Present Reactive Power (VAr or kVAr)
  // modbus_construct(&packets[PACKET5], 3, READ_INPUT_REGISTERS, 48, 2, 8);    // read Present Frequency (Hz)
  // modbus_construct(&packets[PACKET6], 3, READ_INPUT_REGISTERS, 50, 2, 10);   // read Accumulative Positive kWh
  // modbus_construct(&packets[PACKET7], 3, READ_INPUT_REGISTERS, 54, 2, 12);   // read Accumulative Positive kVarh
  // modbus_construct(&packets[PACKET8], 3, READ_INPUT_REGISTERS, 56, 2, 14);   // read Accumulative Negative kVarh
  // modbus_construct(&packets[PACKET9], 3, READ_INPUT_REGISTERS, 58, 2, 16);   // read Power Factor (Cos φ)
  // modbus_construct(&packets[PACKET10], 3, READ_INPUT_REGISTERS, 64, 2, 18);  // read Present Apparent Power (VA or kVA)
  // modbus_construct(&packets[PACKET11], 3, READ_INPUT_REGISTERS, 70, 2, 20);  // ???
  // modbus_construct(&packets[PACKET12], 3, READ_INPUT_REGISTERS, 544, 1, 22); // read Year and month ??
  // modbus_construct(&packets[PACKET13], 3, READ_INPUT_REGISTERS, 545, 1, 23); // read Day & Hour ??
  // modbus_construct(&packets[PACKET14], 3, READ_INPUT_REGISTERS, 546, 1, 24); // read Minute & Second

  // Initialize the Modbus Finite State Machine
  // modbus_configure(&Serial, baud, SERIAL_8N1, timeout, polling, retry_count, TxEnablePin, packets, TOTAL_NO_OF_PACKETS, regs);
  // modbus_configure(&Serial1, baud, SERIAL_8N1, timeout, polling, retry_count, TxEnablePin, packets, TOTAL_NO_OF_PACKETS, regs);
#if defined(SOFTWARESERIAL)
  modbus_configure(&swSer, baud, SERIAL_8N1, timeout, polling, retry_count, TxEnablePin, packets, TOTAL_NO_OF_PACKETS, regs);
#else
  modbus_configure(&Serial, baud, SERIAL_8N1, timeout, polling, retry_count, TxEnablePin, packets, TOTAL_NO_OF_PACKETS, regs);
#endif

  oldrequestPACKET2 = packets[PACKET2].successful_requests;
  oldrequestPACKET3 = packets[PACKET3].successful_requests;
  oldrequestPACKET4 = packets[PACKET4].successful_requests;
}

void modbus_loop()
{

  // if (!tick1000msModbus)
  //   return;
  // else
  //   tick1000msModbus = false;

  /* Reset the ESP8266 if no Modbus connections for more than 5 minutes */

  if (packets[PACKET1].connection == 1 && packets[PACKET2].connection == 1 && packets[PACKET3].connection == 1 && packets[PACKET4].connection == 1)
  {
    lastmillisoldPacketConnection = millis();
    // oldWatt = Watt;
  }
  else if (millis() - lastmillisoldPacketConnection > 300000UL)
  {
    // while (true);
    esp_restart();
    return;
  }
  else
  {
    return;
  }

  static bool ledOn = 0;

  // modbus_construct(&packets[PACKET1], 3, READ_INPUT_REGISTERS, 10, 2, 0);    // read Present Voltage (Volt)
  // modbus_construct(&packets[PACKET2], 3, READ_INPUT_REGISTERS, 22, 2, 2);    // read Present Current (Ampere)
  // modbus_construct(&packets[PACKET3], 3, READ_INPUT_REGISTERS, 28, 2, 4);    // read Present True Power (Watt or kW)
  // modbus_construct(&packets[PACKET4], 3, READ_INPUT_REGISTERS, 36, 2, 6);    // read Present Reactive Power (VAr or kVAr)
  // modbus_construct(&packets[PACKET5], 3, READ_INPUT_REGISTERS, 48, 2, 8);    // read Present Frequency (Hz)
  // modbus_construct(&packets[PACKET6], 3, READ_INPUT_REGISTERS, 50, 2, 10);   // read Accumulative Positive kWh
  // modbus_construct(&packets[PACKET7], 3, READ_INPUT_REGISTERS, 54, 2, 12);   // read Accumulative Positive kVarh
  // modbus_construct(&packets[PACKET8], 3, READ_INPUT_REGISTERS, 56, 2, 14);   // read Accumulative Negative kVarh
  // modbus_construct(&packets[PACKET9], 3, READ_INPUT_REGISTERS, 58, 2, 16);   // read Power Factor (Cos φ)
  // modbus_construct(&packets[PACKET10], 3, READ_INPUT_REGISTERS, 64, 2, 18);  // read Present Apparent Power (VA or kVA)
  // modbus_construct(&packets[PACKET11], 3, READ_INPUT_REGISTERS, 70, 2, 20);  // ???
  // modbus_construct(&packets[PACKET12], 3, READ_INPUT_REGISTERS, 544, 1, 22); // read Year and month ??
  // modbus_construct(&packets[PACKET13], 3, READ_INPUT_REGISTERS, 545, 1, 23); // read Day & Hour ??
  // modbus_construct(&packets[PACKET14], 3, READ_INPUT_REGISTERS, 546, 1, 24); // read Minute & Second

  // Check if Watt Threshold has been changed or buffer is empty.
  // If yes, update the last watt Threshold and its buffer value
  if (wattThreshold != lastwattThreshold || strlen(bufwattThreshold) == 0)
  {
    lastwattThreshold = wattThreshold;
    dtostrf(wattThreshold, 0, 0, bufwattThreshold);
  }

  if (currentThreshold != currentThreshold_old || strlen(bufCurrentThreshold) == 0)
  {
    currentThreshold_old = currentThreshold;
    dtostrf(currentThreshold, 0, 1, bufCurrentThreshold);
  }

  numModbusSuccessReq++;

  // ledOn = !ledOn;
  // digitalWrite(PIN_LED, ledOn);

  if (packets[PACKET2].successful_requests != oldrequestPACKET2 &&
      packets[PACKET3].successful_requests != oldrequestPACKET3 &&
      packets[PACKET4].successful_requests != oldrequestPACKET4)
  {
    // update old values
    oldrequestPACKET2 = packets[PACKET2].successful_requests;
    oldrequestPACKET3 = packets[PACKET3].successful_requests;
    oldrequestPACKET4 = packets[PACKET4].successful_requests;
  }
  else
  {
    return;
  }

  float Voltage;
  float Ampere;
  float Watt;
  // float Var;
  // float Frequency;
  float Pstkwh;
  // float Pstkvarh;
  // float Ngtkvarh;
  // float PowerFactor;
  // float ApparentPower;
  // float Unk2;

  unsigned long temp;
  unsigned long *p = &temp;

  // float Voltage;
  temp = (unsigned long)regs[0] << 16 | regs[1];
  Voltage = *(float *)p;
  Voltage = Voltage - 1.9;

  // float Ampere;
  unsigned long tempCurrent = (unsigned long)regs[2] << 16 | regs[3];
  unsigned long *pCurrent = &tempCurrent;
  Ampere = *(float *)pCurrent;

  // float Watt;
  temp = (unsigned long)regs[4] << 16 | regs[5];
  Watt = *(float *)p;

  // //float Var;
  // temp = (unsigned long)regs[6] << 16 | regs[7];
  // Var = *(float *)p;

  // //float Frequency;
  // temp = (unsigned long)regs[8] << 16 | regs[9];
  // Frequency = *(float *)p;

  // float Pstkwh;
  temp = (unsigned long)regs[6] << 16 | regs[7];
  Pstkwh = *(float *)p;

  // //float Pstkvarh;
  // temp = (unsigned long)regs[12] << 16 | regs[13];
  // Pstkvarh = *(float *)p;

  // //float Ngtkvarh;
  // temp = (unsigned long)regs[14] << 16 | regs[15];
  // Ngtkvarh = *(float *)p;

  // //float PowerFactor;
  // temp = (unsigned long)regs[16] << 16 | regs[17];
  // PowerFactor = *(float *)p;

  // //float ApparentPower;
  // temp = (unsigned long)regs[18] << 16 | regs[19];
  // ApparentPower = *(float *)p;

  // //float Unk2;
  // temp = (unsigned long)regs[20] << 16 | regs[21];
  // Unk2 = *(float *)p;


  static unsigned int count_avg;
  static float voltage_avg;
  static float ampere_avg;
  static float watt_avg;
  static float pstkwh_avg;

  count_avg++;
  voltage_avg = voltage_avg + Voltage;
  ampere_avg = ampere_avg + Ampere;
  watt_avg = watt_avg + Watt;
  pstkwh_avg = pstkwh_avg + Pstkwh;


  dtostrf(Voltage, 0, 1, bufVoltage); /* Voltage */
  dtostrf(Ampere, 0, 2, bufAmpere);   /* Ampere */
  dtostrf(Watt, 0, 1, bufWatt);       /* Wattage */
  // dtostrf(Var, 0, 1, bufVar);                     /* Positive Var */
  // dtostrf(Frequency, 0, 1, bufFrequency);         /* Frequency Hz */
  dtostrf(Pstkwh, 0, 2, bufPstkwh); /* Positive kWh */
  // dtostrf(Pstkvarh, 0, 1, bufPstkvarh);           /* Positive kVarh */
  // dtostrf(Ngtkvarh, 0, 1, bufNgtkvarh);           /* Negative kVarh */
  // dtostrf(PowerFactor, 0, 1, bufPowerFactor);     /* Power Factor */
  // dtostrf(ApparentPower, 0, 1, bufApparentPower); /* Apparent Power */
  // dtostrf(Unk2, 0, 1, bufUnk2);                   /* Unk2 */

  if (tick1000msModbus)
  {
    ledOn = !ledOn;
    digitalWrite(PIN_LED, false);

    tick1000msModbus = false;

    // if (numModbusSuccessReq <= 60)
    //   return;

    StaticJsonDocument<512> root;

    root[FPSTR(pgm_voltage)] = atof(bufVoltage);
    root[FPSTR(pgm_ampere)] = atof(bufAmpere);
    root[FPSTR(pgm_watt)] = atof(bufWatt);
    // root[FPSTR(pgm_var)] = bufVar;
    // root[FPSTR(pgm_frequency)] = bufFrequency;
    root[FPSTR(pgm_pstkwh)] = atof(bufPstkwh);
    // root[FPSTR(pgm_pstkvarh)] = bufPstkvarh;
    // root[FPSTR(pgm_ngtkvarh)] = bufNgtkvarh;
    // root[FPSTR(pgm_powerfactor)] = bufPowerFactor;
    // root[FPSTR(pgm_apparentpower)] = bufApparentPower;
    // root[FPSTR(pgm_unk2)] = bufUnk2;
    root["heap"] = ESP.getFreeHeap();

    size_t len = measureJson(root);

    char buf[len + 1];
    serializeJson(root, buf, sizeof(buf));

    // if (ws.hasClient(clientID))
    // {
    //   ws.textAll(buf);
    // }

    if (mqttClient.connected())
    {
      char bufFullTopic[64];
      strlcpy(bufFullTopic, ESPHTTPServer._config.hostname, sizeof(bufFullTopic) / sizeof(bufFullTopic[0]));
      strncat(bufFullTopic, "/meterreading/1s", sizeof(bufFullTopic) / sizeof(bufFullTopic[0]));
      mqttClient.publish(
          bufFullTopic, // topic
          0,            // qos
          0,            // retain
          buf           // payload
      );

      if (tick60sModbus) {
        tick60sModbus = false;
        char bufFullTopic[64];
        strlcpy(bufFullTopic, ESPHTTPServer._config.hostname, sizeof(bufFullTopic) / sizeof(bufFullTopic[0]));
        strncat(bufFullTopic, "/meterreading/60s", sizeof(bufFullTopic) / sizeof(bufFullTopic[0]));
        mqttClient.publish(
            bufFullTopic, // topic
            0,            // qos
            0,            // retain
            buf           // payload
        );
      }
    }
  }

  ledOn = !ledOn;
  digitalWrite(PIN_LED, true);

  // Serial.print("requests: ");
  // Serial.println(packets[PACKET1].requests);
  // Serial.print("successful_requests: ");
  // Serial.println(packets[PACKET1].successful_requests);
  // Serial.print("failed_requests: ");
  // Serial.println(packets[PACKET1].failed_requests);
  // Serial.print("exception_errors: ");
  // Serial.println(packets[PACKET1].exception_errors);
  // Serial.print("connection: ");
  // Serial.println(packets[PACKET1].connection);
}

void modbus_loop_DMM()
{
  static bool ledOn = 0;

  modbus_construct(&packets[PACKET1], 4, READ_HOLDING_REGISTERS, 0, 1, 0); // read Present Voltage (Volt)

  bool success = 0;
  static uint32_t numSuccessReq_old = 0;
  uint32_t numSuccessReq = packets[PACKET1].successful_requests;

  static uint32_t numFailedReq_old = 0;
  uint32_t numFailedReq = packets[PACKET1].failed_requests;

  if (numSuccessReq != numSuccessReq_old || numFailedReq != numFailedReq_old)
  {
    if (numSuccessReq != numSuccessReq_old)
    {
      // update old values
      numSuccessReq_old = numSuccessReq;
      // DEBUGLOG("successful_requests [PACKET1]: %d\r\n", packets[PACKET1].successful_requests);
      success = true;
    }

    if (numFailedReq != numFailedReq_old)
    {
      // update old values
      numFailedReq_old = numFailedReq;
      DEBUGLOG("failed_requests [PACKET1]: %d\r\n", packets[PACKET1].failed_requests);

      digitalWrite(2, HIGH);
    }
  }
  else
  {
    return;
  }

  if (!success)
    return;

  ledOn = !ledOn;
  digitalWrite(2, ledOn);

  unsigned long DMM;

  unsigned long temp;

  temp = (unsigned long)regs[0];
  DMM = temp;
  static uint32_t DMM_old = 0;
  if (DMM_old != DMM)
  {
    DMM_old = DMM;
    float DMM_float;
    DMM_float = DMM / 2.0;
    DEBUGLOG("DMM: %.1f\r\n", DMM_float);
  }
}
