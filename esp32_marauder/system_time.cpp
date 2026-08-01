
#ifdef HAS_RTC
  #include "RTC.h"
#endif

#ifdef HAS_GPS
  #ifdef HAS_GPSI2C
    #include "GpsI2c.h"
  #else
    #include "GpsInterface.h"
  #endif
#endif


bool set_system_time(struct tm *timeInfo) {
  // struct tm tmp = timeInfo;
  time_t t = mktime(timeInfo);
  if (t == (time_t)-1) {
      log_w("set_system_time: mktime failed");
      return false;
  }
  struct timeval now = { .tv_sec = t, .tv_usec = 0 };
    if (settimeofday(&now, NULL) != 0) {
        log_d("settimeofday failed");
        return false;
    }
    system_time_set = true;
    log_d("system time updated");

    #ifdef HAS_RTC
      log_d("set_system_time: calling rtc_obj.adjust_rtc");
      rtc_obj.adjust_rtc(timeInfo);
    #endif

    return true;
}

bool set_system_time(const String& time_str) {
    struct tm tm_info = {0};
    // log_d("set_system_time: '%s'", time_str.c_str());
    if (strptime(time_str.c_str(), "%F %T", &tm_info) != NULL) {
        return set_system_time(&tm_info);
    }
    log_d("set_system_time: invalid time_str '%s'", time_str.c_str());
    return false;
}
