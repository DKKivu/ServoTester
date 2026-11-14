#pragma once
#include "IEscOutput.h"

enum class OneShotType { ONESHOT125, ONESHOT42 };

class OneShotEscOutput : public IEscOutput {
public:
    OneShotEscOutput(uint8_t pin, OneShotType type)
        : _pin(pin), _type(type) {}

    void begin(int /*minPulseUs*/, int /*maxPulseUs*/) override {
        ledcSetup(_channel, 2000, 15); // 2 кГц, 15-бит
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

    const char* name() const override {
        return (_type == OneShotType::ONESHOT125) ? "OneShot125" : "OneShot42";
    }

    ~OneShotEscOutput() override {
        ledcDetachPin(_pin);
        // Фаза 2: освобождение канала — см. отдельный шаг. Пока оставляем.
    }

private:
    void applyPulse(int us) {
        const uint32_t periodUs = 500; // 2 кГц
        const uint32_t duty = (static_cast<uint32_t>(us) << 15) / periodUs;
        ledcWrite(_channel, duty);
    }

    static constexpr uint8_t _channel = 0;

    uint8_t      _pin;
    OneShotType  _type;
    int          _min = 125;
    int          _max = 250;
    int          _raw = 125;
};