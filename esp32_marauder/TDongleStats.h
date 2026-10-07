#pragma once

#include "configs.h"

#ifdef HAS_T_DONGLE_DISPLAY

#include <stdint.h>

class TDongleStats {
 public:
  static const char* modeLabel(uint8_t mode);
};

#endif
