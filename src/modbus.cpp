// =================================================================================================
// eModbus: Copyright 2020 by Michael Harwerth, Bert Melis and the contributors to ModbusClient
//               MIT license - see license.md for details
// =================================================================================================
// Includes: <Arduino.h> for Serial etc., WiFi.h for WiFi support

// modbus_construct(&packets[PACKET1], 3, READ_INPUT_REGISTERS, 10, 2, 0); // read Present Voltage (Volt)
// modbus_construct(&packets[PACKET2], 3, READ_INPUT_REGISTERS, 22, 2, 2); // read Present Current (Ampere)
// modbus_construct(&packets[PACKET3], 3, READ_INPUT_REGISTERS, 28, 2, 4); // read Present True Power (Watt or kW)
// modbus_construct(&packets[PACKET4], 3, READ_INPUT_REGISTERS, 50, 2, 6); // read Accumulative Positive kWh
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

#include "modbus.h"

// #include "SimpleModbusMaster.h"

#define SOFTWARESERIAL

#include "FSWebServerLib.h"
#include <ArduinoJson.h>
#include "modbus.h"
#include "mqtt.h"
#include "timehelper.h"
#include "gahe1progmem.h"
// #include "config.h"
#include <Ticker.h>

#define RELEASE

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

#if defined(SOFTWARESERIAL)
#include "SoftwareSerial.h"
uint8_t Rx = 14; // WEMOS_PIN_D5;
uint8_t Tx = 12; // WEMOS_PIN_D6;
#endif

//////////////////// Port information ///////////////////
#define baud 9600

// #define polling 50 // the scan rate; default value is 40
// #define modbusTimeout 100
// #define retry_count 30

#define polling 200 // the scan rate; default value is 40
#define modbusTimeout 1000
#define retry_count 10

// used to toggle the receive/transmit pin on the driver
#define TxEnablePin 5 // WEMOS_PIN_D1

#define PIN_LED 2 // WEMOS_PIN_D4

// The total amount of available memory on the master to store data
#define TOTAL_NO_OF_REGISTERS 10 * TOTAL_NO_OF_PACKETS

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

// bool tick1000msModbus;
// bool tick60sModbus;
uint32_t numModbusSuccessReq = 0;

byte counter = 0;

// void Ticking1000ms()
// {
//     tick1000msModbus = true;
//     counter = 0;
// }

// void Ticking60s()
// {
//     tick60sModbus = true;
// }

////////////////////////////////////////////////////////////////////////
ModbusRTU mb;

#define SLAVE_ID 3
#define REG_COUNT 24

uint16_t res[REG_COUNT];

bool newReadingAvailable = false;

