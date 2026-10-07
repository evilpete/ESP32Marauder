#include "configs.h"

#ifndef Sound_HPP
#define Sound_HPP


#include <Arduino.h>
#include <Wire.h>

/*
  One sound API for every board. Pick the hardware in configs.h:

    HAS_CYD_SOUND  : piezo on SOUND_PIN driven by tone()      (CYD_Sound.hpp)
    HAS_ES8311     : ES8311 I2S codec, amp via CH32V003       (ES8311.hpp)
    neither        : NullSound, every call is an empty inline (compiles away)

  configs.h defines HAS_SOUND when either backend is present.

    sound_obj.RunSetup();          // once, after Wire / CH32V003 are up
    sound_obj.click();             // UI / menu click
    sound_obj.geigerClick(i);      // scan hit: 0 = AP (low), 1 = station (high)
    sound_obj.tick();              // Short click / low geiger tick
    sound_obj.click2();            // Longer Click /  geiger tick
    sound_obj.beep(freq, ms);      // tone
    sound_obj.stop();
    sound_obj.setVolume(v);        // ES8311 only, no-op on CYD
    sound_obj.supported();

  All play calls honour the "EnableSND" setting.
*/

#ifdef HAS_SOUND
  #include "settings.h"
  extern Settings settings_obj;
#endif

  // No sound hardware: same calls as the real drivers, all no-ops
class NullSound {
  public:
    bool supported = false;

    bool begin() { return false; }
    void stop() {}
    void setVolume(uint8_t) {}
    int32_t getVolume() { return -1; }
    void mute(bool) {}
    void beep(uint32_t = 0, uint32_t = 0) {}
    void click() {}
    void tick() {}
    void titk() {}
    void click2(uint16_t mod1 = 2, uint16_t mod2 = 4) {}
};

  // Pick the backend once; Sound forwards to sound_dev
#if defined(HAS_ES8311)
  #include "ES8311.hpp"
  #ifdef HAS_CH32V003
    #include "CH32V003_IOExpander.hpp"   // drives the speaker amp enable
  #endif
  using SoundDev = ES8311;
  inline SoundDev &sound_dev = ES8311_obj;
#elif defined(HAS_CYD_SOUND)
  #include "CYD_Sound.hpp"
  using SoundDev = CYD_Sound;
  inline SoundDev &sound_dev = CYD_Sound_obj;
#else
  using SoundDev = NullSound;
  inline SoundDev sound_dev;
#endif

class Sound {

  public:
     Sound() = default;
     #ifdef HAS_CH32V003
       ~Sound() { CH32V003_obj.SetAudio(0); }    // Turn off output amp
     #endif

    bool RunSetup(TwoWire *mywire = nullptr) {

      if ( sound_dev.supported ) {
          log_d("Sound already started");
          return true;
      }
      #if defined(HAS_ES8311)
        bool ok = mywire ? sound_dev.begin(mywire) : sound_dev.begin();
        if (!ok) {
          log_e("ES8311 init failed");
          return false;
        }
        #ifdef HAS_CH32V003
          CH32V003_obj.SetAudio(1);    // Turn on output amp
        #endif
        ES8311_obj.setVolume(200);
        return sound_dev.supported;
      #else
        (void)mywire;
        return sound_dev.begin();
      #endif
    }

    bool supported() { return sound_dev.supported; }

    void click()  { if (enabled()) sound_dev.click(); log_d("click"); }
    void tick()   { if (enabled()) sound_dev.tick(); log_d("tick");}
    void tit()   { if (enabled()) sound_dev.tit(); log_d("tit");}
    // void click2() { if (enabled()) sound_dev.click2(); log_d("click2");}
    // std::function<void()> click2 = sound_dev.click2;
    // void (&click2)() = sound_dev.click2;
    void click2(uint16_t mod1, uint16_t mod2) {
      sound_dev.click2(mod1, mod2);
    }


    void geigerClick(uint8_t i = 0) {
      if (i) click();
      else   tick();
      log_d("geigerClick");
    }

    void beep(uint32_t frequencyHz = 1000, uint32_t durationMs = 80) {
      if (enabled()) sound_dev.beep(frequencyHz, durationMs);
    }

    void stop()                 { sound_dev.stop(); }
    void mute(bool on)          { sound_dev.mute(on); }
    void setVolume(uint8_t vol) { sound_dev.setVolume(vol); }
    int32_t getVolume()         { return sound_dev.getVolume(); }

  private:
    bool enabled() {
      #ifdef HAS_SOUND
        return settings_obj.loadSetting<bool>("EnableSND");
      #else
        return false;
      #endif
    }
};

inline Sound sound_obj;

#endif  // Sound_HPP
