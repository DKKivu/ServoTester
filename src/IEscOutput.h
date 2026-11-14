#pragma once
#include <Arduino.h>

class IEscOutput {
public:
    virtual ~IEscOutput() = default;

    virtual void begin(int minPulseUs, int maxPulseUs) = 0;
    virtual void setRaw(int value) = 0;
    virtual void setThrottle(float percent) = 0;
    virtual int  getRaw() const = 0;
    virtual const char* name() const = 0;
};