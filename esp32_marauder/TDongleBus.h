#pragma once
#include "configs.h"

#ifdef HAS_T_DONGLE_DISPLAY

#include <stdint.h>

void deselectTDongleSharedSpi(uint8_t tftCsPin, uint8_t sdCsPin);

#endif
