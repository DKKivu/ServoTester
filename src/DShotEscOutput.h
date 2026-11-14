#pragma once
#include <ESP32_DSHOT.h>
#include "IEscOutput.h"

enum class DShotSpeed { DSHOT150, DSHOT300, DSHOT600, DSHOT1200 };

class DShotEscOutput : public IEscOutput {
public:
    DShotEscOutput(uint8_t pin, DShotSpeed speed)
        : _pin(pin), _speed(speed) {}

    void begin(int /*minPulseUs*/, int /*maxPulseUs*/) override {
        _dshot.begin(_pin, mode());
        DSHOT::arm();                 // арming обязателен для ESC
        _raw = 48;                    // минимальный throttle (disarmed)
        _dshot.set(_raw);
    }

    void setRaw(int value) override {
        _raw = constrain(value, 48, 2047);
        _dshot.set(_raw);
    }

    void setThrottle(float percent) override {
        percent = constrain(percent, 0.0f, 100.0f);
        int val = 48 + static_cast<int>((2047 - 48) * percent / 100.0f);
        setRaw(val);
    }

    int getRaw() const override { return _raw; }

    const char* name() const override {
        switch (_speed) {
            case DShotSpeed::DSHOT150:  return "DShot150";
            case DShotSpeed::DSHOT300:  return "DShot300";
            case DShotSpeed::DSHOT600:  return "DShot600";
            case DShotSpeed::DSHOT1200: return "DShot1200";
        }
        return "DShot";
    }

private:
    DSHOT::dshot_mode_t mode() const {
        switch (_speed) {
            case DShotSpeed::DSHOT150:  return DSHOT::DSHOT150;
            case DShotSpeed::DSHOT300:  return DSHOT::DSHOT300;
            case DShotSpeed::DSHOT600:  return DSHOT::DSHOT600;
            case DShotSpeed::DSHOT1200: return DSHOT::DSHOT1200;
        }
        return DSHOT::DSHOT300;
    }

    uint8_t       _pin;
    DShotSpeed    _speed;
    DSHOT         _dshot;
    int           _raw = 48;
};