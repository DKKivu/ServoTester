#pragma once
#include "AppTypes.h"
#include "Settings.h"

class Display {
public:
    virtual ~Display() = default;

    virtual void begin() = 0;
    virtual void showBoot() = 0;
    virtual void showRunning(Mode mode, int pulseUs, int angleDeg) = 0;
    virtual void showMainMenu(int selected) = 0;
    virtual void showSettingsMenu(const Settings& s, int selected) = 0;
};