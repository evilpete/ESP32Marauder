// #include "configs.h"

#pragma once

#ifndef temp_sensor_hpp
#define temp_sensor_hpp

#if defined(HAS_TEMP_SENSOR)

#pragma GCC diagnostic warning "-Wcpp"


  /*
  // ESP_IDF_VERSION_MAJOR ESP_ARDUINO_VERSION_MAJOR
  #if (defined(ESP_IDF_VERSION_MAJOR) && (ESP_IDF_VERSION_MAJOR >= 5)) && \
        (defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32S3) \
        || defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C5) \
        || defined(CONFIG_IDF_TARGET_ESP3) \
        || defined(CONFIG_IDF_TARGET_ESP32C6) || defined(CONFIG_IDF_TARGET_ESP32H2))
      #define HAS_CPU_TEMP
  #else
    #undef HAS_CPU_TEMP
  #endif
  */

//do we have a SENSOR?
#if defined(HAS_TEMP_SENSOR) && \
    !(defined(HAS_CPU_TEMP) || defined(HAS_SHTC3) || defined(HAS_AHT20))
  #undef HAS_TEMP_SENSOR
  #error here
#endif



#include <Arduino.h>
#include <Wire.h>


#if defined(HAS_SHTC3)
    #include <SHTC3.hpp>
#endif
#if defined(HAS_CPU_TEMP)
    #include "cpu_temp_sensor.hpp"
#endif

  class TempSensor {
   public:
       ~TempSensor() {
         #if defined(HAS_CPU_TEMP)
           disable_sys_temp();
         #endif
       }

      bool supported = false;
      bool cpu_supported = false;
      TwoWire *_wire;
      uint32_t lastRead = 0;

      void RunSetup(TwoWire *wireInstance = nullptr);
      float temperature();
      float cpu_temperature();
  };  //  class TempSensor


#if !defined(HAS_SHTC3) && !defined(HAS_CPU_TEMP)
  inline void TempSensor::RunSetup(TwoWire *wireInstance) { log_d("!! TempSensor::RunSetup"); }
  inline float TempSensor::temperature() { return 0.0; }


  #if defined(HAS_CPU_TEMP)
    log_d("HAS_CPU_TEMP is true");
  #else
    log_d("HAS_CPU_TEMP is false");
  #endif

#else

  inline void TempSensor::RunSetup(TwoWire *wireInstance) {
    if ( supported ) {
      log_d("TempSensor already started");
    }

    if (wireInstance == nullptr)
      _wire = &Wire;
    else
      _wire = wireInstance;

    #if defined(HAS_SHTC3)
        this->supported = SHTC3_obj.begin(_wire);
        log_d("HAS_SHTC3 supported = %d", this->supported);
    #elif defined(HAS_HAS_AHT20)
      // noop
    #endif

    #if defined(HAS_CPU_TEMP)
      this->cpu_supported = init_sys_temp();
      if (!this->supported)
        this->supported = this->cpu_supported;
      log_d("HAS_CPU_TEMP supported = %d", this->supported);
    #else
        this->supported = false;
    #endif
  }

  inline float TempSensor::temperature() {
    if (!supported) return 0.0;

    uint32_t now = millis();

      #if defined(HAS_SHTC3)
        if (now - lastRead < 60000)
          return SHTC3_obj.temperature();
        SHTC3_obj.read();
        lastRead = now;
        return SHTC3_obj.temperature();
      #elif defined(HAS_HAS_AHT20)
        return 0.0;
      #elif defined(HAS_CPU_TEMP)
        if (now - lastRead < 60000)
          return get_sys_temperature();

        lastRead = now;
        return read_sys_temp();
      #else
        return 0.0;
      #endif
  }

  inline float TempSensor::cpu_temperature() {
    #if defined(HAS_CPU_TEMP)
      if (this->cpu_supported)
        return read_sys_temp();
    #endif
      return 0.0;
  }



inline TempSensor TempSensor_obj;

#endif  //  !HAS_SHTC3  !HAS_CPU_TEMP

#endif    // HAS_TEMP_SENSOR

#endif    // temp_sensor_hpp
