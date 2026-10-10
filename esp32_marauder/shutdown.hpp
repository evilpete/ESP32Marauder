
#pragma once

#ifndef SHUTDOWN_HPP
#define SHUTDOWN_HPP

#ifdef HAS_SCREEN
  #include "Display.h"
  extern Display display_obj;
#endif
#if defined(DEEPSLEEP) || defined(POWER_HOLD_PIN)

#ifdef HAS_ZIGBEE
    #include "esp_zigbee_core.h"
#endif

  #include "driver/gpio.h"
  #include "esp_sleep.h"
  #if SOC_RTCIO_PIN_COUNT > 0
    #include "driver/rtc_io.h"
  #endif

  // Pin used to wake from DeepSleep(), -1 = no wake (acts as power off).
  // Classic ESP32/S3 (CYD etc.) can wake on GPIO0 (BOOT). On C3/C5/C6 the
  // BOOT button is not an LP GPIO and GPIO0 is often I2C, so default to none.
  #ifndef DEEPSLEEP_WAKE_PIN
    #if SOC_PM_SUPPORT_EXT0_WAKEUP || SOC_PM_SUPPORT_EXT_WAKEUP
      #define DEEPSLEEP_WAKE_PIN 0
    #else
      #define DEEPSLEEP_WAKE_PIN -1
    #endif
  #endif

  // should this be in a separate .cpp file
  inline void DeepSleep(int8_t wakeup_but = -1) {
    esp_err_t x = 0;

    #ifdef HAS_SCREEN
      display_obj.tft.fillScreen(TFT_BLACK);
      display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
      display_obj.tft.drawCentreString("DeepSleep", TFT_WIDTH / 2, TFT_HEIGHT / 2, 5);
      delay(1200);
    #endif

    // 1. Disconnect from the network gracefully
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    // esp_wifi_stop();
    // ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_stop());
    delay(200);
    Serial.flush();


    #ifdef HAS_BT
      // This handles stopping and deinitializing BT gracefully
      // esp_bluedroid_disable();
      ESP_ERROR_CHECK_WITHOUT_ABORT(esp_bt_controller_disable());
      ESP_ERROR_CHECK_WITHOUT_ABORT(esp_bt_controller_deinit());
      delay(200);
    #endif

    #ifdef HAS_ZIGBEE
      // wifi_scan_obj>StopZigbeeScan();
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_ieee802154_sleep());
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_ieee802154_disable());
        // esp_zigbee_deinit();
    #endif
        delay(200);

    #if defined(MARAUDER_WS_C5_28)
      CH32V003_obj.SetAudio(0);
      CH32V003_obj.setPWM(0);
    #endif
      delay(200);

    #if defined(HAS_SCREEN)  && defined(ST7789_DRIVER)

      // 1. Put the display driver into sleep mode
      display_obj.tft.writecommand(0x10); // Sleep-in command (SLPIN)
      delay(5);               // Wait for shutdown time

      // 2. Turn off the LCD display output (if using ST7789 or similar)
      display_obj.tft.writecommand(0x38); // Or use ST7789_DISPOFF

      // 3. Turn off the backlight (replace 38 with your actual backlight GPIO pin)

      #ifdef TFT_BL
        digitalWrite(TFT_BL, LOW);  
      #endif

    #endif
      

    // Should we isolate  pins with external pull-up resistors
    // to minimize current consumption.
    // #ifdef I2C_SDA
    //   rtc_gpio_isolate(I2C_SDA);
    //   rtc_gpio_isolate(I2C_SCL);
    // #endif

    // Code specific to the classic ESP32 (e.g., WROOM-32) goes here
    // #ifdef CONFIG_IDF_TARGET_ESP32
    // rtc_gpio_isolate(GPIO_NUM_12);
    // 18 19 5 23 10 33 32 16 17 20 
    esp_sleep_config_gpio_isolate(); // Void Value
    
    // Start clean: no timer/touch/ULP/GPIO wake sources left over from elsewhere.
    // With no wake source enabled the chip stays "off" until reset or power cycle.
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL));

    if (wakeup_but >= 0) {
    #if SOC_PM_SUPPORT_EXT0_WAKEUP || SOC_PM_SUPPORT_EXT_WAKEUP
      // Classic ESP32 / S2 / S3: EXT0 works on any RTC-capable pin (e.g. GPIO0 BOOT)
      if (rtc_gpio_is_valid_gpio((gpio_num_t)wakeup_but)) {
        gpio_hold_dis((gpio_num_t)wakeup_but);
        pinMode(wakeup_but, INPUT_PULLUP);
        while (digitalRead(wakeup_but) == LOW) delay(10);   // don't wake on the press that got us here
        delay(50);
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_sleep_enable_ext0_wakeup((gpio_num_t)wakeup_but, 0)); // 0 means LOW
      } else {
        Serial.printf("GPIO%d cannot wake from deep sleep, no wake source\n", wakeup_but);
      }
    #elif SOC_GPIO_SUPPORT_DEEPSLEEP_WAKEUP
      // C3/C5/C6...: only LP GPIOs can wake (C3: 0-5, C5: 0-6, C6: 0-7).
      // e.g. the C5 BOOT button (GPIO28) cannot, and the call would fail.
      if (SOC_GPIO_DEEP_SLEEP_WAKE_VALID_GPIO_MASK & (1ULL << wakeup_but)) {
        gpio_hold_dis((gpio_num_t)wakeup_but);
        pinMode(wakeup_but, INPUT_PULLUP);
        while (digitalRead(wakeup_but) == LOW) delay(10);
        delay(50);
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_deep_sleep_enable_gpio_wakeup((1ULL << wakeup_but), ESP_GPIO_WAKEUP_GPIO_LOW));
      } else {
        Serial.printf("GPIO%d cannot wake from deep sleep, no wake source\n", wakeup_but);
      }
    #else
      #warning "Unsupported sleep/wakeup architecture on this chip"
    #endif
    }

    Serial.println("Going to sleep now...");
    Serial.flush();
    delay(100); // Give serial monitor time to flush

    // Enter deep sleep
    esp_deep_sleep_start();
  }

  inline void shutdown_system() {   // The name shutdown already exists globally. lwIP's sockets.h

    #ifdef POWER_HOLD_PIN
        // T-HMI
        //  if on battery, can be turn off with the PWR_ON_PIN/POWER_HOLD_PIN if on battery
        Serial.println("Set POWER_HOLD_PIN:  LOW");
        Serial.flush();
        digitalWrite(POWER_HOLD_PIN, LOW);

        //  if plugged in we use DEEPSLEEP instead
        delay(500);
        Serial.println("DeepSleep");
        DeepSleep();
    #else
        DeepSleep(DEEPSLEEP_WAKE_PIN);
    #endif
  }

