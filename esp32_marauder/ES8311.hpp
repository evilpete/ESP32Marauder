#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <driver/i2s_std.h>

  // --- I2C (codec control) ---

#ifndef ES8311_SDA
  #ifdef I2C_SDA
    #define ES8311_SDA I2C_SDA
    #define ES8311_SCL I2C_SCL
  #else
    #define ES8311_SDA 0
    #define ES8311_SCL 1
  #endif
#endif

#ifndef ES8311_FREQ
  #ifdef I2C_FREQ
    #define ES8311_FREQ I2C_FREQ
  #else
    #define ES8311_FREQ 100000
  #endif
#endif

#ifndef ES8311_ADDR
  #define ES8311_ADDR 0x18    // CE pin low; 0x19 when CE is high
#endif

  // --- I2S (audio data) ---
  // Set these per board in configs.h. Defaults are the Waveshare
  // ESP32-C5-Touch-LCD-2.8 BSP wiring. MCLK = -1 means no MCLK line:
  // the codec then derives its master clock from BCLK.

#ifndef ES8311_I2S_MCLK
  #define ES8311_I2S_MCLK  -1
#endif
#ifndef ES8311_I2S_BCLK
  #define ES8311_I2S_BCLK  24
#endif
#ifndef ES8311_I2S_WS
  #define ES8311_I2S_WS    25
#endif
#ifndef ES8311_I2S_DOUT
  #define ES8311_I2S_DOUT  26
#endif

#if defined(TFT_CS) && (TFT_CS >= 0) && \
    (ES8311_I2S_MCLK == TFT_CS || ES8311_I2S_BCLK == TFT_CS || \
     ES8311_I2S_WS == TFT_CS || ES8311_I2S_DOUT == TFT_CS)
  #error "ES8311 I2S pin collides with TFT_CS - define ES8311_I2S_* for this board"
#endif

#if defined(CONFIG_IDF_TARGET_ESP32C5) && \
    (ES8311_I2S_MCLK == 13 || ES8311_I2S_BCLK == 13 || ES8311_I2S_WS == 13 || ES8311_I2S_DOUT == 13 || \
     ES8311_I2S_MCLK == 14 || ES8311_I2S_BCLK == 14 || ES8311_I2S_WS == 14 || ES8311_I2S_DOUT == 14)
  #warning "ES8311 I2S pin on GPIO13/14 (ESP32-C5 USB D-/D+) will kill the USB-Serial/JTAG console"
#endif

  // --- AUDIO CONFIG ---
#define I2S_SAMPLE_RATE    16000   // 16kHz is ideal and efficient for beeps and clicks
#define I2S_MCLK_MULT      256     // MCLK = 256 * fs = 4.096 MHz @ 16 kHz
  // Without MCLK, BCLK = 2 slots * 32 bits * fs = 64 * fs and the codec
  // multiplies it x4 internally to reach 256 * fs.

#define BEEP_FREQ          1000
#define BEEP_DURATION_MS   80

class ES8311 {
 public:
    bool supported = false;

    ES8311() = default;

    ~ES8311() { deinitI2S(); }

      // Explicitly delete copy primitives to protect hardware allocations
    ES8311(const ES8311&) = delete;
    ES8311& operator=(const ES8311&) = delete;

      // Initializes both I2C (ES8311) and I2S hardware peripherals
    bool begin(int sda = ES8311_SDA, int scl = ES8311_SCL,
               int bclk = ES8311_I2S_BCLK, int ws = ES8311_I2S_WS,
               int dout = ES8311_I2S_DOUT, int mclk = ES8311_I2S_MCLK,
               uint32_t sample_rate = I2S_SAMPLE_RATE) {
        if (supported) return true;

        #ifdef I2C_SDA
          if (sda != I2C_SDA)
            _wire = &Wire1;
          else
        #endif
          _wire = &Wire;

          // Wire.begin() returns true if the bus is already up with these pins
        if (!_wire->begin(sda, scl, ES8311_FREQ)) {
            log_e("Wire.begin failed");
            return false;
        }
        delay(10);
        return begin(_wire, bclk, ws, dout, mclk, sample_rate);
    }

