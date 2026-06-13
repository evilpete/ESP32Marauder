
#include "configs.h"

#ifdef HAS_RTC

#ifndef rtc_h
#define rtc_h


#include <Arduino.h>
#include "RTClib.h"

#include <time.h>
#include <sys/time.h>

#include "WiFiScan.h"

extern WiFiScan wifi_scan_obj;


/*
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -28800;      // Replace with your offset (e.g., -28800 for PST)
const int daylightOffset_sec = 3600;    // Adjust daylight savings (e.g., 3600 for DST)
*/


class RTC  {    // RTC_PCF8523

  public:
    RTC_PCF8523 rtclock;

    void RunSetup();
    bool supported = false;
    String dt_string();
    String millis_dt_string();
    bool sync_rtc_ntp();
    bool synced = false;

    bool getSystemTimeFromString(const char* timeStr);
    char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    const char* ntpServer = "pool.ntp.org";
    const long gmtOffset_sec = 0;   // Always 0 for UTC
    const int daylightOffset_sec = 0;
    void setSystemTimeFromCompile();
    void syncFromRTC();

};

#endif // rtc_h

#endif // HAS_RTC
