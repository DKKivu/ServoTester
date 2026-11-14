#pragma once
#include "IEscOutput.h"

enum class OneShotType { ONESHOT125, ONESHOT42 };

class OneShotEscOutput : public IEscOutput {
public:
    OneShotEscOutput(uint8_t pin, OneShotType type)
        : _pin(pin), _type(type) {}

    void begin(int /*minPulseUs*/, int /*maxPulseUs*/) override {
        // 2 кГц, 15-бит (максимум при 80 МГц APB: 80e6/2000 = 40000 → 15 бит)
        ledcSetup(_channel, 2000, 15);
        ledcAttachPin(_pin, _channel);

        _min = (_type == OneShotType::ONESHOT125) ? 125 : 42;
        _max = (_type == OneShotType::ONESHOT125) ? 250 : 84;
        _raw = _min;
        applyPulse(_raw);
    }

    void setRaw(int us) override {
        _raw = constrain(us, _min, _max);
        applyPulse(_raw);
    }

    void setThrottle(float percent) override {
        percent = constrain(percent, 0.0f, 100.0f);
        int us = _min + static_cast<int>((_max - _min) * percent / 100.0f);
        setRaw(us);
    }

    int getRaw() const override { return _raw; }

    const char* name() const override {
        return (_type == OneShotType::ONESHOT125) ? "OneShot125" : "OneShot42";
    }

    ~OneShotEscOutput() override {
        ledcDetachPin(_pin);
    }

private:
    void applyPulse(int us) {
        const uint32_t periodUs = 500; // 2 кГц
        uint32_t duty = (static_cast<uint32_t>(us) << 15) / periodUs;
        ledcWrite(_channel, duty);
    }

    static constexpr uint8_t _channel = 0;

    uint8_t      _pin;
    OneShotType  _type;
    int          _min, _max, _raw;
};