#ifndef sntphelper_h
#define sntphelper_h

#include <time.h>

extern bool syncNtpEventTriggered; // True if a time even has been triggered
extern bool NTP_OK;


time_t getNtpTimeSDK();

#endif