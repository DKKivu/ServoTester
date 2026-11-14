#pragma once
#include "AppTypes.h"
#include "Settings.h"
#include "SettingsStorage.h"
#include "Display.h"
#include "Button.h"
#include "RotaryEncoder.h"
#include "ServoOutput.h"

class ServoTesterApp {
public:
    ServoTesterApp(SettingsStorage& storage,
                   Display&         display,
                   RotaryEncoder&   encoder,
                   Button&          button,
                   ServoOutput&     servo);

    void begin();
    void loop();

private:
    enum class MenuState { Off, Main, Settings };

    // --- Обработка ввода ---
    void handleEncoder(int delta);
    void handleButton(ButtonEvent ev);
    void handleRunningEncoder(int delta);
    void handleMenuEncoder(int delta);
    void handleMainMenuClick();
    void handleSettingsClick();

    // --- Синхронизация углов и импульсов ---
    void recalculateFromMaxAngle();
    void recalculateFromMaxPulse();

    // --- Логика ---
    void updateSweep(unsigned long now);
    void render();

    void enterMenu();
    void exitMenu();
    void cycleMode();
    void setMode(Mode m);
    int  angleFromPulse() const;

    // --- Зависимости (DIP) ---
    SettingsStorage& _storage;
    Display&         _display;
    RotaryEncoder&   _encoder;
    Button&          _button;
    ServoOutput&     _servo;

    // --- Состояние ---
    Settings      _settings;
    Mode          _mode       = Mode::Manual;
    MenuState     _menuState  = MenuState::Off;
    int           _menuIndex  = 0;
    bool          _sweepUp    = true;
    unsigned long _lastSweep  = 0;
};