    bool begin(TwoWire *mywire,
               int bclk = ES8311_I2S_BCLK, int ws = ES8311_I2S_WS,
               int dout = ES8311_I2S_DOUT, int mclk = ES8311_I2S_MCLK,
               uint32_t sample_rate = I2S_SAMPLE_RATE) {

        if (supported) return true;

        _wire = mywire;
        _sampleRate = sample_rate;
        _useMclk = (mclk >= 0);

        if (!probe()) {
            log_e("ES8311 not found at 0x%02X", ES8311_ADDR);
            return false;
        }
          // Start MCLK/BCLK/LRCK first; the codec is a clock slave and its
          // state machine wants the clocks present when it powers up.
        if (!initI2S(bclk, ws, dout, mclk)) {
            log_e("initI2S failed");
            return false;
        }
        if (!initCodec()) {
            log_e("initCodec failed");
            deinitI2S();
            return false;
        }

        supported = true;
        return true;
    }

      // 0 .. 255, 0xBF = 0 dB, 0xFF = +32 dB
    void setVolume(uint8_t vol) {
        if (supported) writeRegister(0x32, vol);
    }

    int getVolume() {
        if (!supported) return -1;
        return readRegister(0x32);
    }

    void mute(bool on) {
        if (!supported) return;
        writeRegister(0x31, on ? 0x60 : 0x00);
    }

      // Playback APIs
    void playBeep(float frequencyHz = BEEP_FREQ, uint32_t durationMs = BEEP_DURATION_MS, int16_t amplitude = 10000) {
        if (!supported) return;

        size_t num_samples = (_sampleRate * durationMs) / 1000;

        int16_t *buf = reinterpret_cast<int16_t *>(malloc(num_samples * sizeof(int16_t)));
        if (!buf) {
          log_e("malloc fail");
          return;
        }

        float amplitude_f = static_cast<float>(amplitude);
        for (size_t i = 0; i < num_samples; i++) {
            float t = static_cast<float>(i) / _sampleRate;
            buf[i] = static_cast<int16_t>(amplitude_f * sinf(2.0f * PI * frequencyHz * t));
        }

        size_t bytes_written;
        i2s_channel_write(_txHandle, buf, num_samples * sizeof(int16_t), &bytes_written, portMAX_DELAY);
        free(buf);
    }

    void playClick4() {
        if (!supported) return;
        const size_t num_samples = 150;
        int16_t buf[num_samples] = {0};
          // Quick hardware click impulse
        for (size_t i = 0; i < 20; i++) {
            buf[i] = (i % 4 == 0) ? 22000 : -22000;
        }
        size_t bytes_written;
        i2s_channel_write(_txHandle, buf, sizeof(buf), &bytes_written, portMAX_DELAY);
    }

    void playClick3() {
        if (!supported) return;
        const size_t num_samples = 150;
        int16_t buf[num_samples] = {0};
          // Quick hardware click impulse
        for (size_t i = 0; i < 20; i++) {
            buf[i] = (i % 3 == 0) ? 22000 : -22000;
        }
        size_t bytes_written;
        i2s_channel_write(_txHandle, buf, sizeof(buf), &bytes_written, portMAX_DELAY);
    }

    void playClick() {
        if (!supported) return;
        const size_t num_samples = 150;
        int16_t buf[num_samples] = {0};
          // Quick hardware click impulse
        for (size_t i = 0; i < 20; i++) {
            buf[i] = (i % 2 == 0) ? 22000 : -22000;
        }
        size_t bytes_written;
        i2s_channel_write(_txHandle, buf, sizeof(buf), &bytes_written, portMAX_DELAY);
    }

 private:
    TwoWire *_wire = nullptr;

    uint32_t _sampleRate = I2S_SAMPLE_RATE;
    i2s_chan_handle_t _txHandle = nullptr;
    bool _useMclk = true;

    bool writeRegister(uint8_t reg, uint8_t val) {
        _wire->beginTransmission(ES8311_ADDR);
        _wire->write(reg);
        _wire->write(val);
        return _wire->endTransmission() == 0;
    }

    int readRegister(uint8_t reg) {
        _wire->beginTransmission(ES8311_ADDR);
        _wire->write(reg);
        if (_wire->endTransmission(false) != 0) return -1;
        if (_wire->requestFrom((uint8_t)ES8311_ADDR, (uint8_t)1) != 1) return -1;
        return _wire->read();
    }

      // Chip ID registers: 0xFD = 0x83, 0xFE = 0x11
    bool probe() {
        int id1 = readRegister(0xFD);
        int id2 = readRegister(0xFE);
        log_d("ES8311 id %02X %02X", id1, id2);
        return id1 == 0x83 && id2 == 0x11;
    }

