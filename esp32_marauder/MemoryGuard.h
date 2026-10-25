#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef ARDUINO
#include "freertos/FreeRTOS.h"
#include "configs.h"   // for board macros (e.g. MARAUDER_LOW_INTERNAL_RAM)
#endif

namespace marauder {

// Memory-constrained targets (e.g. ESP32-C5 boards that also drive a display)
// keep very little internal SRAM free at runtime, well below the standard
// floor, so the guard would latch paused permanently and block all WiFi/BLE
// scanning. Such boards define MARAUDER_LOW_INTERNAL_RAM (see configs.h) to
// lower the floor to their real headroom. The numbers can be overridden
// per board via the MARAUDER_MEMGUARD_* macros; tune against the
// "[memguard] free=... largest=..." log on the target.
#if defined(MARAUDER_LOW_INTERNAL_RAM)
  #ifndef MARAUDER_MEMGUARD_PAUSE_FREE
    #define MARAUDER_MEMGUARD_PAUSE_FREE     (10 * 1024)
  #endif
  #ifndef MARAUDER_MEMGUARD_RESUME_FREE
    #define MARAUDER_MEMGUARD_RESUME_FREE    (14 * 1024)
  #endif
  #ifndef MARAUDER_MEMGUARD_PAUSE_LARGEST
    #define MARAUDER_MEMGUARD_PAUSE_LARGEST  (6 * 1024)
  #endif
  #ifndef MARAUDER_MEMGUARD_RESUME_LARGEST
    #define MARAUDER_MEMGUARD_RESUME_LARGEST (8 * 1024)
  #endif
#else
  #define MARAUDER_MEMGUARD_PAUSE_FREE     (24 * 1024)
  #define MARAUDER_MEMGUARD_RESUME_FREE    (40 * 1024)
  #define MARAUDER_MEMGUARD_PAUSE_LARGEST  (8 * 1024)
  #define MARAUDER_MEMGUARD_RESUME_LARGEST (12 * 1024)
#endif

struct MemoryGuardThresholds {
  size_t pauseFreeBytes = MARAUDER_MEMGUARD_PAUSE_FREE;
  size_t resumeFreeBytes = MARAUDER_MEMGUARD_RESUME_FREE;
  size_t pauseLargestBlockBytes = MARAUDER_MEMGUARD_PAUSE_LARGEST;
  size_t resumeLargestBlockBytes = MARAUDER_MEMGUARD_RESUME_LARGEST;
};

class MemoryGuardState {
 public:
  explicit MemoryGuardState(
      MemoryGuardThresholds thresholds = MemoryGuardThresholds{});

  bool allow(size_t freeBytes, size_t largestBlockBytes,
             size_t requestedBytes = 0);
  bool paused() const;

 private:
  MemoryGuardThresholds thresholds_;
  bool paused_ = false;
};

class QueueBackpressureState {
 public:
  explicit QueueBackpressureState(size_t limit);
  bool allow(size_t currentSize);
  bool paused() const;

 private:
  size_t limit_;
  size_t resume_limit_;
  bool paused_ = false;
};

#ifdef ARDUINO
class RuntimeMemoryGuard {
 public:
  static RuntimeMemoryGuard& instance();

  bool allow(size_t requestedBytes = 0);
  bool paused() const;
  uint32_t rejectedAllocations() const;

 private:
  MemoryGuardState state_;
  uint32_t rejected_allocations_ = 0;
  mutable portMUX_TYPE mux_ = portMUX_INITIALIZER_UNLOCKED;
};

template <typename List, typename Value>
bool memoryGuardedAdd(List* list, const Value& value,
                      size_t extraReserveBytes = 32) {
  if (list == nullptr) return false;
  const size_t requested = sizeof(Value) + sizeof(void*) + extraReserveBytes;
  if (!RuntimeMemoryGuard::instance().allow(requested)) return false;
  return list->add(value);
}
#endif

}  // namespace marauder
