// AHT20 ARDUINO LIBRARY (header-only)
//
// Temperature / humidity sensor over I2C.
//
// Usage:
//     #include "AHT20.hpp"
//     AHT20 aht;
//     aht.begin();              // or aht.begin(&Wire1);
//     float t, h;
//     if (aht.getdData(&h, &t)) { ... }

#ifndef __AHT20_HPP__
#define __AHT20_HPP__

#ifdef HAS_AHT20

#include <Arduino.h>
#include <Wire.h>

#define AHT20_ADDRESS 0x38
#define AHT20_CALIBRATE 0xE1     ///< Calibration Command
#define AHT20_INITIALIZE 0xBE    ///< Init Command
#define AHT20_TRIGGER 0xAC       ///< Trigger reading command
#define AHT20_SOFTRESET 0xBA     ///< Soft reset command
#define AHT20_STATUS 0x71        ///< Status byte command

#define AHTX0_STAT_BUSY 0x80
#define AHTX0_STAT_CALIB 0x08

#define AHT20_MEAS_TIMEOUT_MS 200   ///< Max wait for a conversion
#define AHT20_CACHE_MS 3000         ///< Reuse a reading for this long

class AHT20 {
 private:
    TwoWire *_wire = nullptr;
    uint32_t lastq = 0;
    bool haveData = false;

    // Wait for the sensor to finish a conversion. Returns false on timeout.
    bool startSensor() {
        _wire->beginTransmission(AHT20_ADDRESS);
        _wire->write(AHT20_TRIGGER);
        _wire->write(0x33);
        _wire->write(0x00);
        if (_wire->endTransmission() != 0)
            return false;

        uint32_t timer_s = millis();
        while (millis() - timer_s < AHT20_MEAS_TIMEOUT_MS) {
            delay(20);

            if (_wire->requestFrom(AHT20_ADDRESS, 1) != 1)
                continue;

            unsigned char c = _wire->read();
            if ((c & AHTX0_STAT_BUSY) == 0)
                return true;      // conversion complete
        }

        return false;             // time out
    }

 public:
    float temperature = 0.0;      ///< Last reading, degrees C
    float humidity = 0.0;         ///< Last reading, percent RH

    bool begin(TwoWire *wireInstance = &Wire) {
        _wire = wireInstance;

        _wire->beginTransmission(AHT20_ADDRESS);
        _wire->write(AHT20_INITIALIZE);
        _wire->write(0x08);
        _wire->write(0x00);
        if (_wire->endTransmission() != 0)
            return false;

        delay(10);
        return isCalibrated();
    }

    void reset() {
        if (_wire == nullptr) return;

        _wire->beginTransmission(AHT20_ADDRESS);
        _wire->write(AHT20_SOFTRESET);
        _wire->endTransmission();
        delay(20);

        haveData = false;
    }

    uint8_t status() {
        if (_wire == nullptr) return 0xFF;

        if (_wire->requestFrom(AHT20_ADDRESS, 1) != 1)
            return 0xFF;

        return _wire->read();
    }

    bool isBusy() { return (status() & AHTX0_STAT_BUSY) != 0; }

    bool isCalibrated() {
        uint8_t s = status();
        return (s != 0xFF) && ((s & AHTX0_STAT_CALIB) != 0);
    }

    bool readData() {
        if (_wire == nullptr)
            return false;

        if (!startSensor())
            return false;

        if (_wire->requestFrom(AHT20_ADDRESS, 6) != 6)
            return false;

        unsigned char str[6] = {0};
        uint8_t index = 0;

        while (_wire->available() && index < sizeof(str))
            str[index++] = _wire->read();

        if (index != sizeof(str))
            return false;

        if (str[0] & AHTX0_STAT_BUSY)
            return false;

        // 20 bit humidity: str[1], str[2] and the high nibble of str[3]
        uint32_t _h = ((uint32_t)str[1] << 12) |
                      ((uint32_t)str[2] << 4) |
                      ((uint32_t)str[3] >> 4);
        humidity = ((float)_h / 1048576.0) * 100.0;

        // 20 bit temperature: low nibble of str[3], str[4], str[5]
        uint32_t _t = ((uint32_t)(str[3] & 0x0F) << 16) |
                      ((uint32_t)str[4] << 8) |
                      (uint32_t)str[5];
        temperature = ((float)_t / 1048576.0) * 200.0 - 50.0;

        lastq = millis();
        haveData = true;

        return true;
    }

    // qu : force a query rather than reusing a cached reading
    bool getdData(float *h, float *t, bool qu = false) {
        if (qu || !haveData || ((millis() - lastq) >= AHT20_CACHE_MS)) {
            if (!readData())
                return false;
        }

        if (h != nullptr)
            *h = humidity;

        if (t != nullptr)
            *t = temperature;

        return true;
    }

    bool getTemp(float *t, bool qu = false) { return getdData(nullptr, t, qu); }

    bool getHum(float *h, bool qu = false) { return getdData(h, nullptr, qu); }
};

#endif   //  HAS_AHT20
#endif   //  __AHT20_HPP__
