#pragma once

#ifndef PowerMgmt_hpp
#define PowerMgmt_hpp

// CPU frequency / Dynamic Frequency Scaling helpers.
//
// On builds where the Arduino libs were compiled with CONFIG_PM_ENABLE and the
// board defines HAS_PM, the IDF power manager owns the CPU clock: calling
// setCpuFrequencyMhz() bypasses it and gets overridden by DFS.  All CPU clock
// changes go through pm_set_cpu_freq() so the right mechanism is used.
//
// DFS keeps the CPU at max_freq while any non-idle task runs (and while
// WiFi / BLE hold their PM locks), so getCpuFrequencyMhz() nearly always
// reads max.  Use pm_dump_locks() (needs CONFIG_PM_PROFILING) to see the
// time actually spent at each frequency.

#include <Arduino.h>
#include "configs.h"
#include "esp_idf_version.h"

#if ( defined(CORE_DEBUG_LEVEL) && CORE_DEBUG_LEVEL > 1) || defined(DEVELOPER)
  #include "debug_func.hpp"
#endif


#include "esp_pm.h"

  #if ESP_IDF_VERSION_MAJOR >= 5  // ESP-IDF 5.x
    #define PM_CONFIG_TYPE esp_pm_config_t
  #elif defined(CONFIG_IDF_TARGET_ESP32S3) // ESP-IDF 4.4 + ESP32-S3
    #define PM_CONFIG_TYPE esp_pm_config_esp32s3_t
  #elif defined(CONFIG_IDF_TARGET_ESP32S2) // ESP-IDF 4.4 + ESP32-S2
    #define PM_CONFIG_TYPE esp_pm_config_esp32s2_t
  #elif defined(CONFIG_IDF_TARGET_ESP32C3) // ESP-IDF 4.4 + ESP32-C3
    #define PM_CONFIG_TYPE esp_pm_config_esp32c3_t
  #elif defined(CONFIG_IDF_TARGET_ESP32C6) // ESP-IDF 4.4 + ESP32-C6
    #define PM_CONFIG_TYPE esp_pm_config_esp32c6_t
  #elif defined(CONFIG_IDF_TARGET_ESP32) // ESP-IDF 4.4 + original ESP32
    #define PM_CONFIG_TYPE esp_pm_config_esp32_t
  #endif

  #ifndef PM_MAX_FREQ
    #define PM_MAX_FREQ 240
  #endif
  #ifndef PM_MIN_FREQ
    #define PM_MIN_FREQ 160
  #endif
  #if PM_MIN_FREQ < 80
    #define PM_MIN_FREQ 80
  #endif

inline esp_err_t enable_pm() {

#ifdef CONFIG_PM_ENABLE

  #ifdef USE_PM
    PM_CONFIG_TYPE pm_config = {
      .max_freq_mhz = PM_MAX_FREQ
      .min_freq_mhz = PM_MIN_FREQ,
    #ifdef CONFIG_FREERTOS_USE_TICKLESS_IDLE
      .light_sleep_enable = true
    #else
      .light_sleep_enable = false
    #endif
    };

    log_d("esp_pm_configure max=%d min=%d sleep_enable=%s", pm_config.max_freq_mhz, pm_config.min_freq_mhz,
        pm_config.light_sleep_enable ? "true" : "false");

    esp_err_t err = ESP_ERROR_CHECK_WITHOUT_ABORT(esp_pm_configure(&pm_config));


    return err;
  #endif  // USE_PM
#endif    //  CONFIG_PM_ENABLE

}

#endif // PowerMgmt_hpp
