#include <Arduino.h>
#include "sntphelper.h"
#include <time.h>      // time() ctime()
#include <sys/time.h>  // struct timeval
#include <coredecls.h> // settimeofday_cb()

extern "C"
{
#include "sntp.h"
}

bool syncNtpEventTriggered = false; // True if a time even has been triggered

time_t getNtpTimeSDK()
{
    syncNtpEventTriggered = true;

    Serial.println(F("Execute getNtpTimeSDK()"));

    Serial.print(F("NTP server-0: "));
    Serial.println(sntp_getservername(0));

    Serial.print(F("NTP server-1: "));
    Serial.println(sntp_getservername(1));

    Serial.print(F("NTP server-2: "));
    Serial.println(sntp_getservername(2));

    Serial.print(F("Timezone: "));
    Serial.println(sntp_get_timezone());

    //Serial.println((unsigned long)sntp_get_current_timestamp());
    //Serial.println((unsigned long)sntp_get_real_time(sntp_get_current_timestamp()));

    Serial.println(F("Transmit NTP request"));

    unsigned long startMicros = micros();

    uint32_t current_stamp = sntp_get_current_timestamp();

    unsigned long endMicros = micros();

    Serial.print(F("start: "));
    Serial.print(startMicros);
    Serial.print(F(", end: "));
    Serial.println(endMicros);

    Serial.print(F("Transmit time: "));
    Serial.print(endMicros - startMicros);
    Serial.println(F(" us"));

    //Serial.println(current_stamp);

    if (current_stamp > 1514764800)
    {
        char str[50];

        sprintf(str, "Received timestamp: %d, %s", current_stamp, sntp_get_real_time(current_stamp));

        Serial.print(str); //print the string to the serial port

        //str[0] = '\0';
        //sprintf(str, "%d", current_stamp, sntp_get_real_time(current_stamp));

        //char *ptr;
        //unsigned long real_stamp;
        //real_stamp = strtoul(str, &ptr, 10);
        //Serial.println(real_stamp);

        Serial.println(F("Timestamp is valid, later than 1 January 2018 :-)"));

        // store as last sync timestamp
        // _lastSyncd = current_stamp;
        return current_stamp;
    }
    else if (current_stamp > 0 && current_stamp < 1514764800)
    {
        Serial.println(F("Timestamp is invalid, older than 1 January 2018 :-("));
        return 0;
    }
    else if (current_stamp == 0)
    {
        Serial.println(F("No NTP Response :-("));
        return 0;
    }

    Serial.println(F("Other error"));
    return 0;
}