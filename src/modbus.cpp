
#include <ESP8266WiFi.h>

#include "modbus.h"
#include "mqtt.h"
#include "timehelper.h"

//////////////////// Port information ///////////////////
#define baud 9600

#define timeout 200
#define polling 55 // the scan rate; default value is 40
#define retry_count 0

// used to toggle the receive/transmit pin on the driver
#define TxEnablePin ESP_PIN_5 //D1 wemos

//#define LED 13

// The total amount of available memory on the master to store data
#define TOTAL_NO_OF_REGISTERS 220

#if defined(SOFTWARESERIAL)
#include "SoftwareSerial.h"
uint8_t Rx = 1;
uint8_t Tx = 3;
SoftwareSerial swSer(Rx, Tx, false, 256);
#endif

// This is the easiest way to create new packets
// Add as many as you want. TOTAL_NO_OF_PACKETS
// is automatically updated.
enum
{
  PACKET1,
  PACKET2,
  PACKET3,
  PACKET4,
  PACKET5,
  PACKET6,
  PACKET7,
  PACKET8,
  PACKET9,
  PACKET10,
  PACKET11,
  PACKET12,
  PACKET13,
  PACKET14,
  TOTAL_NO_OF_PACKETS // leave this last entry
};

// Create an array of Packets to be configured
Packet packets[TOTAL_NO_OF_PACKETS];
//packetPointer packet1 = &packets[PACKET1];

// Masters register array
unsigned int regs[TOTAL_NO_OF_REGISTERS];

//unsigned int readRegs[2];

//unsigned long currentMillis;
//long previousMillis = 0;
//long interval = 1000;

uint32_t oldSecond;

uint32_t oldrequestPACKET2;
uint32_t oldrequestPACKET3;
uint32_t oldrequestPACKET6;

float oldAmpere;
float oldWatt;

unsigned long lastRequest = 0;
unsigned long lastmillisoldPacketConnection;

char bufRequestsPACKET3[10];
char bufSuccessful_requestsPACKET3[10];
char bufFailed_requestsPACKET3[10];
char bufException_errorsPACKET3[10];
char bufConnectionPACKET3[10];

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

// Variable to store updated Watt Threshold value
// and set initial value equal to initial value of max watt Threshold.
uint16_t lastwattThreshold = wattThreshold;

// Buffer for Watt Threshold
char bufwattThreshold[10];

unsigned long lastMsg = 0;
unsigned long lastEmonCMSMsg;
unsigned long lastThingSpeakMsg;
unsigned long lastMsg1s = 0;
unsigned long lastMsg10s = 0;
unsigned long lastTimer16s = 0;

//SoftwareSerial swSer(3, 1, false, 256);

