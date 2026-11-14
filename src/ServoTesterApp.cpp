#include "ServoTesterApp.h"
#include <Arduino.h>

#include "PwmEscOutput.h"
#include "OneShotEscOutput.h"
#include "MultishotEscOutput.h"
#include "DShotEscOutput.h"

ServoTesterApp::ServoTesterApp(SettingsStorage& storage,
                               Display&         display,
                               RotaryEncoder&   encoder,
                               Button&          button,
                               uint8_t          escPin)
    : _storage(storage)
    , _display(display)
    , _encoder(encoder)
    , _button(button)
    , _escPin(escPin) {}

void ServoTesterApp::begin() {
    _storage.load(_settings);
    _settings.clampAll();

    // Safety: PWM-серво по центру, ESC-протоколы — на нуле.
    _throttle01 = (_settings.protocol == EscProtocol::PWM) ? 0.5f : 0.0f;

    _encoder.begin();
    _button.begin();

    rebuildEscOutput();

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
            _throttle01 = constrain(_throttle01 + delta * stepNormalized(), 0.0f, 1.0f);
            _esc->setNormalized(_throttle01);
            break;
        case Mode::Sweep:
            _settings.sweepSpeed = constrain(_settings.sweepSpeed + delta, 5, 100);
            break;
        case Mode::Step:
            if (_settings.protocol == EscProtocol::PWM) {
                _settings.step = constrain(_settings.step + delta, 1, 50);
            } else {
                _settings.stepPercent = constrain(_settings.stepPercent + delta, 1, 20);
            }
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
            recalculateFromMaxPulse();
            break;
        case SettingsItem::MaxPulse:
            _settings.maxPulse = constrain(_settings.maxPulse + delta * 10, 1500, 2500);
            recalculateFromMaxPulse();
            break;
        case SettingsItem::Step:
            _settings.step = constrain(_settings.step + delta, 1, 50);
            break;
        case SettingsItem::StepPercent:
            _settings.stepPercent = constrain(_settings.stepPercent + delta, 1, 20);
            break;
        case SettingsItem::SweepSpeed:
            _settings.sweepSpeed = constrain(_settings.sweepSpeed + delta, 5, 100);
            break;
        case SettingsItem::MinAngle:
            _settings.minAngle = constrain(_settings.minAngle + delta, 0, 180);
            recalculateFromMaxAngle();
            break;
        case SettingsItem::MaxAngle:
            _settings.maxAngle = constrain(_settings.maxAngle + delta, 90, 360);
            recalculateFromMaxAngle();
            break;
        case SettingsItem::Protocol: {
            int p = static_cast<int>(_settings.protocol) + delta;
            if (p < 0) p = static_cast<int>(EscProtocol::_Count) - 1;
            if (p >= static_cast<int>(EscProtocol::_Count)) p = 0;
            _settings.protocol = static_cast<EscProtocol>(p);
            // Safety: при переходе на ESC-протокол — минимальный газ.
            if (_settings.protocol != EscProtocol::PWM) {
                _throttle01 = 0.0f;
            }
            rebuildEscOutput();
            break;
        }
        default: break;
    }
}

