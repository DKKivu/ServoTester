#pragma once
#include "IEscOutput.h"

class MultishotEscOutput : public IEscOutput {
public:
    explicit MultishotEscOutput(uint8_t pin) : _pin(pin) {}

    void begin(int /*minPulseUs*/, int /*maxPulseUs*/) override {
        // 32 кГц, 11-бит (максимум при 80 МГц APB: 80e6/32000 = 2500 → 11 бит)
        ledcSetup(_channel, 32000, 11);
        ledcAttachPin(_pin, _channel);

        _min = 5;
        _max = 25;
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
    const char* name() const override { return "Multishot"; }

    ~MultishotEscOutput() override {
        ledcDetachPin(_pin);
    }

private:
    void applyPulse(int us) {
        const uint32_t periodUs = 31; // ~32.26 кГц
        uint32_t duty = (static_cast<uint32_t>(us) << 11) / periodUs;
        ledcWrite(_channel, duty);
    }

    static constexpr uint8_t _channel = 1;

    uint8_t _pin;
    int     _min = 5, _max = 25, _raw = 5;
};