      // Slave mode, MCLK from MCLK pin, 16-bit I2S, DAC only.
      // Sequence follows Espressif's es8311 driver (esp-adf / esp_codec_dev).
    bool initCodec() {
        bool ok = true;

        ok &= writeRegister(0x00, 0x1F);   // Reset all blocks
        delay(20);
        ok &= writeRegister(0x00, 0x00);
        ok &= writeRegister(0x00, 0x80);   // CSM on, slave mode (bit6 = 0)

          // Clock manager, all clocks on. Source is the MCLK pin, or BCLK
          // (bit7) when there is no MCLK line. Internal clock = 256 * fs.
        if (_useMclk) {
            ok &= writeRegister(0x01, 0x3F);
            ok &= writeRegister(0x02, 0x00);   // pre_div 1, pre_mult x1 (MCLK = 256 fs)
        } else {
            ok &= writeRegister(0x01, 0xBF);
            ok &= writeRegister(0x02, 0x10);   // pre_div 1, pre_mult x4 (BCLK = 64 fs)
        }
        ok &= writeRegister(0x03, 0x10);   // single speed, ADC OSR
        ok &= writeRegister(0x04, 0x10);   // DAC OSR
        ok &= writeRegister(0x05, 0x00);   // ADC/DAC clk div 1
        ok &= writeRegister(0x06, 0x03);   // BCLK div 4 (slave: ignored)
        ok &= writeRegister(0x07, 0x00);   // LRCK div = 256
        ok &= writeRegister(0x08, 0xFF);

        ok &= writeRegister(0x09, 0x0C);   // SDP in (DAC): I2S, 16-bit, unmuted
        ok &= writeRegister(0x0A, 0x0C);   // SDP out (ADC): I2S, 16-bit

        ok &= writeRegister(0x0B, 0x00);   // System
        ok &= writeRegister(0x0C, 0x00);
        ok &= writeRegister(0x10, 0x1F);
        ok &= writeRegister(0x11, 0x7F);
        ok &= writeRegister(0x13, 0x10);

          // Power up analog + DAC path
        ok &= writeRegister(0x0D, 0x01);   // Power up analog circuits
        ok &= writeRegister(0x0E, 0x02);   // Enable analog PGA / ADC modulator
        ok &= writeRegister(0x12, 0x00);   // Power up DAC
        ok &= writeRegister(0x14, 0x1A);   // Enable output to HP drive (also mic PGA)
        ok &= writeRegister(0x15, 0x40);
        ok &= writeRegister(0x37, 0x08);   // Bypass DAC equalizer
        ok &= writeRegister(0x45, 0x00);
        ok &= writeRegister(0x31, 0x00);   // DAC unmute
        ok &= writeRegister(0x32, 0xBF);   // DAC volume 0 dB

        return ok;
    }

    bool initI2S(int bclk, int ws, int dout, int mclk) {
        log_d("bclk=%d ws=%d dout=%d mclk=%d", bclk, ws, dout, mclk);

        i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
        chan_cfg.auto_clear = true;   // output silence when the DMA queue runs dry

        esp_err_t err = i2s_new_channel(&chan_cfg, &_txHandle, NULL);
        if (err != ESP_OK) {
            log_e("i2s_new_channel: %s", esp_err_to_name(err));
            _txHandle = nullptr;
            return false;
        }

        i2s_std_config_t std_cfg = {
            .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(_sampleRate),
            .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
            .gpio_cfg = {
                .mclk = _useMclk ? (gpio_num_t)mclk : I2S_GPIO_UNUSED,
                .bclk = (gpio_num_t)bclk,
                .ws   = (gpio_num_t)ws,
                .dout = (gpio_num_t)dout,
                .din  = I2S_GPIO_UNUSED,
                .invert_flags = { .mclk_inv = false, .bclk_inv = false, .ws_inv = false }
            }
        };
        std_cfg.clk_cfg.mclk_multiple = (i2s_mclk_multiple_t)I2S_MCLK_MULT;
          // No MCLK: widen slots so BCLK = 64 * fs. 16-bit samples are sent
          // MSB first and padded; the codec reads 16 bits and ignores the rest.
        if (!_useMclk)
            std_cfg.slot_cfg.slot_bit_width = I2S_SLOT_BIT_WIDTH_32BIT;

        err = i2s_channel_init_std_mode(_txHandle, &std_cfg);
        if (err == ESP_OK)
            err = i2s_channel_enable(_txHandle);
        if (err != ESP_OK) {
            log_e("i2s std init/enable: %s", esp_err_to_name(err));
            i2s_del_channel(_txHandle);
            _txHandle = nullptr;
            return false;
        }
        return true;
    }

    void deinitI2S() {
        if (_txHandle) {
            i2s_channel_disable(_txHandle);



            i2s_del_channel(_txHandle);
            _txHandle = nullptr;
        }
        supported = false;
    }
};

inline ES8311 ES8311_obj;
