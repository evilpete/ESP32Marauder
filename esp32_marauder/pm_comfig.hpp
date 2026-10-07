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

// esp_pm_config_t is the IDF 5.x name; IDF 4.x used per-target structs.
#if defined(HAS_PM) && defined(CONFIG_PM_ENABLE) && ESP_IDF_VERSION_MAJOR >= 5
  #define MARAUDER_USE_PM 1
  #include "esp_pm.h"
#endif

// Defaults: min 160 keeps APB at 80MHz so SPI (TFT / SD) clock dividers stay valid.
#ifndef PM_MAX_FREQ
  #define PM_MAX_FREQ 240
#endif
#ifndef PM_MIN_FREQ
  #define PM_MIN_FREQ 160
#endif
#if PM_MIN_FREQ < 80
  #error "PM_MIN_FREQ below 80MHz drops APB below 80MHz and breaks SPI clocks"
#endif

// Set the CPU clock range.  With PM, DFS scales between min_mhz and max_mhz
// (pass min == max to pin the clock).  Without PM, max_mhz is applied directly.
static inline esp_err_t pm_set_cpu_freq(uint32_t max_mhz, uint32_t min_mhz = 0) {
  if (min_mhz == 0 || min_mhz > max_mhz)
    min_mhz = max_mhz;

#ifdef MARAUDER_USE_PM
  esp_pm_config_t pm_config = {
    .max_freq_mhz = (int)max_mhz,
    .min_freq_mhz = (int)min_mhz,
  #ifdef PM_LIGHT_SLEEP
    .light_sleep_enable = true
  #else
    .light_sleep_enable = false
  #endif
  };
  esp_err_t err = ESP_ERROR_CHECK_WITHOUT_ABORT(esp_pm_configure(&pm_config));
  log_d("esp_pm_configure(%lu/%lu) = %s", max_mhz, min_mhz, esp_err_to_name(err));
  return err;
#else
  return setCpuFrequencyMhz(max_mhz) ? ESP_OK : ESP_ERR_INVALID_ARG;
#endif
}

// Apply the board's default DFS range (PM_MAX_FREQ / PM_MIN_FREQ).
static inline esp_err_t pm_set_default_freq() {
  return pm_set_cpu_freq(PM_MAX_FREQ, PM_MIN_FREQ);
}

// Print PM lock / frequency-mode statistics to the serial console.
static inline void pm_dump_locks() {
#if defined(MARAUDER_USE_PM) && defined(CONFIG_PM_PROFILING)
  fflush(stdout);
  esp_pm_dump_locks(stdout);
  fflush(stdout);
#elif defined(MARAUDER_USE_PM)
  Serial.println(F("PM lock dump needs CONFIG_PM_PROFILING=y in the Arduino libs"));
#else
  Serial.println(F("Power management not enabled in this build"));
#endif
  Serial.print(F("CpuFrequency = "));
  Serial.print(getCpuFrequencyMhz());
  Serial.println(F(" Mhz"));
}

#endif // PowerMgmt_hpp
