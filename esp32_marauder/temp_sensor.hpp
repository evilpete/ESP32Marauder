#include "configs.h"

#pragma once

#ifndef temp_sensor_hpp
#define temp_sensor_hpp


//do we have a SENSOR?
#if defined(HAS_TEMP_SENSOR) && \
    !( defined(HAS_CPU_TEMP) || defined(HAS_SHTC3) || defined(HAS_AHT20) )
  #warning "HAS_TEMP_SENSOR set without source device HAS_SHTC3 or HAS_CPU_TEMP"
  #undef HAS_TEMP_SENSOR
#endif


#if defined(HAS_TEMP_SENSOR)

#include <Arduino.h>
#include <Wire.h>

  // ESP_IDF_VERSION_MAJOR ESP_ARDUINO_VERSION_MAJOR
  #if (defined(ESP_IDF_VERSION_MAJOR) && (ESP_IDF_VERSION_MAJOR >= 5)) && \
        (defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32S3) \
        || defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C5) \
        || defined(CONFIG_IDF_TARGET_ESP32C6) || defined(CONFIG_IDF_TARGET_ESP32H2) )
      #define HAS_CPU_TEMP
  #else
    #undef HAS_CPU_TEMP
  #endif

  #if defined(HAS_SHTC3)
      #include <SHTC3.hpp>
      SHTC3 SHTC3_obj;
  #elif defined (HAS_CPU_TEMP)
      #include "cpu_temp_sensor.hpp"
  #endif


  class TempSensor {
     public:
      bool supported = false;
      TwoWire *_wire;
      uint32_t lastRead = 0;

      void RunSetup(TwoWire *wireInstance = nullptr);
      float temperature();
  };  //  class TempSensor


#if !defined(HAS_SHTC3) && !defined(HAS_CPU_TEMP)
  inline void TempSensor::RunSetup(TwoWire *wireInstance) {}
  inline float TempSensor::temperature() { return 0.0; }
#else

  inline void TempSensor::RunSetup(TwoWire *wireInstance) {
    if (wireInstance == nullptr)
      _wire = &Wire;
    else
      _wire = wireInstance;

    #if defined(HAS_SHTC3)
        this->supported = SHTC3_obj.begin(_wire);
    #elif defined(HAS_HAS_AHT20)
      // noop
    #elif defined (HAS_CPU_TEMP)
        this->supported = init_sys_temp();
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
      #elif defined (HAS_CPU_TEMP)
        if (now - lastRead < 60000)
          return get_sys_temperature();

        lastRead = now;
        return read_sys_temp();
      #else init
        return 0.0;
      #endif
  }

#endif  //  !HAS_SHTC3  !HAS_CPU_TEMP

#endif    // HAS_TEMP_SENSOR

#endif    // temp_sensor_hpp
