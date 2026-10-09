#pragma once

#include "configs.h"
#include "esp_idf_version.h"

// ESP32-C2 | ESP32-C3 | ESP32-C5 | ESP32-C6 | ESP32-C61 | ESP32-H2 | ESP32-P4 | ESP32-S2 | ESP32-S3 |

#ifndef __cpu_temp_sensor_hpp__
#define __cpu_temp_sensor_hpp__ 1

#if defined(HAS_CPU_TEMP)


#if !defined(CONFIG_IDF_TARGET_ESP32)
  #include "driver/temperature_sensor.h"
  inline temperature_sensor_handle_t temp_handle = NULL;
#else
  #include "driver/temp_sensor.h"
#endif

// #ifdef __cplusplus
//   // extern uint8_t temprature_sens_read();
//   extern "C" {
//       uint8_t temprature_sens_read();
//   }
// #endif

// uint8_t temprature_sens_read();

inline float _celsius = 0.0;
inline bool already_inited = false;

  inline bool init_sys_temp() {
    esp_err_t x = 0;
    float y = 0.0;

    log_d("init_sys_temp");
    if ( already_inited ) {
      log_d("cpu_temp_sensor: already running");
      return true;
    }

    #if !defined(CONFIG_IDF_TARGET_ESP32)
        log_d("cpu_temp_sensor: Init");

        // Configure sensor range (e.g., -10<C2><B0>C to 80<C2><B0>C)
        temperature_sensor_config_t temp_sensor_config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(-10, 80);

        // Install and enable the internal sensor
        x = ESP_ERROR_CHECK_WITHOUT_ABORT(temperature_sensor_install(&temp_sensor_config, &temp_handle));

        log_d("cpu_temp_sensor: x = %d", x);
        // ESP_ERR_INVALID_STATE implies The framework already allocated it!
        // thus  can safely proceed knowing it is up and running.
        if (x != ESP_OK && x != ESP_ERR_INVALID_STATE) return false;

        x = ESP_ERROR_CHECK_WITHOUT_ABORT(temperature_sensor_enable(temp_handle));

        log_d("cpu_temp_sensor: x = %d", x);
      #else
        // Default range is -10°C to 80°C (TSENS_DAC_L2)
        temp_sensor_config_t config = TSENS_CONFIG_DEFAULT();

        x = ESP_ERROR_CHECK_WITHOUT_ABORT(temp_sensor_set_config(config));
        log_d("cpu_temp_sensor: x = %d", x);
        if (x != ESP_OK) return false;

        x = ESP_ERROR_CHECK_WITHOUT_ABORT(temp_sensor_start());

         // temp_sensor_read_celsius(&_celsius);
         // log_d("get_celsius = %0.1f", _celsius);

      #endif

       // y  = temprature_sens_read();
       // log_d("temprature_sens_read = %0.1f", y);


      if (x == ESP_OK) return false;

      already_inited = true;

    return true;
  }

  inline float get_sys_temperature() { return _celsius; }


  inline float read_sys_temp() {
    if (!already_inited)
      return 0.0;

    #if !defined(CONFIG_IDF_TARGET_ESP32)

      // Read the temperature in Celsius
      esp_err_t x = temperature_sensor_get_celsius(temp_handle, &_celsius);

    #else

      esp_err_t x = temp_sensor_read_celsius(&_celsius);

    #endif

      if (x == ESP_OK)
        return _celsius;

      ESP_ERROR_CHECK_WITHOUT_ABORT(x);
      log_d("Error reading cpu_temp_sensor");

      return 0.0;
  }



  inline void disable_sys_temp() {
    if (!already_inited) return;

    #if !defined(CONFIG_IDF_TARGET_ESP32)
      temperature_sensor_disable(temp_handle);
    #else
    extern esp_err_t temp_sensor_stop(void);
      temp_sensor_stop();
    #endif
    already_inited = false;
  }

#endif     // HAS_CPU_TEMP

#endif    // __cpu_temp_sensor_hpp__
