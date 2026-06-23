#include <Preferences.h>

#include "configs.h"

#include "Display.h"
/*
  Three #ifdef blocks defining brightnessInit() brightnessOn brightnessOff... etc...
    ifdef HAS_AW9364
    else ledcAttach
    else just digitalWrite
    else dummy/noop versions

    // Every block provides the following
     uint8_t BL_NUM_LEVELS
    void brightnessInit()
    void brightnessCycle()
    void backlightOn()
    void backlightOff()
    uint8_t getBrightnessLevel()
    void brightnessSet(uint8_t level)
    void brightnessSave(uint8_t level)

*/

#if defined(HAS_SCREEN) && !defined(TFT_BL)
  #warning "HAS_SCREEN is defined and TFT_BL is undefined"
#endif


#if defined(HAS_SCREEN) && defined(TFT_BL)


Preferences bl_prefs;
uint8_t bl_level_idx = 9; // default brightness

// HAS_AW9364 used by MARAUDER_CYD_HMI
// AW9364: 4-Channel 1-wire Dimming LED Driver
#if defined(HAS_AW9364)

   uint8_t BL_NUM_LEVELS = 16;

  void _setBrightness(uint8_t value)
  {
      static uint8_t _brightness = 0;

      if (_brightness == value) {
          return;
      }

      if (value > 16) {
          value = 16;
      }
      if (value == 0) {
          digitalWrite(TFT_BL, 0);
          delay(3);
          _brightness = 0;
          return;
      }
      if (_brightness == 0) {
          digitalWrite(TFT_BL, 1);
          _brightness = BL_NUM_LEVELS;
          delayMicroseconds(30);
      }
      int from = BL_NUM_LEVELS - _brightness;
      int to = BL_NUM_LEVELS - value;
      int num = (BL_NUM_LEVELS + to - from) % BL_NUM_LEVELS;
      for (int i = 0; i < num; i++) {
          digitalWrite(TFT_BL, 0);
          digitalWrite(TFT_BL, 1);
      }
      _brightness = value;
  }

  void brightnessInit() {
    log_d("HAS_AW9364 brightnessInit TFT_BL = %d", TFT_BL);
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, 1);
    // ledDriver.begin(TFT_BL);
    bl_prefs.begin("backlight", false);
    bl_level_idx = bl_prefs.getUChar("level", BL_NUM_LEVELS);
    if (bl_level_idx > BL_NUM_LEVELS) bl_level_idx = BL_NUM_LEVELS;
    log_d("HAS_AW9364 brightnessInit level = %d ", bl_level_idx);
    // ledDriver.setBrightness(bl_level_idx);
    // bl_level_idx = BL_NUM_LEVELS;
  }

  void brightnessCycle() {
      bl_level_idx = (bl_level_idx + 1) % BL_NUM_LEVELS;
      _setBrightness(bl_level_idx);
      bl_prefs.putUChar("level", bl_level_idx);
      log_d("[Brightness] Level %d / %.0f %%", (bl_level_idx + 1), (static_cast<float>(bl_level_idx / BL_NUM_LEVELS) * 100));
  }

  uint8_t getBrightnessLevel() {
    log_d("HAS_AW9364 getBrightnessLevel level =  %d ", bl_level_idx);
    return bl_level_idx;
  }

  void brightnessSet(uint8_t level) {
      log_d("HAS_AW9364 brightnessSet level = %d ", level);
      if (level > BL_NUM_LEVELS) level = BL_NUM_LEVELS;
      bl_level_idx = level;
      _setBrightness(bl_level_idx);
      // bl_prefs.putUChar("level", bl_level_idx);
  }

  void brightnessSave(uint8_t level) {
      log_d("HAS_AW9364 brightnessSave level = %d ", level);
      bl_level_idx = level;
      brightnessSet(level);
      bl_prefs.putUChar("level", bl_level_idx);
  }

  void backlightOn() {
    log_d("HAS_AW9364 backlightOn: %d", bl_level_idx);
    digitalWrite(TFT_BL, 1);
    if (bl_level_idx < 3) bl_level_idx = 3;
    _setBrightness(bl_level_idx);
  }

  void backlightOff() {
    log_d("HAS_AW9364 backlightOff: %d", bl_level_idx);
    digitalWrite(TFT_BL, 0);
    // _setBrightness(0);
  }


  // Init PWM brightness AFTER display init (so ledcAttach overrides TFT_eSPI's pinMode)
  // #ifndef HAS_MINI_SCREEN


