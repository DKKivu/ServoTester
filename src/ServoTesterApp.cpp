#include "ServoTesterApp.h"
#include <Arduino.h>

ServoTesterApp::ServoTesterApp(SettingsStorage& storage,
                               Display&         display,
                               RotaryEncoder&   encoder,
                               Button&          button,
                               ServoOutput&     servo)
    : _storage(storage)
    , _display(display)
    , _encoder(encoder)
    , _button(button)
    , _servo(servo) {}

void ServoTesterApp::begin() {
    _storage.load(_settings);
    _settings.clampAll();

    _encoder.begin();
    _button.begin();
    _servo.begin(_settings.minPulse, _settings.maxPulse);
    _servo.setPulse(1500);

    _display.begin();
    _display.showBoot();
    delay(800);
}

void ServoTesterApp::loop() {
    const unsigned long now = millis();

    const int delta = _encoder.readDelta();
    if (delta != 0) handleEncoder(delta);

    const ButtonEvent ev = _button.poll(now);
    if (ev != ButtonEvent::None) handleButton(ev);

    updateSweep(now);
    render();

    delay(20);
}

// -------------------- ввод --------------------

void ServoTesterApp::handleEncoder(int delta) {
    if (_menuState == MenuState::Off) handleRunningEncoder(delta);
    else                              handleMenuEncoder(delta);
}

void ServoTesterApp::handleRunningEncoder(int delta) {
    switch (_mode) {
        case Mode::Manual:
            _servo.setPulseClamped(_servo.getPulse() + delta * _settings.step,
                                   _settings.minPulse, _settings.maxPulse);
            break;
        case Mode::Sweep:
            _settings.sweepSpeed = constrain(_settings.sweepSpeed + delta, 5, 100);
            break;
        case Mode::Step:
            _settings.step = constrain(_settings.step + delta, 1, 50);
            break;
        case Mode::Center:
            break;
        default: break;
    }
}

void ServoTesterApp::handleMenuEncoder(int delta) {
    if (_menuState == MenuState::Main) {
            _menuIndex = ((_menuIndex + delta) % MainMenuCount + MainMenuCount)
                        % MainMenuCount;
            return;
        }
    switch (static_cast<SettingsItem>(_menuIndex)) {
        case SettingsItem::MinPulse:
            _settings.minPulse = constrain(_settings.minPulse + delta * 10, 500, 1500);
            recalculateFromMaxPulse(); // синхронизируем угол
            break;
        case SettingsItem::MaxPulse:
            _settings.maxPulse = constrain(_settings.maxPulse + delta * 10, 1500, 2500);
            recalculateFromMaxPulse(); // синхронизируем угол
            break;
        case SettingsItem::Step:
            _settings.step = constrain(_settings.step + delta, 1, 50);
            break;
        case SettingsItem::SweepSpeed:
            _settings.sweepSpeed = constrain(_settings.sweepSpeed + delta, 5, 100);
            break;
        case SettingsItem::MinAngle:
            _settings.minAngle = constrain(_settings.minAngle + delta, 0, 180);
            recalculateFromMaxAngle(); // синхронизируем импульс
            break;
        case SettingsItem::MaxAngle:
            _settings.maxAngle = constrain(_settings.maxAngle + delta, 90, 360);
            recalculateFromMaxAngle(); // синхронизируем импульс
            break;
        default: break;
    }
}
// Вспомогательный метод для пересчёта
void ServoTesterApp::recalculateFromMaxAngle() {
    // Сохраняем текущее разрешение (мкс на градус)
    float usPerDeg = static_cast<float>(_settings.maxPulse - _settings.minPulse) /
                     (_settings.maxAngle - _settings.minAngle);
    // Пересчитываем maxPulse, сохраняя minPulse и разрешение
    _settings.maxPulse = _settings.minPulse +
                         static_cast<int>(usPerDeg * (_settings.maxAngle - _settings.minAngle));
    _settings.clampAll();
}

void ServoTesterApp::recalculateFromMaxPulse() {
    float usPerDeg = static_cast<float>(_settings.maxPulse - _settings.minPulse) /
                     (_settings.maxAngle - _settings.minAngle);
    if (usPerDeg <= 0.0f) return; // защита от деления на ноль
    // Пересчитываем maxAngle, сохраняя minAngle и разрешение
    _settings.maxAngle = _settings.minAngle +
                         static_cast<int>((_settings.maxPulse - _settings.minPulse) / usPerDeg);
    _settings.clampAll();
}