void ServoTesterApp::handleButton(ButtonEvent ev) {
    switch (_menuState) {
        case MenuState::Off:
            if (ev == ButtonEvent::LongPress) {
                enterMenu();
            } else if (ev == ButtonEvent::Click) {
                if (_mode == Mode::Step) {
                    _throttle01 = constrain(_throttle01 + stepNormalized(), 0.0f, 1.0f);
                    _esc->setNormalized(_throttle01);
                } else {
                    cycleMode();
                }
            }
            break;

        case MenuState::Main:
            if (ev == ButtonEvent::Click)          handleMainMenuClick();
            else if (ev == ButtonEvent::LongPress) exitMenu();
            break;

        case MenuState::Settings:
            if (ev == ButtonEvent::Click)          handleSettingsClick();
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

// -------------------- синхронизация --------------------

void ServoTesterApp::recalculateFromMaxAngle() {
    const float usPerDeg = static_cast<float>(_settings.maxPulse - _settings.minPulse) /
                           static_cast<float>(_settings.maxAngle - _settings.minAngle);
    _settings.maxPulse = _settings.minPulse +
                         static_cast<int>(usPerDeg * (_settings.maxAngle - _settings.minAngle));
    _settings.clampAll();
}

void ServoTesterApp::recalculateFromMaxPulse() {
    const float usPerDeg = static_cast<float>(_settings.maxPulse - _settings.minPulse) /
                           static_cast<float>(_settings.maxAngle - _settings.minAngle);
    if (usPerDeg <= 0.0f) return;
    _settings.maxAngle = _settings.minAngle +
                         static_cast<int>((_settings.maxPulse - _settings.minPulse) / usPerDeg);
    _settings.clampAll();
}

// -------------------- ESC output --------------------

float ServoTesterApp::stepNormalized() const {
    if (_settings.protocol == EscProtocol::PWM) {
        const int range = _settings.maxPulse - _settings.minPulse;
        if (range <= 0) return 0.01f;
        return static_cast<float>(_settings.step) / static_cast<float>(range);
    }
    return _settings.stepPercent / 100.0f;
}

void ServoTesterApp::rebuildEscOutput() {
    if (_esc) {
        delete _esc;
        _esc = nullptr;
    }

    switch (_settings.protocol) {
        case EscProtocol::PWM:
            _esc = new PwmEscOutput(_escPin);
            break;
        case EscProtocol::OneShot125:
            _esc = new OneShotEscOutput(_escPin, OneShotType::ONESHOT125);
            break;
        case EscProtocol::OneShot42:
            _esc = new OneShotEscOutput(_escPin, OneShotType::ONESHOT42);
            break;
        case EscProtocol::Multishot:
            _esc = new MultishotEscOutput(_escPin);
            break;
        case EscProtocol::DShot150:
            _esc = new DShotEscOutput(_escPin, DShotSpeed::DSHOT150);
            break;
        case EscProtocol::DShot300:
            _esc = new DShotEscOutput(_escPin, DShotSpeed::DSHOT300);
            break;
        case EscProtocol::DShot600:
            _esc = new DShotEscOutput(_escPin, DShotSpeed::DSHOT600);
            break;
        case EscProtocol::DShot1200:
            _esc = new DShotEscOutput(_escPin, DShotSpeed::DSHOT1200);
            break;
        default:
            _esc = new PwmEscOutput(_escPin);
            break;
    }

    _esc->begin(_settings.minPulse, _settings.maxPulse);
    _esc->setNormalized(_throttle01);   // восстановление текущей позиции
}

// -------------------- логика --------------------

void ServoTesterApp::cycleMode() {
    setMode(nextMode(_mode));
}

void ServoTesterApp::setMode(Mode m) {
    _mode = m;
    if (_mode == Mode::Center) {
        _throttle01 = 0.5f;
        _esc->setNormalized(_throttle01);
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

    const float step = stepNormalized();
    if (_sweepUp) {
        _throttle01 += step;
        if (_throttle01 >= 1.0f) { _throttle01 = 1.0f; _sweepUp = false; }
    } else {
        _throttle01 -= step;
        if (_throttle01 <= 0.0f) { _throttle01 = 0.0f; _sweepUp = true; }
    }
    _esc->setNormalized(_throttle01);
}

int ServoTesterApp::angleFromThrottle() const {
    // Угол теперь масштабируется в настроенный диапазон [minAngle .. maxAngle].
    return static_cast<int>(
        _settings.minAngle +
        _throttle01 * (_settings.maxAngle - _settings.minAngle) + 0.5f);
}

// -------------------- отрисовка --------------------

void ServoTesterApp::render() {
    switch (_menuState) {
        case MenuState::Off:
            _display.showRunning(_mode,
                                 _esc->name(),
                                 _esc->getRaw(),
                                 _throttle01,
                                 angleFromThrottle());
            break;
        case MenuState::Main:
            _display.showMainMenu(_menuIndex);
            break;
        case MenuState::Settings:
            _display.showSettingsMenu(_settings, _menuIndex);
            break;
    }
}