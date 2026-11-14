#include "OledDisplay.h"

namespace {
    const char* const MAIN_LABELS[]     = {"Manual", "Sweep", "Center", "Step", "Settings"};
    const char* const SETTINGS_LABELS[] = {"MinPulse", "MaxPulse", "Step", "SweepSpd", "MinAngle", "MaxAngle", "Save", "Back"};
}

OledDisplay::OledDisplay(uint8_t sdaPin, uint8_t sclPin)
    : _u8g2(U8G2_R0, U8X8_PIN_NONE, sclPin, sdaPin) {}

void OledDisplay::begin() {
    _u8g2.begin();
    _u8g2.setContrast(255);
    _u8g2.setBusClock(400000);
    _u8g2.setFont(u8g2_font_5x7_tf);
}

void OledDisplay::showBoot() {
    _u8g2.clearBuffer();
    _u8g2.drawStr(0, 10, "Servo Tester");
    _u8g2.drawStr(0, 20, "ESP32-C3");
    _u8g2.sendBuffer();
}

void OledDisplay::showRunning(Mode mode, int pulseUs, int angleDeg) {
    _u8g2.clearBuffer();

    _u8g2.setCursor(0, 8);
    switch (mode) {
        case Mode::Manual: _u8g2.print("MANUAL"); break;
        case Mode::Sweep:  _u8g2.print("SWEEP");  break;
        case Mode::Center: _u8g2.print("CENTER"); break;
        case Mode::Step:   _u8g2.print("STEP");   break;
        default: break;
    }

    _u8g2.setCursor(0, 18);
    _u8g2.print("P:");  _u8g2.print(pulseUs);  _u8g2.print("us");

    _u8g2.setCursor(0, 28);
    _u8g2.print("A:");  _u8g2.print(angleDeg); _u8g2.print((char)176);

    _u8g2.sendBuffer();
}

void OledDisplay::showMainMenu(int selected) {
    _u8g2.clearBuffer();

    int start = selected - 1;
    if (start < 0) start = 0;
    if (start > MainMenuCount - 3) start = MainMenuCount - 3;
    if (start < 0) start = 0;

    for (int i = 0; i < 3; i++) {
        int idx = start + i;
        if (idx >= MainMenuCount) break;
        _u8g2.setCursor(4, 10 + i * 10);
        _u8g2.print(idx == selected ? ">" : " ");
        _u8g2.print(MAIN_LABELS[idx]);
    }
    _u8g2.sendBuffer();
}

void OledDisplay::showSettingsMenu(const Settings& s, int selected) {
    _u8g2.clearBuffer();

    int start = selected - 1;
    if (start < 0) start = 0;
    if (start > SettingsMenuCount - 3) start = SettingsMenuCount - 3;
    if (start < 0) start = 0;

    for (int i = 0; i < 3; i++) {
        int idx = start + i;
        if (idx >= SettingsMenuCount) break;
        _u8g2.setCursor(4, 10 + i * 10);
        _u8g2.print(idx == selected ? ">" : " ");
        _u8g2.print(SETTINGS_LABELS[idx]);

        switch (static_cast<SettingsItem>(idx)) {
            case SettingsItem::MinPulse:   _u8g2.print(":"); _u8g2.print(s.minPulse);   break;
            case SettingsItem::MaxPulse:   _u8g2.print(":"); _u8g2.print(s.maxPulse);   break;
            case SettingsItem::Step:       _u8g2.print(":"); _u8g2.print(s.step);       break;
            case SettingsItem::SweepSpeed: _u8g2.print(":"); _u8g2.print(s.sweepSpeed); break;
            case SettingsItem::MaxAngle: _u8g2.print(":"); _u8g2.print(s.maxAngle); break;
            case SettingsItem::MinAngle: _u8g2.print(":"); _u8g2.print(s.minAngle); break;
            default: break;
        }
    }
    _u8g2.sendBuffer();
}