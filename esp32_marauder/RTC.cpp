#include "configs.h"

#ifdef HAS_RTC

#include "RTC.h"

// https://github.com/adafruit/RTClib
// RTC_PCF8523 rtc;

// https://forum.arduino.cc/t/time-library-functions-with-esp32-core/515397/9
// scanf(&timeinfo, "%m %d %Y / %H:%M:%S")
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/system_time.html

// BM8563 : M5StickC_Plus / ONX2432G028

#if !defined(RTC_SDA) && defined(I2C_SDA)
  #define RTC_SDA I2C_SDA
  #define RTC_SCL I2C_SCL
#endif

void RTC::RunSetup() {

  log_d("RTC::RunSetup SDA=%d SCL=%d", RTC_SDA, RTC_SCL);

  #if defined(I2C_SDA) && (RTC_SDA != I2C_SDA)
    log_d("RTC::RunSetup Using Wire1");
    _wire = &Wire1;
  #else
    _wire = &Wire;
  #endif

  #ifdef I2C_SCL
    _wire->setPins(RTC_SDA, RTC_SCL);
    _wire->begin();
  #endif

  supported = rtclock.begin(_wire);

  if (!supported) {
    log_w("Couldn't find RTC");
    // return supported;
  }

  setup();
}


#if defined(HAS_PCF8523)

bool RTC::setup() {

  log_d("RTC::PCF8523_setup");

  if (! rtclock.initialized() || rtclock.lostPower()) {
    Serial.println(F("RTC NOT initialized"));
    Serial.flush();
    log_w("RTC is NOT initialized");
    rtclock.adjust(DateTime(F(__DATE__), F(__TIME__)));
    synced = false;
    setSystemTimeFromCompile();
    Serial.println(F("SystemTime set from Build time"));
  } else {
    synced = true;
    syncFromRTC();
    Serial.println(F("SystemTime set from RTC"));
  }

  // do this to ensure the RTC is running.
  rtclock.start();

  Serial.println(dt_string());

  return supported;
}

// float RTC::getTemperature() { return 0.0;  }

#elif defined(HAS_DS1307)
// M5StickC_Plus

bool RTC::setup() {

  log_d("RTC::DS1307_setup");

    if (! rtclock.isrunning()) {
      Serial.println(F("RTC NOT initialized"));
      Serial.flush();
      log_w("RTC is NOT initialized");
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    } else {
      synced = true;
      syncFromRTC();
      Serial.println(F("SystemTime set from RTC"));
    }
}


// float RTC:getTemperature() { rtclock.getTemperature(); }

#elif defined(HAS_BM8563)
  bool RTC::setup() {
    log_d("RTC::BM8563_setup");
  }
#endif

void RTC::syncFromRTC() {
  // 1. Read time from the PCF8523
  
  #ifdef HAS_PCF8523 
    DateTime now = rtclock.now();
  #endif

  // 2. Populate the standard C tm structure
  struct tm tm_time;
  tm_time.tm_year = now.year() - 1900; // Years since 1900
  tm_time.tm_mon  = now.month() - 1;   // Months (0-11)
  tm_time.tm_mday = now.day();         // Day of the month (1-31)
  tm_time.tm_hour = now.hour();        // Hours (0-23)
  tm_time.tm_min  = now.minute();      // Minutes (0-59)
  tm_time.tm_sec  = now.second();      // Seconds (0-59)
  
  // Set isdst to -1 to let the system decide based on your timezone settings
  tm_time.tm_isdst = -1;

  // 3. Convert tm to time_t and configure settimeofday
  time_t t = mktime(&tm_time);
  struct timeval tv = { .tv_sec = t, .tv_usec = 0 };
  
  settimeofday(&tv, NULL);
  synced = true;
  Serial.println("ESP32 system time successfully synced to PCF8523 RTC!");
}