bool cbWrite(Modbus::ResultCode event, uint16_t transactionId, void *data)
{
#ifdef ESP8266

    static float Voltage;
    static float Ampere;
    static float Watt;
    static float Var;
    static float Frequency;
    static float Pstkwh;
    static float Pstkvarh;
    static float Ngtkvarh;
    static float PowerFactor;
    static float ApparentPower;
    static float Unk2;
    static unsigned long YearMonth;
    static unsigned long DayHour;
    static unsigned long MinuteSecond;

    if (event != Modbus::EX_SUCCESS)
    {
        DEBUGLOG("Request result: 0x%02X, Mem: %d\n", event, ESP.getFreeHeap());
        // Serial.print("Request result: 0x");
        // Serial.print(event, HEX);
    }
    else if (event == Modbus::EX_SUCCESS && counter == 0)
    {

        // uint32_t value32;
        // mb.readHreg(SLAVE_ID, 8, (uint16_t *)&value32, 2, cbWrite);
        // value32 = (value32 >> 16) | (value32 << 16); // Uncomment for words swap
        // Serial.printf("voltage = %.2f\n", value32);

        unsigned long temp;
        unsigned long *p = &temp;

        // float Voltage;
        temp = (unsigned long)res[0] << 16 | res[1];
        Voltage = *(float *)p;
        // Voltage = Voltage - 1.9;

        // uint16_t const value[2] = { res[13], res[12] };  // Assuming little-endianness
        // float f;

        // std::memcpy(reinterpret_cast<void*>(&f), reinterpret_cast<void const*>(value), sizeof f);

        unsigned long tempCurrent = (unsigned long)res[12] << 16 | res[13];
        unsigned long *pCurrent = &tempCurrent;
        Ampere = *(float *)pCurrent;

        // float Watt;
        temp = (unsigned long)res[18] << 16 | res[19];
        Watt = *(float *)p;

        // //float Var;
        // temp = (unsigned long)regs[6] << 16 | regs[7];
        // Var = *(float *)p;

        // //float Frequency;
        // temp = (unsigned long)regs[8] << 16 | regs[9];
        // Frequency = *(float *)p;

        // float Pstkwh;
        // temp = (unsigned long)res[40] << 16 | res[41];
        // Pstkwh = *(float *)p;

        // Serial.printf("Volt=%.2f Amp=%.2f Watt=%.2f kWh=%.2f\n", Voltage, Ampere, Watt, Pstkwh);
        DEBUGLOG("counter=%d Volt=%.2f Amp=%.2f Watt=%.2f\n", counter, Voltage, Ampere, Watt);
        // Serial.println(Voltage);
        // Serial.println(Ampere);
        // Serial.println(Watt);
        // Serial.println(Voltage);
    }
    else if (event == Modbus::EX_SUCCESS && counter == 1)
    {
        unsigned long temp;
        unsigned long *p = &temp;

        // float Var;
        temp = (unsigned long)res[0] << 16 | res[1];
        Var = *(float *)p;

        // float Frequency;
        temp = (unsigned long)res[12] << 16 | res[13];
        Frequency = *(float *)p;

        // float Pstkwh;
        temp = (unsigned long)res[14] << 16 | res[15];
        Pstkwh = *(float *)p;

        // float Pstkvarh;
        temp = (unsigned long)res[18] << 16 | res[19];
        Pstkvarh = *(float *)p;

        // float Ngtkvarh;
        temp = (unsigned long)res[20] << 16 | res[21];
        Ngtkvarh = *(float *)p;

        // float PowerFactor;
        temp = (unsigned long)res[22] << 16 | res[23];
        PowerFactor = *(float *)p;

        DEBUGLOG("counter=%d Var=%.2f Frequency=%.2f Pstkwh=%.2f Pstkvarh=%.2f Ngtkvarh=%.2f PowerFactor=%.2f\n",
                 counter, Var, Frequency, Pstkwh, Pstkvarh, Ngtkvarh, PowerFactor);

        newReadingAvailable = true;
    }
    else if (event == Modbus::EX_SUCCESS && counter == 2)
    {
        unsigned long temp;
        unsigned long *p = &temp;

        // float ApparentPower VA or kVA
        temp = (unsigned long)res[0] << 16 | res[1];
        ApparentPower = *(float *)p;

        // float PowerFactor;
        temp = (unsigned long)res[6] << 16 | res[7];
        Unk2 = *(float *)p;

        DEBUGLOG("counter=%d ApparentPower=%.2f Unk2=%.2f\n", counter, ApparentPower, Unk2);
    }
    else if (event == Modbus::EX_SUCCESS && counter == 3)
    {
        YearMonth = (unsigned long)res[0];
        DayHour = (unsigned long)res[1];
        MinuteSecond = (unsigned long)res[2];

        DEBUGLOG("counter=%d YearMonth=%lu DayHour=%lu MinuteSecond=%lu\n", counter, YearMonth, DayHour, MinuteSecond);
    }
#elif ESP32
    Serial.printf_P("Request result: 0x%02X, Mem: %d\n", event, ESP.getFreeHeap());
#else
    Serial.print("Request result: 0x");
    Serial.print(event, HEX);
#endif

    counter++;

    if (counter >= 4)
        counter = 0;

    if (newReadingAvailable)
    {
        newReadingAvailable = false;

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
            strncat(bufFullTopic, "/meterreading/1s", (sizeof(bufFullTopic) - 1) / sizeof(bufFullTopic[0]));
            mqttClient.publish(
                bufFullTopic, // topic
                0,            // qos
                0,            // retain
                buf           // payload
            );

            static uint8_t menit = 0;
            if (minLocal != menit)
            {
                menit = minLocal;
                char bufFullTopic[64];
                strlcpy(bufFullTopic, ESPHTTPServer._config.hostname, sizeof(bufFullTopic) / sizeof(bufFullTopic[0]));
                strncat(bufFullTopic, "/meterreading/60s", (sizeof(bufFullTopic) - 1) / sizeof(bufFullTopic[0]));
                mqttClient.publish(
                    bufFullTopic, // topic
                    0,            // qos
                    0,            // retain
                    buf           // payload
                );
            }
        }
    }

    return true;
}

void modbusSetup()
{
#if defined(ESP8266)
    swSer.begin(baud, SWSERIAL_8N1, Rx, Tx);
    mb.begin(&swSer, TxEnablePin);
#elif defined(ESP32)
    Serial1.begin(9600, SERIAL_8N1);
    mb.begin(&Serial1, TxEnablePin);
#else
    Serial1.begin(9600, SERIAL_8N1);
    mb.begin(&Serial1, TxEnablePin);
    mb.setBaudrate(9600);
#endif
    mb.master();
}

void modbusLoop()
{
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

    static uint8_t detik = 0;
    if (secLocal != detik)
    {

        if (!mb.slave())
        {
            // uint16_t res[REG_COUNT];
            // unsigned long res[REG_COUNT];
            // mb.Hreg(1, 1, coils, 20, cbWrite);
            uint16_t STARTADDRESS;
            switch (counter)
            {
            case 0:
                STARTADDRESS = 10;
                mb.readIreg(SLAVE_ID, STARTADDRESS, res, REG_COUNT, cbWrite); // write all relays statuses to dest.
                break;
            case 1:
                STARTADDRESS = 36;
                mb.readIreg(SLAVE_ID, STARTADDRESS, res, REG_COUNT, cbWrite); // write all relays statuses to dest.
                break;
            case 2:
                STARTADDRESS = 64;
                mb.readIreg(SLAVE_ID, STARTADDRESS, res, REG_COUNT, cbWrite); // write all relays statuses to dest.
                break;
            case 3:

                detik = secLocal; // INI HARUS DISINI !!

                STARTADDRESS = 544;
                mb.readIreg(SLAVE_ID, STARTADDRESS, res, REG_COUNT, cbWrite); // write all relays statuses to dest.
                break;
            default:
                // default
                break;
            }
        }

        while (mb.slave())
        { // slave() is true while request is processed
            mb.task();
            // yield();
        }

        // mb.task();
    }
}

////////////////////////////////////////////////////////////////////////////



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