#endif  // DEEPSLEEP / POWER_HOLD_PIN

#endif  // SHUTDOWN_HPP


#ifdef NEVER_EVER
/*

CST3530 capacitive touch:
    • Hardware Deep Sleep: If your host MCU is going into a deep
    sleep, hold the CST3530 RESET pin LOW to lock the chip into its
    lowest hardware reset state.

CST820 (CST816 Family)
    write to the control registers is required to change this state.
    Writing 0xFF to register 0xFE is commonly used to disable the
    aggressive auto-low-power behavior during active MCU cycles.

FocalTech FT6336
    FT6336 into its lowest sub-microamp Hibernation state, write 0x03
    to the Device Mode Register (0x00).

XPT2046 (Resistive, SPI)
    • Every SPI command byte sent to the XPT2046 contains two
    dedicated Power-Down bits (PD1 and PD0, which are bits 1 and 0
    of the control byte). To force the chip into an absolute
    ultra-low-power sleep state at the end of a transaction, you
    must intentionally set these bits to 00 on the very last byte
    of your SPI communication:

    PD1 Bit Value   PD0 Bit Value   Power State Result

         0           0      Power-Down Mode (Sleep). Alternating conversion drivers are
                            disabled, and the PENIRQ (Pen Interrupt) output is safely
                            activated.


ST7789
      TFT Code to Wake Up and Power On

      // 1. Turn the backlight back on
      digitalWrite(38, HIGH); 

      // 2. Wake the display driver up
      tft.writecommand(0x11); // Sleep-out command (SLPOUT)
      delay(120);             // Wait for power supplies to stabilize

      // 3. Reinitialize or turn display on if needed
      tft.writecommand(0x29); // Display-on command (DISPON) - or
                              reinit if required by your board


ES8311
  • Software Power-Down Control: Power management is split across
    multiple registers. To place the ES8311 into a deep power-down state
    via I2C (Address: 0x18), you must disable the internal digital and
    analog subsystems sequentially to avoid speaker pops:

      1. Register 0x19 (ADC/DAC Control): Power down the digital filters.
      2. Register 0x1C (Power Management): Power down the analog biases.
      3. Register 0x00 (System Control): Set the chip into standby.

  • Low-Power Register Toggles: If you want to conserve power during
    active audio streaming without turning the device completely off,
    you can use the Low Power Control Register (0x0F):

      • Set LPDAC (Bit 7) = 1 to activate low-power mode for the DAC.
      • Set LPPGA (Bit 6) = 1 to activate low-power mode for the PGA input.

  • Hardware Shutdown Limitation: Unlike other complex codecs, the
    ES8311 does not feature a dedicated hardware power-down pin. It
    depends on an external GPIO-controlled Audio PA Enable/Mute switch
    (routing to your amplifier) to handle physical audio isolation,
    while the chip itself is managed strictly over the I2C configuration
    registers.

*/
#endif

