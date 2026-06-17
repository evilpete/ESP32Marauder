/* FLASH SETTINGS
Board: LOLIN D32
 Frequency: 80MHz
Partition Scheme: Minimal SPIFFS
https://www.online-utility.org/image/convert/to/XBM
*/

#include "configs.h"
#include "driver/gpio.h"


#ifndef HAS_SCREEN
  #define MenuFunctions_h
  #define Display_h
#endif

#include <stdio.h>

#ifdef HAS_GPS
  #include "GpsInterface.h"
#endif

#ifdef defined(CORE_DEBUG_LEVEL) && CORE_DEBUG_LEVEL
  #include "driver/gpio.h"
  #include "soc/soc_caps.h"
#endif

#include "Assets.h"
#include "WiFiScan.h"
#ifdef HAS_SD
  #include "SDInterface.h"
#endif
#include "Buffer.h"

#ifdef I2C_FREQ
  #include "Wire.h"
#endif

#ifdef CYD_SOUND
    #include "Sound_CYD.h"
    Sound_CYD sound_obj;
#endif

#ifdef HAS_FLIPPER_LED
  #include "flipperLED.h"
  flipperLED led_obj;
#elif defined(HAS_XIAO_LED) || defined(XIAO_ESP32_S3)
  #include "xiaoLED.h"
  xiaoLED led_obj;
#elif defined(HAS_STICKC_LED) || defined(MARAUDER_M5STICKC) || defined(MARAUDER_M5STICKCP2)
  #include "stickcLED.h"
  stickcLED led_obj;
#elif defined(HAS_NEOPIXEL_LED)
  #include "LedInterface.h"
  LedInterface led_obj;
#endif

#include "settings.h"
#include "CommandLine.h"
#include "lang_var.h"

#ifdef HAS_BATTERY
  #include "BatteryInterface.h"
#endif


#ifdef HAS_SCREEN
  #include "Display.h"
  #include "MenuFunctions.h"
#endif

#ifdef HAS_BUTTONS
  #include "Switches.h"

  #if (U_BTN >= 0)
    Switches u_btn = Switches(U_BTN, 1000, U_PULL);
  #endif
  #if (D_BTN >= 0)
    Switches d_btn = Switches(D_BTN, 1000, D_PULL);
  #endif
  #if (L_BTN >= 0)
    Switches l_btn = Switches(L_BTN, 1000, L_PULL);
  #endif
  #if (R_BTN >= 0)
    Switches r_btn = Switches(R_BTN, 1000, R_PULL);
  #endif
  #if (C_BTN >= 0)
    Switches c_btn = Switches(C_BTN, 1000, C_PULL);
  #endif

#endif

#ifdef PM_ENABLE
  #include "PowerSave.h"
  PowerSave PM_obj;
#endif

#ifdef HAS_CST820
  #include <CST820.h>
  CST820 CST820_touch;
#endif

WiFiScan wifi_scan_obj;
EvilPortal evil_portal_obj;
Buffer buffer_obj;
Settings settings_obj;
CommandLine cli_obj;

#ifdef HAS_SCREEN
  extern void brightnessInit();
  extern void backlightOff();
  extern void backlightOn();
#endif

#ifdef HAS_GPS
  GpsInterface gps_obj;
#endif

#ifdef HAS_RTC
  #include "RTC.h"
  RTC rtc_obj;
#endif

#ifdef HAS_BATTERY
  BatteryInterface battery_obj;
#endif

#ifdef HAS_SCREEN
  Display display_obj;
  MenuFunctions menu_function_obj;
#endif

#if defined(HAS_SD) && !defined(HAS_C5_SD)
  SDInterface sd_obj;
#endif


const String PROGMEM version_number = MARAUDER_VERSION;


// #ifdef HAS_NEOPIXEL_LED
//   Adafruit_NeoPixel strip = Adafruit_NeoPixel(Pixels, PIN, NEO_GRB + NEO_KHZ800);
// #endif

