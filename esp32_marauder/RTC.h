
#include "configs.h"

#ifdef HAS_RTC

#ifndef rtc_h
#define rtc_h


#include <Arduino.h>
#include "RTClib.h"

#include <time.h>
#include <sys/time.h>

#include <Wire.h>
#include "WiFiScan.h"

extern WiFiScan wifi_scan_obj;



/*
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -28800;      // Replace with your offset (e.g., -28800 for PST)
const int daylightOffset_sec = 3600;    // Adjust daylight savings (e.g., 3600 for DST)
*/

// Temp 
// AHT20  comming from Ada
// Si7021  comming from Ada
//  PCT2075  Have STEMMA
//   opt :AHT20 BMP280 = Ali $3

//
// RTC
//   DS3231  
// PCF8523  FeatherLogger
//   opt :DS1307 = coming from Ali
//   opt :DS1302 = Ali $3
//   opt :AT24C32 = Ali $6
//

class RTC  {    // RTC_PCF8523

  public:


    #ifdef HAS_PCF8523
      RTC_PCF8523 rtclock;
      bool PCF8523_setup();
    #elif HAS_DS1307
      RTC_DS1307 rtclock;
      bool DS1307_setup();
    #endif

    void RunSetup();
    bool supported = false;
    String dt_string();
    String millis_dt_string();
    bool sync_rtc_ntp();
    bool synced = false;


    // float getTemperature();  // PCF8523 only

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
