#ifndef _MYENERGYCONFIG_H
#define _MYENERGYCONFIG_H

typedef struct
{
  // int8_t timezone = _configLocation.timezone;
  bool dst = false;
  bool enablertc = true;
  uint32_t syncinterval = 600;
  bool enablentp = true;
  char ntpserver_0[48] = "0.id.pool.ntp.org";
  char ntpserver_1[48] = "0.asia.pool.ntp.org";
  char ntpserver_2[48] = "192.168.10.1";
} strConfigTime;
extern strConfigTime _configTime;

typedef enum timeSource
{
  TIMESOURCE_NOT_AVAILABLE,
  TIMESOURCE_NTP,
  TIMESOURCE_RTC
} TIMESOURCE;
extern TIMESOURCE _timeSource;

#endif