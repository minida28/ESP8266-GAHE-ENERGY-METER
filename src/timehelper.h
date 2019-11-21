#ifndef timehelper_h
#define timehelper_h

#include <time.h>

extern bool tick1000ms;



// timeval tv;
// timespec tp;
extern time_t now;
// uint32_t now_ms, now_us;
// extern time_t uptime;
extern time_t lastSync; ///< Stored time of last successful sync
extern time_t _firstSync; ///< Stored time of first successful sync after boot
extern time_t _lastBoot;

float TimezoneFloat();
char *getDateStr();
char *getTimeStr();
char *getLastBootStr();
char *getUptimeStr();
char *getLastSyncStr();
char *getNextSyncStr();

time_t tmConvert_t(int YYYY, byte MM, byte DD, byte hh, byte mm, byte ss);

void timeSetup();
void timeLoop();

#endif