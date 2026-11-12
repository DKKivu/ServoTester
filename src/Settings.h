#pragma once
#include <Arduino.h>

struct Settings {
    int minPulse   = 500;   // мкс
    int maxPulse   = 2500;  // мкс
    int step       = 10;    // мкс на оборот/клик
    int sweepSpeed = 20;    // мс на шаг

    void clampAll() {
        minPulse   = constrain(minPulse,   500, 1500);
        maxPulse   = constrain(maxPulse,  1500, 2500);
        step       = constrain(step,         1,   50);
        sweepSpeed = constrain(sweepSpeed,   5,  100);
    }
};