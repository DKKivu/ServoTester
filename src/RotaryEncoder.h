#pragma once
#include <ESP_Knob.h>

class RotaryEncoder {
public:
    RotaryEncoder(uint8_t pinA, uint8_t pinB)
        : _pinA(pinA), _pinB(pinB) {}

    void begin() {
        _knob = new ESP_Knob(_pinA, _pinB);
        _knob->begin();
        _lastValue = _knob->getCountValue();
    }

    // Возвращает изменение счётчика с момента прошлого вызова.
    int readDelta() {
        const int current = _knob->getCountValue();
        const int delta   = current - _lastValue;
        _lastValue = current;
        return delta;
    }

private:
    uint8_t   _pinA, _pinB;
    ESP_Knob* _knob = nullptr;
    int       _lastValue = 0;
};