uint32_t currentTime  = 0;


  void DeepSleep(int8_t wakeup_but) {

    wakeup_but = -1;
    // 1. Disconnect from the network gracefully
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    // 2. Explicitly stop the WiFi driver to save power
    esp_wifi_stop();

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
    esp_sleep_config_gpio_isolate();
    
    if (wakeup_but >= 0) {
      gpio_hold_dis((gpio_num_t) wakeup_but);
      pinMode(wakeup_but, INPUT_PULLUP);

    // Configure the wake-up source: wake up when GPIO 0 goes LOW (button press)
    #if SOC_PM_SUPPORT_EXT_WAKEUP
	// For classic ESP32 which supports EXT0 (e.g., ESP32)
        esp_sleep_enable_ext0_wakeup((gpio_num_t)wakeup_but, 0); // 0 means LOW
    #elif SOC_PM_SUPPORT_GPIO_WAKEUP
       // For newer chips that use generic GPIO wakeup (e.g., ESP32-C3, ESP32-S3)
      esp_deep_sleep_enable_gpio_wakeup((1ULL << wakeup_but), ESP_GPIO_WAKEUP_GPIO_LOW);
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

  void shutdown() {
    #ifdef POWER_HOLD_PIN
        // T-HMI
        //  if on battery, can be turn off with the PWR_ON_PIN/POWER_HOLD_PIN if on battery
        Serial.println("Set POWER_HOLD_PIN:  LOW");
        Serial.flush();
        digitalWrite(POWER_HOLD_PIN, LOW);
        //  if plugged in we use DEEPSLEEP instead
        delay(500);
        Serial.println("DeepSleep");
        DeepSleep(0);
    #else
        DeepSleep(0);
    #endif
  }


#ifdef HAS_C5_SD
  SPIClass sharedSPI(SPI);
  SDInterface sd_obj = SDInterface(&sharedSPI, SD_CS);
#endif

void setup()
{

  #ifdef POWER_HOLD_PIN  
    pinMode(POWER_HOLD_PIN, OUTPUT);
    digitalWrite(POWER_HOLD_PIN, HIGH);
  #endif

  #ifdef PWR_EN_PIN  // Enable power to peripherals
    pinMode(PWR_EN_PIN, OUTPUT);
    digitalWrite(PWR_EN_PIN, HIGH);
  #endif

  randomSeed(esp_random());

  #ifndef DEVELOPER
    esp_log_level_set("*", ESP_LOG_NONE);
  #endif

  #ifdef ARDUINO_USB_MODE
    Serial.println("ARDUINO_USB_MODE = " + (String)ARDUINO_USB_MODE);
  #endif
  #ifdef ARDUINO_USB_CDC_ON_BOOT
    Serial.println("ARDUINO_USB_CDC_ON_BOOT = " + (String)ARDUINO_USB_CDC_ON_BOOT);
  #endif
  
  #ifndef HAS_IDF_3
    esp_spiram_init();
  #endif

  Serial.begin(115200);  // 115200);

  #ifdef HAS_ACT_LED
    pinMode(ACT_LED_PIN, OUTPUT);
    delay(100);
    digitalWrite(ACT_LED_PIN, LOW);
  #endif

// -D ARDUINO_USB_MODE=1
//   -D ARDUINO_USB_CDC_ON_BOOT
  // Serial.setTxTimeoutMs(40);
  #if defined(ARDUINO_USB_CDC_ON_BOOT) && ARDUINO_USB_CDC_ON_BOOT == 1
  // Serial.setTxTimeoutMs(40);
    while(!Serial && millis() < 5000)
       delay(10);
  #else
    while(!Serial)
       delay(10);
  #endif

  #ifdef HAS_C5_SD
    sharedSPI.begin(SD_SCK, SD_MISO, SD_MOSI);
    delay(100);
  #endif


  #if defined(TFT_BL)
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH); // ???
  #endif

  #ifdef HAS_SCREEN
    backlightOff();
  #endif

  #if BATTERY_ANALOG_ON == 1
    pinMode(BATTERY_PIN, OUTPUT);
    pinMode(CHARGING_PIN, INPUT);
  #endif

  // Preset SPI CS pins to avoid bus conflicts
  #if defined(HAS_SCREEN) && defined(TFT_CS)
    digitalWrite(TFT_CS, HIGH);
  #endif
  
  #if defined(HAS_SD) && defined(SD_CS) && !defined(HAS_C5_SD)
    pinMode(SD_CS, OUTPUT);
    delay(10);

    digitalWrite(SD_CS, HIGH);
    delay(10);
  #endif

  //Serial.begin(115200);

  //while(!Serial)
  //  delay(10);

  #ifdef CYD_SOUND
      sound_obj.RunSetup();
  #endif

  Serial.println("ESP-IDF version is: " + String(esp_get_idf_version()));
  #ifdef ESP_ARDUINO_VERSION_STR
    Serial.print("Arduino ESP32 Core Version: ");
    Serial.println(ESP_ARDUINO_VERSION_STR);
  #elif defined(ESP_ARDUINO_VERSION)
    Serial.printf("Arduino Core Major: %d, Minor: %d, Patch: %d\n", 
	    ESP_ARDUINO_VERSION_MAJOR, ESP_ARDUINO_VERSION_MINOR, ESP_ARDUINO_VERSION_PATCH);
  #endif




  #ifdef HAS_PSRAM
    if (!psramInit()) {
      Serial.println(F("PSRAM not available"));
    }
  #endif

  #ifdef HAS_SIMPLEX_DISPLAY
    #if defined(HAS_SD)
      // Do some SD stuff
      if(!sd_obj.initSD())
        Serial.println(F("SD Card NOT Supported"));

    #endif
  #endif

  #if defined(HAS_CST820)
      // github.com/evilpete/CST820
      Serial.println(F("CST820_touch.begin()")); Serial.flush();
      CST820_touch.begin(CST820_SDA, CST820_SCL, CST820_RST, CST820_INT);
      // delay(500);
  #endif

  #ifdef HAS_SCREEN
    display_obj.RunSetup();
    display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
  #endif

  #if defined(HAS_SCREEN) && !defined(HAS_MINI_SCREEN)
    brightnessInit();
    backlightOff();
  #endif

  #ifdef HAS_SCREEN
    #if !defined(MARAUDER_CARDPUTER) && !defined(MARAUDER_CARDPUTER_ADV)
      display_obj.tft.drawCentreString("ESP32 Marauder", TFT_WIDTH/2, TFT_HEIGHT * 0.33, 1);
      display_obj.tft.drawCentreString("JustCallMeKoko", TFT_WIDTH/2, TFT_HEIGHT * 0.5, 1);
      display_obj.tft.drawCentreString(display_obj.version_number, TFT_WIDTH/2, TFT_HEIGHT * 0.66, 1);
    #else
      display_obj.tft.drawCentreString("ESP32 Marauder", TFT_HEIGHT/2, TFT_WIDTH * 0.33, 1);
      display_obj.tft.drawCentreString("JustCallMeKoko", TFT_HEIGHT/2, TFT_WIDTH * 0.5, 1);
      display_obj.tft.drawCentreString(display_obj.version_number, TFT_HEIGHT/2, TFT_WIDTH * 0.66, 1);
    #endif
  #endif


  #ifdef HAS_SCREEN
    backlightOn(); // Need this
  #endif

  #ifdef HAS_SCREEN
    // Do some stealth mode stuff
    #ifdef HAS_BUTTONS
      if (c_btn.justPressed()) {
        display_obj.headless_mode = true;

        backlightOff();
      }
    #endif
  #endif

  settings_obj.begin();

  const char* type = settings_obj.getSettingType("ChanHop");

  if (type == nullptr || type[0] == '\0') {
    Serial.println(F("Current settings format not supported. Installing new default settings..."));
    settings_obj.createDefaultSettings(SPIFFS);
  }

  buffer_obj = Buffer();

  #ifndef HAS_SIMPLEX_DISPLAY
    #if defined(HAS_SD)
      // Do some SD stuff
      if(!sd_obj.initSD())
        Serial.println(F("SD Card NOT Supported"));

    #endif
  #endif

  Serial.println("wifi_scan_obj.RunSetup");
  wifi_scan_obj.RunSetup();

  #ifdef HAS_SCREEN
    display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
    display_obj.tft.drawCentreString("Initializing...", TFT_WIDTH/2, TFT_HEIGHT * 0.82, 1);
  #endif

  #ifdef HAS_RTC
    rtc_obj.RunSetup();
  #endif

  evil_portal_obj.setup();

  #if defined(HAS_CST820)
      // github.com/evilpete/CST820
      Serial.println(F("CST820_touch.begin()")); Serial.flush();
      CST820_touch.begin(CST820_SDA, CST820_SCL, CST820_RST, CST820_INT);
      // delay(500);
  #endif

  #ifdef HAS_BATTERY
    battery_obj.RunSetup();
  #endif

  #ifdef HAS_BATTERY
    battery_obj.battery_level = battery_obj.getBatteryLevel();
  #endif

  // Do some LED stuff
  led_obj.RunSetup();

  #ifdef HAS_GPS
    gps_obj.begin();
  #endif

  #ifdef HAS_SCREEN
    display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
  #endif

  #ifdef HAS_SCREEN
    #if defined(MARAUDER_CARDPUTER) || defined(MARAUDER_CARDPUTER_ADV)
      display_obj.clearScreen();
    #endif
    menu_function_obj.RunSetup();
  #endif

  /*char ssidBuf[64] = {0};  // or prefill with existing SSID
  if (keyboardInput(ssidBuf, sizeof(ssidBuf), "Enter SSID")) {
    // user pressed OK
    Serial.println(ssidBuf);
  } else {
    Serial.println(F("User exited keyboard"));
  }

  menu_function_obj.changeMenu(menu_function_obj.current_menu);*/

  wifi_scan_obj.StartScan(WIFI_SCAN_OFF);

  cli_obj.RunSetup();

  #ifdef PM_ENABLE
    PM_obj.pm_config();
  PM_obj.set_wake_intr();
  #endif

// #ifdef defined(CORE_DEBUG_LEVEL) && CORE_DEBUG_LEVEL
//     gpio_dump_io_configuration(stdout, SOC_GPIO_VALID_GPIO_MASK);
// #endif

#ifdef I2C_FREQ
  Wire.setClock(I2C_FREQ);           // reset I2C_FREQ incase it was chamged
#endif


}



void loop()
{
  currentTime = millis();
  bool mini = false;

  #ifdef SCREEN_BUFFER
    #ifndef HAS_ILI9341
      mini = true;
    #endif
  #endif

  #if (defined(HAS_ILI9341) && !defined(MARAUDER_CYD_2USB))
    #ifdef HAS_BUTTONS
      if (c_btn.isHeld()) {
        if (menu_function_obj.disable_touch)
          menu_function_obj.disable_touch = false;
        else
          menu_function_obj.disable_touch = true;

        menu_function_obj.updateStatusBar();

        while (!c_btn.justReleased())
          delay(1);
      }
    #endif
  #endif

  // Update all of our objects
  cli_obj.main(currentTime);
  wifi_scan_obj.main(currentTime);

  #ifdef HAS_GPS
    gps_obj.main();
  #endif

  // Save buffer to SD and/or serial
  buffer_obj.save();

  #ifdef HAS_BATTERY
    battery_obj.main(currentTime);
  #endif
  if ((wifi_scan_obj.currentScanMode != WIFI_PACKET_MONITOR) ||
      (mini)) {
    #ifdef HAS_SCREEN
      menu_function_obj.main(currentTime);
    #endif
  }

  /*
  #ifdef HAS_LED
    led_obj.main(currentTime);
  #endif
  */

  #ifdef HAS_SCREEN
    delay(1);
  #else
    delay(50);
  #endif
}
