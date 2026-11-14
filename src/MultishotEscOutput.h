#pragma once
#include "IEscOutput.h"

class MultishotEscOutput : public IEscOutput {
public:
    explicit MultishotEscOutput(uint8_t pin) : _pin(pin) {}

    void begin(int /*minPulseUs*/, int /*maxPulseUs*/) override {
        ledcSetup(_channel, 32000, 11); // 32 кГц, 11-бит
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

    int getRaw() const override { return _raw; }

    void setNormalized(float v01) override {
        v01 = constrain(v01, 0.0f, 1.0f);
        const int us = _min + static_cast<int>((_max - _min) * v01 + 0.5f);
        setRaw(us);
    }

    float getNormalized() const override {
        if (_max <= _min) return 0.0f;
        return static_cast<float>(_raw - _min) / static_cast<float>(_max - _min);
    }

    void setThrottle(float percent) override {
        setNormalized(constrain(percent, 0.0f, 100.0f) / 100.0f);
    }

    const char* name() const override { return "Multishot"; }

    ~MultishotEscOutput() override {
        ledcDetachPin(_pin);
    }

private:
    void applyPulse(int us) {
        const uint32_t periodUs = 31;
        const uint32_t duty = (static_cast<uint32_t>(us) << 11) / periodUs;
        ledcWrite(_channel, duty);
    }

    static constexpr uint8_t _channel = 1;

    uint8_t _pin;
    int     _min = 5;
    int     _max = 25;
    int     _raw = 5;
};