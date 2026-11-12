#include "OledDisplay.h"

namespace {
    const char* const MAIN_LABELS[]     = {"Manual", "Sweep", "Center", "Step", "Settings"};
    const char* const SETTINGS_LABELS[] = {"MinPulse", "MaxPulse", "Step", "SweepSpd", "Save", "Back"};

    constexpr int ROWS_VISIBLE = 3;

    // Рисует окно из 3 пунктов, центрированное на selected.
    template <typename LabelArray, typename ValueFn>
    void drawMenuList(U8G2& g,
                      const LabelArray& labels,
                      int count, int selected,
                      ValueFn valueFor) {
        int start = selected - 1;
        if (start < 0) start = 0;
        if (start > count - ROWS_VISIBLE) start = count - ROWS_VISIBLE;
        if (start < 0) start = 0;

        for (int i = 0; i < ROWS_VISIBLE; i++) {
            const int idx = start + i;
            if (idx >= count) break;
            g.setCursor(4, 10 + i * 10);
            g.print(idx == selected ? ">" : " ");
            g.print(labels[idx]);

            int value = 0;
            if (valueFor(idx, value)) {
                g.print(":");
                g.print(value);
            }
        }
    }
}

OledDisplay::OledDisplay(uint8_t sdaPin, uint8_t sclPin)
    : _u8g2(U8G2_R0, U8X8_PIN_NONE, sclPin, sdaPin) {}

void OledDisplay::begin() {
    _u8g2.begin();
    _u8g2.setContrast(255);
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
    drawMenuList(_u8g2, MAIN_LABELS, MainMenuCount, selected,
                 [](int, int&) { return false; });
    _u8g2.sendBuffer();
}

void OledDisplay::showSettingsMenu(const Settings& s, int selected) {
    _u8g2.clearBuffer();
    drawMenuList(_u8g2, SETTINGS_LABELS, SettingsMenuCount, selected,
                 [&s](int idx, int& out) {
                     switch (static_cast<SettingsItem>(idx)) {
                         case SettingsItem::MinPulse:   out = s.minPulse;   return true;
                         case SettingsItem::MaxPulse:   out = s.maxPulse;   return true;
                         case SettingsItem::Step:       out = s.step;       return true;
                         case SettingsItem::SweepSpeed: out = s.sweepSpeed; return true;
                         default: return false;
                     }
                 });
    _u8g2.sendBuffer();
}