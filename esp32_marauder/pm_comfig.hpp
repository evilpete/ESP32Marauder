#pragma once

#ifndef _PM_CONFIG_
#define _PM_CONFIG_

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
    #define PM_MIN_FREQ 80
  #endif
  #if PM_MIN_FREQ < 80
    #define PM_MIN_FREQ 80
  #endif

// Adjust based on your calibration
#define TP_THRESHOLD = 40;
#ifdef XPT2046_IRQ
    #define TP_INT XPT2046_IRQ
#endif

inline void int_callback() {
  // Optional interrupt callback
  }
i

inline esp_err_t enable_pm() {

  esp_log_level_set("pm", ESP_LOG_VERBOSE);       // For power management module
  esp_log_level_set("cpu_freq", ESP_LOG_VERBOSE); // For dynamic frequency scaling


#ifdef CONFIG_PM_ENABLE
  log_d("CONFIG_PM_ENABLE Set");

  #if defined(CONFIG_FREERTOS_USE_TICKLESS_IDLE) && defined(TP_INT) && TFT_INT != -1
    // Initialize touch interrupt
    touchAttachInterrupt(TP_INT, callback, TP_THRESHOLD);

    // Enable touch pad as light/deep sleep wake source
    esp_sleep_enable_touchpad_wakeup();
  #endif


  #ifdef CONFIG_FREERTOS_USE_TICKLESS_IDLE
    log_d("CONFIG_FREERTOS_USE_TICKLESS_IDLE Set");
  #else
    log_d("CONFIG_FREERTOS_USE_TICKLESS_IDLE NOT Set");
  #endif

  #ifdef CORE_DEBUG_LEVEL
    log_d("CORE_DEBUG_LEVEL Set = %d", CORE_DEBUG_LEVEL);
  #else
    log_d("CORE_DEBUG_LEVEL NOT Set");
  #endif

  #ifdef DEVELOPER
    log_d("DEVELOPER Set");
  #else
    log_d("DEVELOPER NOT Set");
  #endif

  #ifdef HAS_PWR_MGMT
    log_d("HAS_PWR_MGMT Set");
  #else
    log_d("HAS_PWR_MGMT NOT Set");
  #endif

  #ifdef HAS_PM
    log_d("HAS_PM Set");
  #else
    log_d("HAS_PM NOT Set");
  #endif

  #ifdef CONFIG_PM_RETAIN_PERIPH_IN_LIGHT_SLEEP
    log_d("CONFIG_PM_RETAIN_PERIPH_IN_LIGHT_SLEEP = %d", CONFIG_PM_RETAIN_PERIPH_IN_LIGHT_SLEEP);
  #else
    log_d("CONFIG_PM_RETAIN_PERIPH_IN_LIGHT_SLEEP NOT Set");
  #endif

  #ifdef CONFIG_USJ_NO_AUTO_LS_ON_CONNECTION
    log_d("CONFIG_USJ_NO_AUTO_LS_ON_CONNECTION = %d", CONFIG_USJ_NO_AUTO_LS_ON_CONNECTION);
  #else
    log_d("CONFIG_USJ_NO_AUTO_LS_ON_CONNECTION NOT Set");
  #endif

  #ifdef CONFIG_RTC_CLOCK_BBPLL_POWER_ON_WITH_USB
    log_d("CONFIG_RTC_CLOCK_BBPLL_POWER_ON_WITH_USB = %d", CONFIG_RTC_CLOCK_BBPLL_POWER_ON_WITH_USB);
  #else
    log_d("CONFIG_RTC_CLOCK_BBPLL_POWER_ON_WITH_USB NOT Set");
  #endif

  #ifdef CONFIG_PM_POWER_DOWN_PERIPHERAL_IN_LIGHT_SLEEP
    log_d("CONFIG_PM_POWER_DOWN_PERIPHERAL_IN_LIGHT_SLEEP = %d", CONFIG_PM_POWER_DOWN_PERIPHERAL_IN_LIGHT_SLEEP);
  #else
    log_d("CONFIG_PM_POWER_DOWN_PERIPHERAL_IN_LIGHT_SLEEP NOT Set");
  #endif

  #ifdef CONFIG_PM_POWER_DOWN_CPU_IN_LIGHT_SLEEP
    log_d("CONFIG_PM_POWER_DOWN_CPU_IN_LIGHT_SLEEP = %d", CONFIG_PM_POWER_DOWN_CPU_IN_LIGHT_SLEEP);
  #else
    log_d("CONFIG_PM_POWER_DOWN_CPU_IN_LIGHT_SLEEP NOT Set");
  #endif

  // Force RTC peripherals to stay powered on during sleep
  esp_err_t err = esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
  if (err) ESP_ERROR_CHECK_WITHOUT_ABORT(err);

  // Force the main internal BBPLL clock source to stay active 
  // This prevents the system clock tree from switching down to slower RC/XTAL oscillators
  err = esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_ON); 
  if (err) ESP_ERROR_CHECK_WITHOUT_ABORT(err);

  // Force the XTAL oscillator to stay ON during sleep 
  // (Useful if peripherals like LEDC or ADC require it as a clock source)
  err = esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_ON);
  if (err) ESP_ERROR_CHECK_WITHOUT_ABORT(err);

  #if defined(HAS_PM) || defined(HAS_PWR_MGMT)
    PM_CONFIG_TYPE pm_config = {
      .max_freq_mhz = PM_MAX_FREQ,
      .min_freq_mhz = PM_MIN_FREQ,
     #ifdef CONFIG_FREERTOS_USE_TICKLESS_IDLE
       .light_sleep_enable = true
     #else
      .light_sleep_enable = false
     #endif
    };

    log_d("esp_pm_configure max=%d min=%d sleep_enable=%s", pm_config.max_freq_mhz, pm_config.min_freq_mhz,
        pm_config.light_sleep_enable ? "true" : "false");

    err = ESP_ERROR_CHECK_WITHOUT_ABORT(esp_pm_configure(&pm_config));

    delay(50);


    return err;
  #endif  // HAS_PM
#endif    //  CONFIG_PM_ENABLE

}

#if  defined(CONFIG_PM_ENABLE) && defined(CONFIG_PM_PROFILING)
  // Print PM lock / frequency-mode statistics to the serial console.
  inline void pm_dump_locks() {
    #if defined(HAS_PM) && defined(CONFIG_PM_PROFILING)
      fflush(stdout);
      esp_pm_dump_locks(stdout);
      fflush(stdout);
    #elif defined(HAS_PM)
      Serial.println(F("PM lock dump needs CONFIG_PM_PROFILING=y in the Arduino libs"));
    #else
      Serial.println(F("Power management not enabled in this build"));
    #endif
      Serial.print(F("CpuFrequency = "));
      Serial.print(getCpuFrequencyMhz());
      Serial.println(F(" Mhz"));
    }
#endif

#endif // _PM_CONFIG_
