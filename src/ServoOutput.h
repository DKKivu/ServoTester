#pragma once
#include <ESP32Servo.h>

class ServoOutput {
public:
    explicit ServoOutput(uint8_t pin) : _pin(pin) {}

    void begin(int minUs = 500, int maxUs = 2500) {
        _servo.setPeriodHertz(50);
        _servo.attach(_pin, minUs, maxUs);
        _servo.writeMicroseconds(_pulse);
    }

    void setPulse(int us) {
        _pulse = us;
        _servo.writeMicroseconds(us);
    }

    void setPulseClamped(int us, int minUs, int maxUs) {
        setPulse(constrain(us, minUs, maxUs));
    }

    int getPulse() const { return _pulse; }

private:
    uint8_t _pin;
    Servo   _servo;
    int     _pulse = 1500;
};