#elif !defined(HAS_MINI_SCREEN) 

  // PWM Brightness Control
  const uint8_t BL_LEVELS[] = {26, 51, 77, 102, 128, 153, 179, 204, 230, 255, 255};
  uint8_t BL_NUM_LEVELS = 11;

  #define BL_CHANNEL 0
  #define BL_FREQ 5000
  #define BL_RESOLUTION 8

  // Helper macros for LEDC API compatibility (2.x vs 3.x board package)
  #if !defined(HAS_MINI_SCREEN) && !defined(HAS_AW9364)
    #if ESP_ARDUINO_VERSION_MAJOR >= 3
      #define BL_SETUP()       ledcAttach(TFT_BL, BL_FREQ, BL_RESOLUTION)
      #define BL_SET(duty)     ledcWrite(TFT_BL, (duty))
      #define TLED          TFT_BL
    #else
      #define BL_SETUP()       do { ledcSetup(BL_CHANNEL, BL_FREQ, BL_RESOLUTION); ledcAttachPin(TFT_BL, BL_CHANNEL); } while(0)
      #define BL_SET(duty)     ledcWrite(BL_CHANNEL, (duty))
      #define TLED          BL_CHANNEL
    #endif
  #endif

  void brightnessInit() {
      pinMode(TFT_BL, OUTPUT);
      BL_SETUP();
      bl_prefs.begin("backlight", false);
      bl_level_idx = bl_prefs.getUChar("level", 9);
      if (bl_level_idx >= BL_NUM_LEVELS) bl_level_idx = 9;
      BL_SET(BL_LEVELS[bl_level_idx]);
      uint32_t currentDuty = ledcRead(BL_CHANNEL);

      log_d("brightnessInit: level = %d currentDuty=%d", bl_level_idx, currentDuty);
      log_d("    BL_CHANNEL = %d\n", BL_CHANNEL);
      log_d("    TFT_BL = %d\n", TFT_BL);
      log_d("    BL TLED = %d\n", TLED);

  }

  void brightnessCycle() {
      bl_level_idx = (bl_level_idx + 1) % BL_NUM_LEVELS;
      BL_SET(BL_LEVELS[bl_level_idx]);
      bl_prefs.putUChar("level", bl_level_idx);
      Serial.print(F("[Brightness] Level "));
      Serial.print(bl_level_idx + 1);
      Serial.print(F("/"));
      Serial.print(BL_NUM_LEVELS);
      Serial.print(F(" ("));
      Serial.print(BL_LEVELS[bl_level_idx] * 100 / 255);
      Serial.println(F("%)"));
  }

  uint8_t getBrightnessLevel() {
      return bl_level_idx;
  }

  void brightnessSet(uint8_t level) {
    if (level >= BL_NUM_LEVELS)
      level = BL_NUM_LEVELS - 1;
      bl_level_idx = level;

      uint32_t currentDuty; //  = ledcRead(TLED);
      // Serial.print("currentDuty = "); Serial.println(currentDuty);

      BL_SET(BL_LEVELS[bl_level_idx]);
      // Serial.print("bl_level_idx = "); Serial.println(BL_LEVELS[bl_level_idx]);

      currentDuty = ledcRead(TFT_BL);
      // Serial.print("TFT_BL currentDuty = "); Serial.println(currentDuty);
      log_d("brightnessSet: level = %d currentDuty=%d", bl_level_idx, currentDuty);
  }

  void brightnessSave(uint8_t level) {
      if (level >= BL_NUM_LEVELS) level = BL_NUM_LEVELS - 1;
      bl_level_idx = level;
      BL_SET(BL_LEVELS[bl_level_idx]);
      bl_prefs.putUChar("level", bl_level_idx);
      // Serial.print("bl_level_idx = "); Serial.println(BL_LEVELS[bl_level_idx]);
      uint32_t currentDuty = ledcRead(TLED);
      // Serial.print("currentDuty = "); Serial.println(currentDuty);
      log_d("brightnessSet: level = %d currentDuty=%d", bl_level_idx, currentDuty);
  }

  void backlightOn() {
    // Serial.println("BL brightnessOn");
    BL_SET(BL_LEVELS[bl_level_idx]);
      // Serial.print("bl_level_idx = "); Serial.println(BL_LEVELS[bl_level_idx]);
      uint32_t currentDuty = ledcRead(TLED);
      // Serial.print("currentDuty = "); Serial.println(currentDuty);
      log_d("backlightOn: level = %d currentDuty=%d", bl_level_idx, currentDuty);
  }

  void backlightOff() {
      BL_SET(0);
      log_d("backlightOff");
  }

#else   // HAS_MINI_SCREEN

   uint8_t BL_NUM_LEVELS = 1;
  // dummyFunctions
  void brightnessInit() {
      Serial.print(F("[brightnessInit] HAS_MINI_SCREEN "));
      pinMode(TFT_BL, OUTPUT);
  }

  // dummy
  void brightnessCycle() { }
  uint8_t getBrightnessLevel() { return 9; }
  void brightnessSet(uint8_t level) { (void) level; }
  void brightnessSave(uint8_t level) { (void) level; }

  void backlightOn() {
    log_d("backlightOn");
    #ifdef TFT_BL
      // ???
      #if defined(MARAUDER_MINI) || defined(MARAUDER_MINI_V3)
        digitalWrite(TFT_BL, LOW);
      #endif

      #if !defined(MARAUDER_MINI) && !defined(MARAUDER_MINI_V3)
        digitalWrite(TFT_BL, HIGH);
      #endif
    #else
      // Nothing
    #endif
  }

  void backlightOff() {
    log_d("backlightOff");
    #if defined(MARAUDER_MINI) || defined(MARAUDER_MINI_V3)
      digitalWrite(TFT_BL, HIGH);
    #endif
  
    #if !defined(MARAUDER_MINI) && !defined(MARAUDER_MINI_V3)
      digitalWrite(TFT_BL, LOW);
    #endif
  }

#endif //  HAS_AW9364  / !HAS_MINI_SCREEN / ..

#else // HAS_SCREEN

   uint8_t BL_NUM_LEVELS = 1;
  // Dummy Functions, should never be called but are here just in case
  void brightnessInit() { log_d("-- brightnessInit Fallthrough"); }
  void brightnessCycle() { }
  uint8_t getBrightnessLevel() { return 9; }
  void brightnessSet(uint8_t level) { }
  void brightnessSave(uint8_t level) { }
  void backlightOn() { }
  void backlightOff() { }

#endif // HAS_SCREEN