void modbus_setup()
{

#if defined(SOFTWARESERIAL)
  swSer.begin(baud);
#endif

  // Initialize each packet

  /*
    Read here if you want to know more about electricity / glossary term below
    https://www.progress-energy.com/assets/www/docs/business/power-factor-how-effects-bill.pdf
  */

  modbus_construct(&packets[PACKET1], 3, READ_INPUT_REGISTERS, 10, 2, 0);    // read Present Voltage (Volt)
  modbus_construct(&packets[PACKET2], 3, READ_INPUT_REGISTERS, 22, 2, 2);    // read Present Current (Ampere)
  modbus_construct(&packets[PACKET3], 3, READ_INPUT_REGISTERS, 28, 2, 4);    // read Present True Power (Watt or kW)
  modbus_construct(&packets[PACKET4], 3, READ_INPUT_REGISTERS, 36, 2, 6);    // read Present Reactive Power (VAr or kVAr)
  modbus_construct(&packets[PACKET5], 3, READ_INPUT_REGISTERS, 48, 2, 8);    // read Present Frequency (Hz)
  modbus_construct(&packets[PACKET6], 3, READ_INPUT_REGISTERS, 50, 2, 10);   // read Accumulative Positive kWh
  modbus_construct(&packets[PACKET7], 3, READ_INPUT_REGISTERS, 54, 2, 12);   // read Accumulative Positive kVarh
  modbus_construct(&packets[PACKET8], 3, READ_INPUT_REGISTERS, 56, 2, 14);   // read Accumulative Negative kVarh
  modbus_construct(&packets[PACKET9], 3, READ_INPUT_REGISTERS, 58, 2, 16);   // read Power Factor (Cos φ)
  modbus_construct(&packets[PACKET10], 3, READ_INPUT_REGISTERS, 64, 2, 18);  // read Present Apparent Power (VA or kVA)
  modbus_construct(&packets[PACKET11], 3, READ_INPUT_REGISTERS, 70, 2, 20);  // ???
  modbus_construct(&packets[PACKET12], 3, READ_INPUT_REGISTERS, 544, 1, 22); // read Year and month ??
  modbus_construct(&packets[PACKET13], 3, READ_INPUT_REGISTERS, 545, 1, 23); // read Day & Hour ??
  modbus_construct(&packets[PACKET14], 3, READ_INPUT_REGISTERS, 546, 1, 24); // read Minute & Second

  // Initialize the Modbus Finite State Machine
  //modbus_configure(&Serial, baud, SERIAL_8N1, timeout, polling, retry_count, TxEnablePin, packets, TOTAL_NO_OF_PACKETS, regs);
  //modbus_configure(&Serial1, baud, SERIAL_8N1, timeout, polling, retry_count, TxEnablePin, packets, TOTAL_NO_OF_PACKETS, regs);
#if defined(SOFTWARESERIAL)
  modbus_configure(&swSer, baud, SERIAL_8N1, timeout, polling, retry_count, TxEnablePin, packets, TOTAL_NO_OF_PACKETS, regs);
#else
  modbus_configure(&Serial, baud, SERIAL_8N1, timeout, polling, retry_count, TxEnablePin, packets, TOTAL_NO_OF_PACKETS, regs);
#endif

  oldrequestPACKET2 = packets[PACKET2].successful_requests;
  oldrequestPACKET3 = packets[PACKET3].successful_requests;
  oldrequestPACKET6 = packets[PACKET6].successful_requests;
}