void ServoTesterApp::handleButton(ButtonEvent ev) {
    switch (_menuState) {
        case MenuState::Off:
            if (ev == ButtonEvent::LongPress) {
                enterMenu();
            } else if (ev == ButtonEvent::Click) {
                if (_mode == Mode::Step) {
                    _servo.setPulseClamped(_servo.getPulse() + _settings.step,
                                           _settings.minPulse, _settings.maxPulse);
                } else {
                    cycleMode();
                }
            }
            break;

        case MenuState::Main:
            if (ev == ButtonEvent::Click)      handleMainMenuClick();
            else if (ev == ButtonEvent::LongPress) exitMenu();
            break;

        case MenuState::Settings:
            if (ev == ButtonEvent::Click)      handleSettingsClick();
            else if (ev == ButtonEvent::LongPress) {
                _menuState = MenuState::Main;
                _menuIndex = 0;
            }
            break;
    }
}

void ServoTesterApp::handleMainMenuClick() {
    switch (static_cast<MainMenuItem>(_menuIndex)) {
        case MainMenuItem::Manual: setMode(Mode::Manual); break;
        case MainMenuItem::Sweep:  setMode(Mode::Sweep);  break;
        case MainMenuItem::Center: setMode(Mode::Center); break;
        case MainMenuItem::Step:   setMode(Mode::Step);   break;
        case MainMenuItem::Settings:
            _menuState = MenuState::Settings;
            _menuIndex = 0;
            return;
        default: return;
    }
    _menuState = MenuState::Off;
}

void ServoTesterApp::handleSettingsClick() {
    switch (static_cast<SettingsItem>(_menuIndex)) {
        case SettingsItem::Save:
            _storage.save(_settings);
            _menuState = MenuState::Main;
            _menuIndex = 0;
            break;
        case SettingsItem::Back:
            _menuState = MenuState::Main;
            _menuIndex = 0;
            break;
        default:
            _menuIndex = (_menuIndex + 1) % SettingsMenuCount;
            break;
    }
}

// -------------------- логика --------------------

void ServoTesterApp::cycleMode() {
    setMode(nextMode(_mode));
}

void ServoTesterApp::setMode(Mode m) {
    _mode = m;
    if (_mode == Mode::Center) {
        const int center = (_settings.minPulse + _settings.maxPulse) / 2;
        _servo.setPulse(center);
    }
    if (_mode == Mode::Sweep) {
        _sweepUp = true;
    }
}

void ServoTesterApp::enterMenu() {
    _menuState = MenuState::Main;
    _menuIndex = 0;
}

void ServoTesterApp::exitMenu() {
    _menuState = MenuState::Off;
}

void ServoTesterApp::updateSweep(unsigned long now) {
    if (_mode != Mode::Sweep || _menuState != MenuState::Off) return;
    if (now - _lastSweep < static_cast<unsigned long>(_settings.sweepSpeed)) return;
    _lastSweep = now;

    int p = _servo.getPulse();
    if (_sweepUp) {
        p += _settings.step;
        if (p >= _settings.maxPulse) { p = _settings.maxPulse; _sweepUp = false; }
    } else {
        p -= _settings.step;
        if (p <= _settings.minPulse) { p = _settings.minPulse; _sweepUp = true;  }
    }
    _servo.setPulse(p);
}

int ServoTesterApp::angleFromPulse() const {
    return static_cast<int>(map(_servo.getPulse(),
                                _settings.minPulse, _settings.maxPulse, 0, _settings.maxAngle)); // <-- maxAngle
}

// -------------------- отрисовка --------------------

void ServoTesterApp::render() {
    switch (_menuState) {
        case MenuState::Off:
            _display.showRunning(_mode, _servo.getPulse(), angleFromPulse());
            break;
        case MenuState::Main:
            _display.showMainMenu(_menuIndex);
            break;
        case MenuState::Settings:
            _display.showSettingsMenu(_settings, _menuIndex);
            break;
    }
}