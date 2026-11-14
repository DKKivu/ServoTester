#pragma once
#include <Arduino.h>

class IEscOutput {
public:
    virtual ~IEscOutput() = default;

    virtual void begin(int minPulseUs, int maxPulseUs) = 0;

    // "Сырое" значение (us для PWM/OneShot/Multishot; 48..2047 для DShot).
    // Используется только для отображения на экране и диагностики.
    virtual void setRaw(int value) = 0;
    virtual int  getRaw() const = 0;

    // Нормализованное значение [0.0 .. 1.0].
    // Основной интерфейс для ServoTesterApp — не зависит от единиц протокола.
    virtual void  setNormalized(float v01) = 0;
    virtual float getNormalized() const = 0;

    // Устаревший метод, оставлен для совместимости.
    virtual void setThrottle(float percent) = 0;

    virtual const char* name() const = 0;
};