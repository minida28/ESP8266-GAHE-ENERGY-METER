#include "FSWebServerLib.h"
#include <ArduinoJson.h>
#include <pgmspace.h>
#include "gahe1progmem.h"


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

/*
  NTP-TZ-DST (v2)
  NetWork Time Protocol - Time Zone - Daylight Saving Time
  This example shows:
  - how to read and set time
  - how to set timezone per country/city
  - how is local time automatically handled per official timezone definitions
  - how to change internal sntp start and update delay
  - how to use callbacks when time is updated
  This example code is in the public domain.
*/

// initial time (possibly given by an external RTC)
#define RTC_UTC_TEST 1510592825 // 1510592825 = Monday 13 November 2017 17:07:05 UTC

// This database is autogenerated from IANA timezone database
//    https://www.iana.org/time-zones
// and can be updated on demand in this repository
#include <TZ.h>

// "TZ_" macros follow DST change across seasons without source code change
// check for your nearest city in TZ.h

// espressif headquarter TZ
//#define MYTZ TZ_Asia_Shanghai

// example for "Not Only Whole Hours" timezones:
// Kolkata/Calcutta is shifted by 30mn
//#define MYTZ TZ_Asia_Kolkata

// example of a timezone with a variable Daylight-Saving-Time:
// demo: watch automatic time adjustment on Summer/Winter change (DST)
#define MYTZ TZ_Asia_Jakarta

////////////////////////////////////////////////////////

#include <ESP8266WiFi.h>
#include <coredecls.h> // settimeofday_cb()
#include <Schedule.h>
#include <PolledTimeout.h>

#include <time.h>     // time() ctime()
#include <sys/time.h> // struct timeval

#include <sntp.h> // sntp_servermode_dhcp()

// for testing purpose:
extern "C" int clock_gettime(clockid_t unused, struct timespec *tp);

////////////////////////////////////////////////////////

static timeval tv;
static timespec tp;
static time_t now;
static uint32_t now_ms, now_us;
uint32_t lastBoot = 0;
static uint32_t lastSync = 0;

static esp8266::polledTimeout::periodicMs showTimeNow(60000);
static int time_machine_days = 0; // 0 = now
static bool time_machine_running = false;

// OPTIONAL: change SNTP startup delay
// a weak function is already defined and returns 0 (RFC violation)
// it can be redefined:
//uint32_t sntp_startup_delay_MS_rfc_not_less_than_60000 ()
//{
//    //info_sntp_startup_delay_MS_rfc_not_less_than_60000_has_been_called = true;
//    return 60000; // 60s (or lwIP's original default: (random() % 5000))
//}

// OPTIONAL: change SNTP update delay
// a weak function is already defined and returns 1 hour
// it can be redefined:
//uint32_t sntp_update_delay_MS_rfc_not_less_than_15000 ()
//{
//    //info_sntp_update_delay_MS_rfc_not_less_than_15000_has_been_called = true;
//    return 15000; // 15s
//}

