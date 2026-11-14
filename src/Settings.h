#pragma once
#include <Arduino.h>

struct Settings {
    int minPulse   = 500;  // было 500 — соответствует -60° по даташиту
    int maxPulse   = 2500;  // было 2500 — соответствует +60° по даташиту
    int step       = 10;
    int sweepSpeed = 20;
    int minAngle   = 0;
    int maxAngle   = 180;   // было 180 — официальный ход MG996R

    void clampAll() {
        minPulse   = constrain(minPulse,   0, 1500);
        maxPulse   = constrain(maxPulse,  1500, 3500);
        step       = constrain(step,         1,   50);
        sweepSpeed = constrain(sweepSpeed,   5,  100);
        minAngle   = constrain(minAngle,     0,  180);
        maxAngle   = constrain(maxAngle,    90,  360);
    }
};