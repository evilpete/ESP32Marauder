#ifndef CYD_Sound_HPP
#define CYD_Sound_HPP

#include "configs.h"

#include <Arduino.h>
// #include "driver/ledc.h"

  // Piezo speaker driven by tone() (LEDC) on the CYD boards.
  // Raw driver: no settings checks here, Sound.hpp handles "EnableSND".

#ifndef SOUND_PIN
    #define SOUND_PIN 26
#endif

#ifndef SND_CHANNEL
  #define SND_CHANNEL 3
#endif

class CYD_Sound {
  public:
      bool supported = false;

      bool begin() {
      if ( supported ) {
          log_d("CYD_Sound already started");
          return true;
      }
        log_d("CYD_Sound::begin: SOUND_PIN=%d CHANNEL=%d", SOUND_PIN, SND_CHANNEL);
        // duty_cycle = DUTY;

        #ifndef DEVELOPER
          Serial.println("SOUND_PIN :" + (String)SOUND_PIN);
        #else
          log_d("SND_CHANNEL : %d",  SND_CHANNEL);
          log_i("SOUND_PIN : %d",  SOUND_PIN);
        #endif
        // ledcWrite(SND_CHANNEL, duty_cycle);
        setToneChannel(SND_CHANNEL);
        noTone(SOUND_PIN);
        supported = true;
        return true;
      }

      void stop() {
        noTone(SOUND_PIN);
      }

        // No volume control on a piezo
      void setVolume(uint8_t vol) { (void)vol; }
      int32_t getVolume() { return -1; }
      void mute(bool on) { log_d("mute");  if (on) noTone(SOUND_PIN); }

      void beep(uint32_t frequencyHz = 1000, uint32_t durationMs = 80) {
          log_d("Beep");
          tone(SOUND_PIN, frequencyHz, durationMs);
      }

        // UI / menu click
      void click() {   // Arg not used
          log_d("click");
          tone(SOUND_PIN, 400, 8);
      }

        // Geiger tick, low (APs)
      void tick(bool x = 1) { // Arg not used
          log_d("tick");
            Tone(SOUND_PIN,150, 20);
            Tone(SOUND_PIN,180, 10);
      }

        // Geiger tick, high (stations)
      void click2(uint16_t mod1 = 2, uint16_t mod2 = 4) {
          log_d("click2");
          if (mod1 < 50) mod2 *= 100;
          if (mod2 < 50) mod1 *= 100;
          tone(SOUND_PIN, mod1, 10);
          tone(SOUND_PIN, mod2, 10);
      }

//      void Sound_CYD::geigerClick(uint8_t i) {
//          if (i)
//              tone(SOUND_PIN, 250, 10);
//          else
//              tone(SOUND_PIN, 150, 10);
//      }

      // void s_power_on() { tone(SOUND_PIN,523,80); tone(SOUND_PIN,659,100); tone(SOUND_PIN,784,120); }
      // void s_ready() { tone(SOUND_PIN,523,100); tone(SOUND_PIN,659,100); tone(SOUND_PIN,784,100); tone(SOUND_PIN,1046,180); }
      // void s_ready_2() { tone(SOUND_PIN,784,80); delay(50); tone(SOUND_PIN,1046,100); }            // Ready
      // void s_error() { tone(SOUND_PIN,523,150); delay(50); tone(SOUND_PIN,330,180); }            // Error
      // void s_error_2() { for(int i=0;i<3;i++){ tone(SOUND_PIN,300,150); delay(50);} }
};

inline CYD_Sound CYD_Sound_obj;

#endif  /* CYD_Sound_HPP */
