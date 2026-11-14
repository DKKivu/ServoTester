#pragma once
#include <ESP32Servo.h>
#include "IEscOutput.h"

class PwmEscOutput : public IEscOutput {
public:
    explicit PwmEscOutput(uint8_t pin) : _pin(pin) {}

    void begin(int minPulseUs, int maxPulseUs) override {
        _min = minPulseUs;
        _max = maxPulseUs;
        _servo.setPeriodHertz(50);
        _servo.attach(_pin, minPulseUs, maxPulseUs);
        _raw = (minPulseUs + maxPulseUs) / 2;
        _servo.writeMicroseconds(_raw);
    }

    void setRaw(int us) override {
        _raw = constrain(us, _min, _max);
        _servo.writeMicroseconds(_raw);
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

    const char* name() const override { return "PWM"; }

private:
    uint8_t _pin;
    Servo   _servo;
    int     _min = 1000;
    int     _max = 2000;
    int     _raw = 1500;
};