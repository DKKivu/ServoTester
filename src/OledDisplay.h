#pragma once
#include <U8G2lib.h>
#include "Display.h"

class OledDisplay : public Display {
public:
    OledDisplay(uint8_t sdaPin, uint8_t sclPin);

    void begin() override;
    void showBoot() override;
    void showRunning(Mode mode, int pulseUs, int angleDeg) override;
    void showMainMenu(int selected) override;
    void showSettingsMenu(const Settings& s, int selected) override;

private:
    U8G2_SSD1306_72X40_ER_F_HW_I2C _u8g2;
};