void modbus_loop()
{
  if (!tick1000ms)
  {
    return;
  }

  /* Reset the ESP8266 if no Modbus connections for more than 2 minutes */

  if (packets[PACKET1].connection == 1 && packets[PACKET2].connection == 1 && packets[PACKET3].connection == 1 && packets[PACKET6].connection == 1)
  {
    lastmillisoldPacketConnection = millis();
    // oldWatt = Watt;
  }
  else if (millis() - lastmillisoldPacketConnection > 120000)
  {
    // while (true);
    return;
  }

  //  modbus_construct(&packets[PACKET1], 3, READ_INPUT_REGISTERS, 10, 2, 0);    // read Present Voltage (Volt)
  //  modbus_construct(&packets[PACKET2], 3, READ_INPUT_REGISTERS, 22, 2, 2);   // read Present Current (Ampere)
  //  modbus_construct(&packets[PACKET3], 3, READ_INPUT_REGISTERS, 28, 2, 4);   // read Present True Power (Watt or kW)
  //  modbus_construct(&packets[PACKET4], 3, READ_INPUT_REGISTERS, 36, 2, 6);   // read Present Reactive Power (VAr or kVAr)
  //  modbus_construct(&packets[PACKET5], 3, READ_INPUT_REGISTERS, 48, 2, 8);   // read Present Frequency (Hz)
  //  modbus_construct(&packets[PACKET6], 3, READ_INPUT_REGISTERS, 50, 2, 10);  // read Accumulative Positive kWh
  //  modbus_construct(&packets[PACKET7], 3, READ_INPUT_REGISTERS, 54, 2, 12);  // read Accumulative Positive kVarh
  //  modbus_construct(&packets[PACKET8], 3, READ_INPUT_REGISTERS, 56, 2, 14);  // read Accumulative Negative kVarh
  //  modbus_construct(&packets[PACKET9], 3, READ_INPUT_REGISTERS, 58, 2, 16);  // read Power Factor (Cos φ)
  //  modbus_construct(&packets[PACKET10], 3, READ_INPUT_REGISTERS, 64, 2, 18);  // read Present Apparent Power (VA or kVA)
  //  modbus_construct(&packets[PACKET11], 3, READ_INPUT_REGISTERS, 70, 2, 20);  // ???
  //  modbus_construct(&packets[PACKET12], 3, READ_INPUT_REGISTERS, 544, 1, 22);   // read Year and month ??
  //  modbus_construct(&packets[PACKET13], 3, READ_INPUT_REGISTERS, 545, 1, 23);  // read Day & Hour ??
  //  modbus_construct(&packets[PACKET14], 3, READ_INPUT_REGISTERS, 546, 1, 24);  // read Minute & Second

  //update values if new request based on Watt request [PACKET3]
  static uint32_t numReq_old;
  if (packets[PACKET3].requests != numReq_old)
  {

    //update old values
    numReq_old = packets[PACKET3].requests;

    //convert packet status to char array for later use
    dtostrf(packets[PACKET3].requests, 0, 0, bufRequestsPACKET3);
    dtostrf(packets[PACKET3].successful_requests, 0, 0, bufSuccessful_requestsPACKET3);
    dtostrf(packets[PACKET3].failed_requests, 0, 0, bufFailed_requestsPACKET3);
    dtostrf(packets[PACKET3].exception_errors, 0, 0, bufException_errorsPACKET3);
    dtostrf(packets[PACKET3].connection, 0, 0, bufConnectionPACKET3);
  }
  else
  {
    return;
  }

  if (packets[PACKET2].successful_requests != oldrequestPACKET2 &&
      packets[PACKET3].successful_requests != oldrequestPACKET3 &&
      packets[PACKET6].successful_requests != oldrequestPACKET6)
  {
    //update old values
    oldrequestPACKET2 = packets[PACKET2].successful_requests;
    oldrequestPACKET3 = packets[PACKET3].successful_requests;
    oldrequestPACKET6 = packets[PACKET6].successful_requests;

    float Voltage;
    float Ampere;
    float Watt;
    float Var;
    float Frequency;
    float Pstkwh;
    float Pstkvarh;
    float Ngtkvarh;
    float PowerFactor;
    float ApparentPower;
    float Unk2;

    unsigned long temp;

    //float Voltage;
    temp = (unsigned long)regs[0] << 16 | regs[1];
    Voltage = *(float *)&temp;

    //float Ampere;
    temp = (unsigned long)regs[2] << 16 | regs[3];
    Ampere = *(float *)&temp;

    //float Watt;
    temp = (unsigned long)regs[4] << 16 | regs[5];
    Watt = *(float *)&temp;

    if (mqttClient.connected())
    {
      char duar[20];
      dtostrf(temp, 0, 0, duar);
      mqttClient.publish("gila", 0, 0, duar);
    }

    // unsigned long ulWatt = temp;

    //float Var;
    temp = (unsigned long)regs[6] << 16 | regs[7];
    Var = *(float *)&temp;

    //float Frequency;
    temp = (unsigned long)regs[8] << 16 | regs[9];
    Frequency = *(float *)&temp;

    //float Pstkwh;
    temp = (unsigned long)regs[10] << 16 | regs[11];
    Pstkwh = *(float *)&temp;

    //float Pstkvarh;
    temp = (unsigned long)regs[12] << 16 | regs[13];
    Pstkvarh = *(float *)&temp;

    //float Ngtkvarh;
    temp = (unsigned long)regs[14] << 16 | regs[15];
    Ngtkvarh = *(float *)&temp;

    //float PowerFactor;
    temp = (unsigned long)regs[16] << 16 | regs[17];
    PowerFactor = *(float *)&temp;

    //float ApparentPower;
    temp = (unsigned long)regs[18] << 16 | regs[19];
    ApparentPower = *(float *)&temp;

    //float Unk2;
    temp = (unsigned long)regs[20] << 16 | regs[21];
    Unk2 = *(float *)&temp;

    /* Convert all floats into string prior to publish to MQTT broker */

    dtostrf(Voltage, 0, 1, bufVoltage);             /* Voltage */
    dtostrf(Ampere, 0, 2, bufAmpere);               /* Ampere */
    dtostrf(Watt, 0, 1, bufWatt);                   /* Wattage */
    dtostrf(Var, 0, 1, bufVar);                     /* Positive Var */
    dtostrf(Frequency, 0, 1, bufFrequency);         /* Frequency Hz */
    dtostrf(Pstkwh, 0, 1, bufPstkwh);               /* Positive kWh */
    dtostrf(Pstkvarh, 0, 1, bufPstkvarh);           /* Positive kVarh */
    dtostrf(Ngtkvarh, 0, 1, bufNgtkvarh);           /* Negative kVarh */
    dtostrf(PowerFactor, 0, 1, bufPowerFactor);     /* Power Factor */
    dtostrf(ApparentPower, 0, 1, bufApparentPower); /* Apparent Power */
    dtostrf(Unk2, 0, 1, bufUnk2);                   /* Unk2 */
  }
  else
  {
    return;
  }

  // Check if Watt Threshold has been changed or buffer is empty.
  // If yes, update the last watt Threshold and its buffer value
  if (wattThreshold != lastwattThreshold || strlen(bufwattThreshold) == 0)
  {
    lastwattThreshold = wattThreshold;
    dtostrf(wattThreshold, 0, 0, bufwattThreshold);
  }

  // -------------------------------------------------------------------
  // PROCESS SPIFFS & ArduinoJson
  // -------------------------------------------------------------------

  // return;

  //process only if websocket has client or MQTT is connected
  if (WiFi.status() == WL_CONNECTED || (ws.hasClient(num) || mqttClient.connected()))
  {
  }
  else
  {
    return;
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    return;
  }

  // -------------------------------------------------------------------
  // 20 SECOND
  // -------------------------------------------------------------------

  static unsigned long lastTimer15s = millis();
  if (millis() - lastTimer15s >= 20000)
  {

    lastTimer15s = millis(); //  Update time

    static bool updateToCloud;
    if (packets[PACKET6].successful_requests >= 10)
    {
      updateToCloud = true;
    }
    if (updateToCloud && WiFi.status() == WL_CONNECTED)
    {
      runAsyncClientEmoncms();
      runAsyncClientThingspeak();
    }
  }

  if (!mqttClient.connected())
  {
    return;
  }

  // -------------------------------------------------------------------
  // 1 SECOND ..../meterreading/1s
  // -------------------------------------------------------------------

  if (mqttClient.connected())
  {
    StaticJsonBuffer<1024> jsonBuffer;
    JsonObject &root = jsonBuffer.createObject();

    root[FPSTR(pgm_voltage)] = bufVoltage;
    root[FPSTR(pgm_ampere)] = bufAmpere;
    root[FPSTR(pgm_watt)] = bufWatt;
    root[FPSTR(pgm_var)] = bufVar;
    root[FPSTR(pgm_frequency)] = bufFrequency;
    root[FPSTR(pgm_pstkwh)] = bufPstkwh;
    root[FPSTR(pgm_pstkvarh)] = bufPstkvarh;
    root[FPSTR(pgm_ngtkvarh)] = bufNgtkvarh;
    root[FPSTR(pgm_powerfactor)] = bufPowerFactor;
    root[FPSTR(pgm_apparentpower)] = bufApparentPower;
    root[FPSTR(pgm_unk2)] = bufUnk2;

    size_t len = root.measureLength();

    char buf[len];
    root.printTo(buf, len + 1);

    mqttClient.publish("ESP13579541/meterreading/1s", 2, 0, buf);
  }

  //publish watt and and ampere readings in higher publish rate (publish every 1 second)
  //if watt is above wattThreshold

  unsigned long timer1;

  if (atof(bufWatt) > wattThreshold)
  {
    timer1 = 1000;
    oldWatt = atof(bufWatt);
  }
  else
  {
    timer1 = 10000;
    if (oldWatt > wattThreshold)
    {
      oldWatt = atof(bufWatt);
      //const char* wattTopic = PSTR("/rumah/sts/kwh1/watt");
      //const char* ampereTopic = PSTR("/rumah/sts/kwh1/ampere");
      char wattTopic[] = "/rumah/sts/kwh1/watt";
      char ampereTopic[] = "/rumah/sts/kwh1/ampere";

      mqttClient.publish(wattTopic, 0, 0, bufWatt);
      mqttClient.publish(ampereTopic, 0, 0, bufAmpere);
    }
  }

  File pubSubJsonFile = SPIFFS.open(PUBSUBJSON_FILE, "r");
  if (!pubSubJsonFile)
  {
    PRINT("Failed to open PUBSUBJSON_FILE file\r\n");
    pubSubJsonFile.close();
    return;
  }

  static uint16_t size = pubSubJsonFile.size();
  PRINT("PUBSUBJSON_FILE file size: %d bytes\r\n", size);
  if (size > 1280)
  {
    PRINT("WARNING, file size maybe too large\r\n");
    pubSubJsonFile.close();
    return;
  }

  StaticJsonBuffer<1280> jsonBuffer;
  // DynamicJsonBuffer jsonBuffer;
  JsonObject &root = jsonBuffer.parseObject(pubSubJsonFile);

  //close the file, save your memory, keep healthy :-)
  pubSubJsonFile.close();

  if (!root.success())
  {
    PRINT("Failed to parse PUBSUBJSON_FILE file\r\n");
    return;
  }

  JsonArray &pub_param = root[FPSTR(pgm_pub_param)];

  static unsigned long lastTimer;
  if (millis() - lastTimer >= timer1)
  {
    lastTimer = millis(); //  Update time

    uint8_t lenBaseTopic = strlen(root[FPSTR(pgm_pub_default_basetopic)]);

    const char *bt = root[FPSTR(pgm_pub_default_basetopic)];

    for (int i = 0; i < 11; i++)
    {

      char cat[lenBaseTopic + 1];

      strlcpy(cat, bt, sizeof(cat) / sizeof(cat[0]));

      const char *pr = pub_param[i];

      strcat(cat, pr);

      //          if (ws.hasClient(num)) {
      //            ws.text(num, cat);
      //          }

      if (i == 0)
      {
        if (!isnan(atof(bufVoltage)) || atoi(bufVoltage) == 0)
        {
          mqttClient.publish(cat, 0, 0, bufVoltage);
        }
      }
      if (i == 1)
      {
        mqttClient.publish(cat, 0, 0, bufAmpere);
      }
      if (i == 2)
      {
        mqttClient.publish(cat, 0, 0, bufWatt);
      }
      if (i == 3)
      {
        mqttClient.publish(cat, 0, 0, bufVar);
      }
      if (i == 4)
      {
        mqttClient.publish(cat, 0, 0, bufFrequency);
      }
      if (i == 5)
      {
        if (atof(bufPstkwh) >= 0.01 || !isnan(atof(bufPstkwh)))
        {
          mqttClient.publish(cat, 0, 0, bufPstkwh);
        }
      }
      if (i == 6)
      {
        mqttClient.publish(cat, 0, 0, bufPstkvarh);
      }
      if (i == 7)
      {
        mqttClient.publish(cat, 0, 0, bufNgtkvarh);
      }
      if (i == 8)
      {
        mqttClient.publish(cat, 0, 0, bufPowerFactor);
      }
      if (i == 9)
      {
        mqttClient.publish(cat, 0, 0, bufApparentPower);
      }
      if (i == 10)
      {
        mqttClient.publish(cat, 0, 0, bufUnk2);
      }
    }
  }

  // -------------------------------------------------------------------
  // 10 SECOND
  // -------------------------------------------------------------------

  static unsigned long lastTimer10s;
  if (millis() - lastTimer10s >= 10000)
  {

    lastTimer10s = millis(); //  Update time

    //measure base topic length
    uint8_t lenBaseTopic = strlen(root[FPSTR(pgm_pub_10s_basetopic)]);

    //pointer to base topic
    const char *bt = root[FPSTR(pgm_pub_10s_basetopic)];

    for (int i = 0; i < 11; i++)
    {

      //create buffer to hold the result of concat
      char cat[lenBaseTopic + 1];

      //copy to buffer
      strlcpy(cat, bt, sizeof(cat) / sizeof(cat[0]));

      //pointer to param
      const char *pr = pub_param[i];

      //concat param to buffer
      strcat(cat, pr);

      // if (ws.hasClient(num))
      // {
      //   //ws.text(num, cat);
      // }

      if (i == 0)
      {
        if (!isnan(atof(bufVoltage)) || atoi(bufVoltage) == 0)
        {
          mqttClient.publish(cat, 0, 0, bufVoltage);
        }
      }
      if (i == 1)
      {
        mqttClient.publish(cat, 0, 0, bufAmpere);
      }
      if (i == 2)
      {
        mqttClient.publish(cat, 0, 0, bufWatt);
      }
      if (i == 3)
      {
        mqttClient.publish(cat, 0, 0, bufVar);
      }
      if (i == 4)
      {
        mqttClient.publish(cat, 0, 0, bufFrequency);
      }
      if (i == 5)
      {
        if (atof(bufPstkwh) >= 0.01 || !isnan(atof(bufPstkwh)))
        {
          mqttClient.publish(cat, 0, 0, bufPstkwh);
        }
      }
      if (i == 6)
      {
        mqttClient.publish(cat, 0, 0, bufPstkvarh);
      }
      if (i == 7)
      {
        mqttClient.publish(cat, 0, 0, bufNgtkvarh);
      }
      if (i == 8)
      {
        mqttClient.publish(cat, 0, 0, bufPowerFactor);
      }
      if (i == 9)
      {
        mqttClient.publish(cat, 0, 0, bufApparentPower);
      }
      if (i == 10)
      {
        mqttClient.publish(cat, 0, 0, bufUnk2);
      }
    }
  }

  ////----------TIME

  // int hibyteYear = (regs[22] & 0xff00) >> 8;
  // int lobyteMonth = (regs[22] & 0xff);
  // int hibyteDay = (regs[23] & 0xff00) >> 8;
  // int lobyteHour = (regs[23] & 0xff);
  // int hibyteMinute = (regs[24] & 0xff00) >> 8;
  uint16_t lobyteSecond = (regs[24] & 0xff);

  // use the following code to avoid using delay()

  // print only if second value has changed; i.e print every second
  if (lobyteSecond != oldSecond)
  {
    oldSecond = lobyteSecond;

    /*
      Serial1.print("readReg[22]: ");
      Serial1.println(regs[22], HEX);
      Serial1.print("readReg[23]: ");
      Serial1.println(regs[23], HEX);
      Serial1.print("readReg[24]: ");
      Serial1.println(regs[24], HEX);
    */

    DEBUGMODBUS("%d Year, %d Month, %d Day, %d Hour, %d Minute, %d Second\n",
                hibyteYear, lobyteMonth, hibyteDay, lobyteHour, hibyteMinute, lobyteSecond);

    //    DEBUGMODBUS("V: %.1f, ", Voltage);
    //    DEBUGMODBUS("I: %.3f, ", Ampere);
    //    DEBUGMODBUS("W: %.1f, ", Watt);
    //    DEBUGMODBUS("VAr: %.1f, ", Var);
    //    DEBUGMODBUS("Hz: %.2f, ", Frequency);
    //    DEBUGMODBUS("PstkWh: %.3f, ", Pstkwh);
    //    DEBUGMODBUS("PstkVarh: %.3f, ", Pstkvarh);
    //    DEBUGMODBUS("NgtkVarh: %.3f, ", Ngtkvarh);
    //    DEBUGMODBUS("PF: %.1f, ", PowerFactor);
    //    DEBUGMODBUS("VA: %.1f, ", ApparentPower);
    //    DEBUGMODBUS("Unk2: %.1f", Unk2);
    //    DEBUGMODBUS("\r\n");

    /*
      Serial1.print("requests: ");
      Serial1.println(packets[PACKET1].requests);
      Serial1.print("successful_requests: ");
      Serial1.println(packets[PACKET1].successful_requests);
      Serial1.print("failed_requests: ");
      Serial1.println(packets[PACKET1].failed_requests);
      Serial1.print("exception_errors: ");
      Serial1.println(packets[PACKET1].exception_errors);
      Serial1.print("connection: ");
      Serial1.println(packets[PACKET1].connection);
    */

    /*

      Serial1.print("readReg[120]: ");
      Serial1.println(regs[120]);
      Serial1.print("readReg[121]: ");
      Serial1.println(regs[121]);
      Serial1.print("readReg[122]: ");
      Serial1.println(regs[122]);
      Serial1.print("readReg[123]: ");
      Serial1.println(regs[123]);
      Serial1.print("readReg[124]: ");
      Serial1.println(regs[124]);
      Serial1.print("readReg[125]: ");
      Serial1.println(regs[125]);
      Serial1.print("readReg[126]: ");
      Serial1.println(regs[126]);
      Serial1.print("readReg[127]: ");
      Serial1.println(regs[127]);
      Serial1.print("readReg[128]: ");
      Serial1.println(regs[128]);
      Serial1.print("readReg[129]: ");
      Serial1.println(regs[129]);

      Serial1.print("requests: ");
      Serial1.println(packets[PACKET1].requests);
      Serial1.print("successful_requests: ");
      Serial1.println(packets[PACKET1].successful_requests);
      Serial1.print("failed_requests: ");
      Serial1.println(packets[PACKET1].failed_requests);
      Serial1.print("exception_errors: ");
      Serial1.println(packets[PACKET1].exception_errors);
      Serial1.print("connection: ");
      Serial1.println(packets[PACKET1].connection);

    */
  }
}
