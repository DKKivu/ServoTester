#pragma once
#include "AppTypes.h"
#include "Settings.h"
#include "SettingsStorage.h"
#include "Display.h"
#include "Button.h"
#include "RotaryEncoder.h"
#include "IEscOutput.h"
#include "EscProtocol.h"

class ServoTesterApp {
public:
    ServoTesterApp(SettingsStorage& storage,
                   Display&         display,
                   RotaryEncoder&   encoder,
                   Button&          button,
                   uint8_t          escPin);

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
    void recalculateFromMaxAngle(int oldMaxAngle, int newMaxAngle);
    void recalculateFromMaxPulse(int oldMaxPulse, int newMaxPulse);

    // --- ESC output ---
    void  rebuildEscOutput();
    float stepNormalized() const;

    // --- Логика ---
    void updateSweep(unsigned long now);
    void render();

    void enterMenu();
    void exitMenu();
    void cycleMode();
    void setMode(Mode m);
    int  angleFromThrottle() const;

    // --- Зависимости ---
    SettingsStorage& _storage;
    Display&         _display;
    RotaryEncoder&   _encoder;
    Button&          _button;
    uint8_t          _escPin;

    IEscOutput*      _esc = nullptr;

    // --- Состояние ---
    Settings      _settings;
    Mode          _mode       = Mode::Manual;
    MenuState     _menuState  = MenuState::Off;
    int           _menuIndex  = 0;
    bool          _sweepUp    = true;
    unsigned long _lastSweep  = 0;
    float         _throttle01 = 0.5f;   // нормализованное положение 0.0..1.0
};