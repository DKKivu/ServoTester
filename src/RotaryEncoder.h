#pragma once
#include <ESP32Encoder.h>

class RotaryEncoder {
public:
    RotaryEncoder(uint8_t clkPin, uint8_t dtPin)
        : _clkPin(clkPin), _dtPin(dtPin) {}

    void begin() {
        _enc.attachHalfQuad(_clkPin, _dtPin);
        _enc.setCount(0);
        _lastCount = 0;
    }

    int readDelta() {
        const long count = _enc.getCount();
        const int  delta = static_cast<int>(count - _lastCount);
        _lastCount = count;
        return delta;
    }

private:
    uint8_t      _clkPin, _dtPin;
    ESP32Encoder _enc;
    long         _lastCount = 0;
};