#pragma once
#include <Arduino.h>
#include "EscProtocol.h"

struct Settings {
    int minPulse    = 1000;
    int maxPulse    = 2000;
    int step        = 10;    // мкс, используется в PWM
    int stepPercent = 5;     // %, используется в не-PWM
    int sweepSpeed  = 20;
    int minAngle    = 0;
    int maxAngle    = 120;
    EscProtocol protocol = EscProtocol::PWM;

    void clampAll() {
        minPulse    = constrain(minPulse,     500, 1500);
        maxPulse    = constrain(maxPulse,    1500, 2500);
        step        = constrain(step,           1,   50);
        stepPercent = constrain(stepPercent,    1,   20);
        sweepSpeed  = constrain(sweepSpeed,     5,  100);
        minAngle    = constrain(minAngle,       0,  180);
        maxAngle    = constrain(maxAngle,      90,  360);

        if (static_cast<int>(protocol) < 0 ||
            static_cast<int>(protocol) >= static_cast<int>(EscProtocol::_Count)) {
            protocol = EscProtocol::PWM;
        }
    }
};