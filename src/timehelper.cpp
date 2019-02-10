#include <Arduino.h>
#include <time.h>
#include <sys/time.h>  // struct timeval
#include <coredecls.h> // settimeofday_cb()
#include "timehelper.h"
#include "sntphelper.h"
#include "EspGoodies.h"
// #include "rtchelper.h"

#define DEBUGPORT Serial

#define RELEASE

#ifndef RELEASE
#define PROGMEM_T __attribute__((section(".irom.text.template")))
#define DEBUGLOG(fmt, ...)                        \
    {                                             \
        static const char pfmt[] PROGMEM_T = fmt; \
        DEBUGPORT.printf_P(pfmt, ##__VA_ARGS__);  \
    }
#else
#define DEBUGLOG(...)
#endif

// for testing purpose:
extern "C" int clock_gettime(clockid_t unused, struct timespec *tp);

bool tick1000ms = false;

bool NTP_OK = false;

timeval tv;
timespec tp;
time_t now;
timeval cbtime; // time set in callback
bool cbtime_set = false;
uint32_t now_ms, now_us;
time_t uptime;
time_t lastSync; ///< Stored time of last successful sync
time_t _firstSync; ///< Stored time of first successful sync after boot
time_t _lastBoot;

uint16_t syncInterval;      ///< Interval to set periodic time sync
uint16_t shortSyncInterval; ///< Interval to set periodic time sync until first synchronization.
uint16_t longSyncInterval;  ///< Interval to set periodic time sync

strConfigTime _configTime;
TIMESOURCE _timeSource;

void time_is_set(void)
{
    gettimeofday(&cbtime, NULL);
    cbtime_set = true;
    Serial.println(F("\r\n------------------ settimeofday() was called ------------------\r\n"));
}

float TimezoneFloat()
{
    time_t rawtime;
    struct tm *timeinfo;
    char buffer[10];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer, 6, "%z", timeinfo);

    char bufTzHour[10];
    strncpy(bufTzHour, buffer, 3);
    int8_t hour = atoi(bufTzHour);

    char bufTzMin[4];
    bufTzMin[0] = buffer[0]; // sign
    bufTzMin[1] = buffer[3];
    bufTzMin[2] = buffer[4];
    float min = atoi(bufTzMin) / 60.0;

    float TZ_FLOAT = hour + min;
    return TZ_FLOAT;
}

char *getDateStr()
{
    DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);
    static char buf[20];
    time_t rawtime;
    time(&rawtime);
    struct tm *timeinfo = localtime(&rawtime);
    strftime(buf, sizeof(buf)/sizeof(buf[0]), "%a %d %b %Y", timeinfo);

    return buf;
}

char *getTimeStr()
{
    DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);
    static char buf[20];
    time_t rawtime;
    time(&rawtime);
    struct tm *timeinfo = localtime(&rawtime);
    strftime(buf, sizeof(buf)/sizeof(buf[0]), "%T", timeinfo);

    return buf;
}

char *getLastBootStr()
{
    DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);

    // time_t uptime = tp.tv_sec;
    time_t lastBoot = now - uptime;
    static char buf[30];
    struct tm *tm = localtime(&lastBoot);
    sprintf_P(buf, PSTR("%s"), asctime(tm));

    return buf;
}

char *getUptimeStr()
{
    DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);
    //time_t uptime = utcTime - _lastBoot;
    // time_t uptime = millis() / 1000;
    // time_t uptime = tp.tv_sec;

    uint16_t days;
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;

    struct tm *tm = gmtime(&uptime); // convert to broken down time
    days = tm->tm_yday;
    hours = tm->tm_hour;
    minutes = tm->tm_min;
    seconds = tm->tm_sec;

    static char buf[30];
    //365 days 23:23:23
    sprintf_P(buf, PSTR("%u days %02d:%02d:%02d"), days, hours, minutes, seconds);

    return buf;
}

char *getLastSyncStr()
{
    DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);

    time_t diff = now - lastSync;

    uint16_t days;
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;

    struct tm *tm = gmtime(&diff); // convert to broken down time
    days = tm->tm_yday;
    hours = tm->tm_hour;
    minutes = tm->tm_min;
    seconds = tm->tm_sec;

    static char buf[30];
    if (days > 0)
    {
        sprintf_P(buf, PSTR("%u day %d hr ago"), days, hours);
    }
    else if (hours > 0)
    {
        sprintf_P(buf, PSTR("%d hr %d min ago"), hours, minutes);
    }
    else if (minutes > 0)
    {
        sprintf_P(buf, PSTR("%d min ago"), minutes);
    }
    else
    {
        sprintf_P(buf, PSTR("%d sec ago"), seconds);
    }

    return buf;
}

char *getNextSyncStr()
{
    DEBUGLOG("%s\r\n", __PRETTY_FUNCTION__);

    time_t _syncInterval = 3600;

    time_t diff = (lastSync + _syncInterval) - now;

    uint16_t days;
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;

    struct tm *tm = gmtime(&diff); // convert to broken down time
    days = tm->tm_yday;
    hours = tm->tm_hour;
    minutes = tm->tm_min;
    seconds = tm->tm_sec;

    static char buf[30];
    sprintf_P(buf, PSTR("%u days %02d:%02d:%02d"), days, hours, minutes, seconds);

    return buf;
}

void timeSetup()
{
    // Synchronize time useing SNTP. This is necessary to verify that
    // the TLS certificates offered by the server are currently valid.
    DEBUGLOG("Setting time using SNTP\r\n");
    settimeofday_cb(time_is_set);

    configTime(0, 0, "pool.ntp.org", "192.168.10.1");
    configTZ(TZ_Asia_Jakarta);
    // configTZ(TZ_Asia_Kathmandu);
}

void timeLoop()
{
    gettimeofday(&tv, nullptr);
    clock_gettime(0, &tp);
    now = tv.tv_sec;
    uptime = tp.tv_sec;
    now_ms = millis();
    now_us = micros();

    // localtime / gmtime every second change
    static time_t lastv = 0;
    if (lastv != tv.tv_sec)
    {
        lastv = tv.tv_sec;
        tick1000ms = true;
    }

    if (cbtime_set)
    {
        cbtime_set = false;

        lastSync = now;
    }
}