// Function to set system time from an ISO 8601 string (e.g., "2026-06-11T13:26:00")
bool RTC::getSystemTimeFromString(const char* timeStr) {
    struct tm timeinfo = {0};
    
    // Parse the string into the tm struct using format specifiers
    // %Y = Year, %m = Month, %d = Day, %H = Hour, %M = Minute, %S = Second
    if (strptime(timeStr, "%Y-%m-%dT%H:%M:%S", &timeinfo) == NULL) {
        Serial.println("Failed to parse time string.");
        return false;
    }
    
    // Convert tm struct to Unix timestamp (seconds since Jan 1 1970)
    time_t epochTime = mktime(&timeinfo);
    if (epochTime == -1) {
        Serial.println("Failed to convert tm to epoch time.");
        return false;
    }
    
    // Structure required by settimeofday
    struct timeval tv;
    tv.tv_sec = epochTime;
    tv.tv_usec = 0;        // Microseconds
    
    // Apply time directly to ESP32 internal clock
    if (settimeofday(&tv, NULL) != 0) {
        Serial.println("Failed to update system time.");
        return false;
    }
    
    synced = true;
    Serial.println("System time updated successfully!");
    return true;
}



bool RTC::sync_rtc_ntp() {
  struct tm timeinfo;

  if (!supported) {
    log_w("RTC not supported");
    return false;
  }

  if (!wifi_scan_obj.wifi_connected) {
    log_w("Wifi Not Connected");
    return false;
  }

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  delay(1000);

  if (!getLocalTime(&timeinfo)) {
    log_w("Failed to obtain time from NTP");
    return false;
  }
  synced = true;

  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  Serial.println("RTC successfully set via NTP!");

 time_t now;
  time(&now);
  struct tm* timeStruct = localtime(&now);

  DateTime ntpTime(
    timeStruct->tm_year + 1900, // Year starts from 1900
    timeStruct->tm_mon + 1,     // Month (1-12)
    timeStruct->tm_mday,        // Day of the month
    timeStruct->tm_hour,        // Hour
    timeStruct->tm_min,         // Minute
    timeStruct->tm_sec          // Second
  );

  #ifdef HAS_PCF8523
    rtclock.adjust(ntpTime);
    Serial.println("PCF8523 RTC updated with NTP time.");
    // log_d("PCF8523 RTC updated with NTP time.");
  #endif

  /*
  rtclock.adjust(DateTime(
    timeinfo.tm_year + 1900, 
    timeinfo.tm_mon + 1, 
    timeinfo.tm_mday, 
    timeinfo.tm_hour, 
    timeinfo.tm_min, 
    timeinfo.tm_sec
  ));
  */

  return true;

}


void RTC::setSystemTimeFromCompile() {
    struct tm tm_build;

    // Parse built-in compile time macros
    strptime(__DATE__, "%b %d %Y", &tm_build);
    strptime(__TIME__, "%T", &tm_build);
    
    // Set as UTC
    tm_build.tm_isdst = -1; 
    time_t t_of_day = mktime(&tm_build);

    // Update the system time
    struct timeval tv = { .tv_sec = t_of_day, .tv_usec = 0 };
    settimeofday(&tv, NULL);
}

String RTC::dt_string() {

  if (!supported) {
    log_w("RTC not supported");
    return "";
  }

  char format[] = "YYYY-MM-DD hh:mm:ss";
  
  DateTime dt = rtclock.now();
  // Serial.println(dt);
  // Serial.flush();

  return rtclock.now().toString(format);
}

String RTC::millis_dt_string() {   // punt
  unsigned long currentMillis = millis(); 

  unsigned long seconds = currentMillis / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;
  unsigned long days = hours / 24;

  // Use modulo (%) to get the remainder for each unit
  seconds %= 60;
  minutes %= 60;
  hours %= 24;

  String timeString = String(days) + "d ";
    
  if (hours < 10) timeString += "0";
    timeString += String(hours) + ":";
          
  if (minutes < 10) timeString += "0";
    timeString += String(minutes) + ":";
                
  if (seconds < 10) timeString += "0";
    timeString += String(seconds);

  return timeString;
}


#endif //HAS_RTC