#define PTM(w)                \
    Serial.print(" " #w "="); \
    Serial.print(tm->tm_##w);

void printTm(const char *what, const tm *tm)
{
    Serial.print(what);
    PTM(isdst);
    PTM(yday);
    PTM(wday);
    PTM(year);
    PTM(mon);
    PTM(mday);
    PTM(hour);
    PTM(min);
    PTM(sec);
}

void showTime()
{
#ifndef RELEASE
    gettimeofday(&tv, nullptr);
    clock_gettime(0, &tp);
    now = time(nullptr);
    now_ms = millis();
    now_us = micros();

    Serial.println();
    printTm("localtime:", localtime(&now));
    Serial.println();
    printTm("gmtime:   ", gmtime(&now));
    Serial.println();

    // time from boot
    Serial.print("clock:     ");
    Serial.print((uint32_t)tp.tv_sec);
    Serial.print("s / ");
    Serial.print((uint32_t)tp.tv_nsec);
    Serial.println("ns");

    // time from boot
    Serial.print("millis:    ");
    Serial.println(now_ms);
    Serial.print("micros:    ");
    Serial.println(now_us);

    // EPOCH+tz+dst
    Serial.print("gtod:      ");
    Serial.print((uint32_t)tv.tv_sec);
    Serial.print("s / ");
    Serial.print((uint32_t)tv.tv_usec);
    Serial.println("us");

    // EPOCH+tz+dst
    Serial.print("time:      ");
    Serial.println((uint32_t)now);

    // timezone and demo in the future
    Serial.printf("timezone:  %s\n", MYTZ);

    // human readable
    Serial.print("ctime:     ");
    Serial.print(ctime(&now));

#if LWIP_VERSION_MAJOR > 1
    // LwIP v2 is able to list more details about the currently configured SNTP servers
    for (int i = 0; i < SNTP_MAX_SERVERS; i++)
    {
        IPAddress sntp = *sntp_getserver(i);
        const char *name = sntp_getservername(i);
        if (sntp.isSet())
        {
            Serial.printf("sntp%d:     ", i);
            if (name)
            {
                Serial.printf("%s (%s) ", name, sntp.toString().c_str());
            }
            else
            {
                Serial.printf("%s ", sntp.toString().c_str());
            }
            Serial.printf("IPv6: %s Reachability: %o\n",
                          sntp.isV6() ? "Yes" : "No",
                          sntp_getreachability(i));
        }
    }
#endif

    Serial.println();
#endif
}

void time_is_set_scheduled()
{
    // everything is allowed in this function

    if (time_machine_days == 0)
    {
        time_machine_running = !time_machine_running;
    }

    // time machine demo
    if (time_machine_running)
    {
        if (time_machine_days == 0)
            DEBUGLOG("---- settimeofday() has been called - possibly from SNTP\r\n"
                     "     (starting time machine demo to show libc's automatic DST handling)\r\n\r\n");
        now = time(nullptr);
        const tm *tm = localtime(&now);
        DEBUGLOG("future=%3ddays: DST=%s - ",
                 time_machine_days,
                 tm->tm_isdst ? "true " : "false");
        DEBUGLOG("%s", ctime(&now));
        gettimeofday(&tv, nullptr);
        constexpr int days = 30;
        time_machine_days += days;
        if (time_machine_days > 360)
        {
            tv.tv_sec -= (time_machine_days - days) * 60 * 60 * 24;
            time_machine_days = 0;
        }
        else
        {
            tv.tv_sec += days * 60 * 60 * 24;
        }
        settimeofday(&tv, nullptr);

        //---- set last boot once
        static bool lastBootIsSet = false;
        if (!lastBootIsSet)
        {
            lastBootIsSet = true;
            lastBoot = (uint32_t)now - millis() / 1000;
        }

        //---- set last sync
        now = time(nullptr); // kalau diulang time(nullptr)-nya, timestamp-nya salah... aneh banget
        lastSync = (uint32_t)now;
    }
    else
    {
#ifndef RELEASE
        showTime();
#endif
    }
}

void sendDateTime(uint8_t mode)
{
    DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);

    struct tm *ptm;

    now = time(nullptr);

    ptm = localtime(&now);

    // DynamicJsonDocument root(2048);
    StaticJsonDocument<1024> root;

    root["d"] = ptm->tm_mday;
    root["m"] = ptm->tm_mon;
    root["y"] = ptm->tm_year + 1900;
    root["hr"] = ptm->tm_hour;
    root["min"] = ptm->tm_min;
    root["sec"] = ptm->tm_sec;
    // root["tz"] = TimezoneFloat();
    //   root["tzStr"] = _configLocation.timezonestring;
    root["utc"] = (uint32_t)now;
    // root["local"] = localTime;

    // root[FPSTR(pgm_date)] = getDateStr(localTime);
    // root[FPSTR(pgm_time)] = getTimeStr(localTime);
    root[FPSTR(pgm_uptime)] = millis() / 1000;
    root[FPSTR(pgm_lastboot)] = lastBoot;
    // root[FPSTR(pgm_internetstatus)] = FPSTR(internetstatus_P[internet]);
    // root[FPSTR(pgm_rtcstatus)] = FPSTR(rtcstatus_P[GetRtcStatus()]);

    //   if (syncByRtcFlag)
    //   {
    //     root[FPSTR(pgm_lastsyncby)] = FPSTR(pgm_RTC);
    //   }
    //   else if (syncByNtpFlag)
    //   {
    //     root[FPSTR(pgm_lastsyncby)] = FPSTR(pgm_NTP);
    //   }
    //   else
    //   {
    //     root[FPSTR(pgm_lastsyncby)] = FPSTR(pgm_None);
    //   }

    //   root[FPSTR(pgm_nextsync)] = getNextSyncStr();

    // if (lastSyncByNtp)
    if (true)
    {
        root[FPSTR(pgm_lastsyncbyntp)] = lastSync;
        root[FPSTR(pgm_nextsync)] = lastSync + 3600;
    }
    else
    {
        root[FPSTR(pgm_lastsyncbyntp)] = FPSTR(pgm_never);
    }

    //   if (lastSyncByRtc)
    //   {
    //     root[FPSTR(pgm_lastsyncbyrtc)] = getLastSyncStr(lastSyncByRtc);
    //   }
    //   else
    //   {
    //     root[FPSTR(pgm_lastsyncbyrtc)] = FPSTR(pgm_never);
    //   }

    size_t len = measureJson(root);
    char buf[len + 1];
    serializeJson(root, buf, len + 1);

    if (mode == 0)
    {
        //
    }
    else if (mode == 1)
    {
        // events.send(buf);
        // events.send(buf, "timeDate", millis());
    }
    else if (mode == 2)
    {
        if (ws.hasClient(clientID))
        {
            ws.text(clientID, buf);
        }
        else
        {
            DEBUGLOG("ClientID %d is no longer available.\r\n", clientID);
        }
    }
}





void Timesetup()
{
    DEBUGLOG("\r\nSetting-up time...\r\n\r\n");

    // setup RTC time
    // it will be used until NTP server will send us real current time
    time_t rtc = RTC_UTC_TEST;
    timeval tv = {rtc, 0};
    timezone tz = {0, 0};
    settimeofday(&tv, &tz);

    // install callback - called when settimeofday is called (by SNTP or us)
    // once enabled (by DHCP), SNTP is updated every hour
    settimeofday_cb(time_is_set_scheduled);

    // NTP servers may be overriden by your DHCP server for a more local one
    // (see below)
    configTime(MYTZ, "pool.ntp.org");

    // OPTIONAL: disable obtaining SNTP servers from DHCP
    //sntp_servermode_dhcp(0); // 0: disable obtaining SNTP servers from DHCP (enabled by default)

    // start network
    //   WiFi.persistent(false);
    //   WiFi.mode(WIFI_STA);
    //   WiFi.begin(STASSID, STAPSK);

    // don't wait for network, observe time changing
    // when NTP timestamp is received
    DEBUGLOG("Time is currently set by a constant:\n");

    showTime();
}

void Timeloop()
{
    if (showTimeNow)
    {
        showTime();
    }

    if (sendDateTimeFlag)
    {
        sendDateTimeFlag = false;
        sendDateTime(2);
    }
}