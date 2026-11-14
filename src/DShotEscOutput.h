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

        // Фаза 3: армируем ESC только один раз за всё время работы.
        if (!_armed) {
            DSHOT::arm();
            _armed = true;
        }

        _raw = 48; // минимальный throttle (disarmed / 0%)
        _dshot.set(static_cast<uint16_t>(_raw));
    }

    void setRaw(int value) override {
        _raw = constrain(value, 48, 2047);
        _dshot.set(static_cast<uint16_t>(_raw));
    }

    int getRaw() const override { return _raw; }

    void setNormalized(float v01) override {
        v01 = constrain(v01, 0.0f, 1.0f);
        const int value = 48 + static_cast<int>((2047 - 48) * v01 + 0.5f);
        setRaw(value);
    }

    float getNormalized() const override {
        return static_cast<float>(_raw - 48) / static_cast<float>(2047 - 48);
    }

    void setThrottle(float percent) override {
        setNormalized(constrain(percent, 0.0f, 100.0f) / 100.0f);
    }

    const char* name() const override {
        switch (_speed) {
            case DShotSpeed::DSHOT150:  return "DShot150";
            case DShotSpeed::DSHOT300:  return "DShot300";
            case DShotSpeed::DSHOT600:  return "DShot600";
            case DShotSpeed::DSHOT1200: return "DShot1200";
        }
        return "DShot";
    }

    // Явный запрос на переармировку (например, после включения питания ESC).
    static void requestRearm() { _armed = false; }

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

    static inline bool _armed = false; // C++17 inline static member

    uint8_t       _pin;
    DShotSpeed    _speed;
    DSHOT         _dshot;
    int           _raw = 48;
};