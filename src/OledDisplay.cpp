#include "OledDisplay.h"
#include <cstring>

namespace {
    const char* const MAIN_LABELS[] = {
        "Manual", "Sweep", "Center", "Step", "Settings"
    };
    const char* const SETTINGS_LABELS[] = {
        "MinPulse", "MaxPulse", "Step", "StepPct", "SweepSpd",
        "MinAngle", "MaxAngle", "Protocol", "Save", "Back"
    };

    const char* modeName(Mode m) {
        switch (m) {
            case Mode::Manual: return "MANUAL";
            case Mode::Sweep:  return "SWEEP";
            case Mode::Center: return "CENTER";
            case Mode::Step:   return "STEP";
            default: return "?";
        }
    }

    const char* abbreviateProtocol(const char* protocol) {
        if (strcmp(protocol, "PWM") == 0)        return "PWM";
        if (strcmp(protocol, "DShot150") == 0)   return "DS150";
        if (strcmp(protocol, "DShot300") == 0)   return "DS300";
        if (strcmp(protocol, "DShot600") == 0)   return "DS600";
        if (strcmp(protocol, "DShot1200") == 0)  return "DS1200";
        if (strcmp(protocol, "OneShot125") == 0) return "OS125";
        if (strcmp(protocol, "OneShot42") == 0)  return "OS42";
        if (strcmp(protocol, "Multishot") == 0)  return "MS";
        return protocol;
    }
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

void OledDisplay::showRunning(Mode mode,
                              const char* protocol,
                              int rawValue,
                              float normalized,
                              int angleDeg) {
    _u8g2.clearBuffer();

    const bool isPwm = (strcmp(protocol, "PWM") == 0);
    const int  pct   = static_cast<int>(normalized * 100.0f + 0.5f);

    // Line 1: MODE PROTO
    _u8g2.setCursor(0, 8);
    _u8g2.print(modeName(mode));
    _u8g2.print(" ");
    _u8g2.print(abbreviateProtocol(protocol));

    // Line 2: primary value
    _u8g2.setCursor(0, 18);
    if (isPwm) {
        _u8g2.print("P:");
        _u8g2.print(rawValue);
        _u8g2.print("us");
    } else {
        _u8g2.print("T:");
        _u8g2.print(pct);
        _u8g2.print("%");
    }

    // Line 3: angle for PWM, raw value for non-PWM (Фаза 4)
    _u8g2.setCursor(0, 28);
    if (isPwm) {
        _u8g2.print("A:");
        _u8g2.print(angleDeg);
        _u8g2.print((char)176);   // символ градуса
    } else {
        _u8g2.print("R:");
        _u8g2.print(rawValue);    // "сырое" значение (48..2047 для DShot, us для OS/MS)
    }

    _u8g2.sendBuffer();
}

void OledDisplay::showMainMenu(int selected) {
    _u8g2.clearBuffer();

    int start = selected - 1;
    if (start < 0) start = 0;
    if (start > MainMenuCount - 3) start = MainMenuCount - 3;
    if (start < 0) start = 0;

    for (int i = 0; i < 3; i++) {
        const int idx = start + i;
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
        const int idx = start + i;
        if (idx >= SettingsMenuCount) break;
        _u8g2.setCursor(4, 10 + i * 10);
        _u8g2.print(idx == selected ? ">" : " ");
        _u8g2.print(SETTINGS_LABELS[idx]);

        switch (static_cast<SettingsItem>(idx)) {
            case SettingsItem::MinPulse:
                _u8g2.print(":"); _u8g2.print(s.minPulse); break;
            case SettingsItem::MaxPulse:
                _u8g2.print(":"); _u8g2.print(s.maxPulse); break;
            case SettingsItem::Step:
                _u8g2.print(":"); _u8g2.print(s.step); break;
            case SettingsItem::StepPercent:
                _u8g2.print(":"); _u8g2.print(s.stepPercent); _u8g2.print("%"); break;
            case SettingsItem::SweepSpeed:
                _u8g2.print(":"); _u8g2.print(s.sweepSpeed); break;
            case SettingsItem::MinAngle:
                _u8g2.print(":"); _u8g2.print(s.minAngle); break;
            case SettingsItem::MaxAngle:
                _u8g2.print(":"); _u8g2.print(s.maxAngle); break;
            case SettingsItem::Protocol:
                _u8g2.print(":"); _u8g2.print(protocolName(s.protocol)); break;
            default: break;
        }
    }
    _u8g2.sendBuffer();
}