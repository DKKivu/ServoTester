#pragma once
#include <AiEsp32RotaryEncoder.h>

class RotaryEncoder {
public:
    RotaryEncoder(uint8_t clkPin, uint8_t dtPin)
        : _encoder(clkPin, dtPin, -1, -1, 4) {} // -1 = нет пина кнопки/VCC, 4 = фронтов на щелчок

    void begin() {
        _encoder.begin();
        _encoder.setBoundaries(-1000000, 1000000, false); // большой диапазон, без авто-сброса
        _lastValue = _encoder.readEncoder();
    }

    // Возвращает изменение счётчика с момента прошлого вызова.
    int readDelta() {
        const long current = _encoder.readEncoder();
        const int  delta   = static_cast<int>(current - _lastValue);
        _lastValue = current;
        return delta;
    }

private:
    AiEsp32RotaryEncoder _encoder;
    long                 _lastValue = 0;
};