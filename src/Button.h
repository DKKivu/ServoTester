#pragma once
#include <Arduino.h>

enum class ButtonEvent { None, Click, LongPress };

class Button {
public:
    explicit Button(uint8_t pin) : _pin(pin) {}

    void begin() {
        pinMode(_pin, INPUT_PULLUP);
        _lastState = HIGH;
    }

    ButtonEvent poll(unsigned long now) {
        const bool state = digitalRead(_pin);

        if (state == LOW && _lastState == HIGH) {
            _pressTime = now;
            _longPressHandled = false;
        }

        if (state == HIGH && _lastState == LOW) {
            _lastState = state;
            if (!_longPressHandled && (now - _pressTime < CLICK_MAX_MS))
                return ButtonEvent::Click;
            return ButtonEvent::None;
        }

        if (state == LOW && !_longPressHandled &&
            (now - _pressTime > LONG_PRESS_MS)) {
            _longPressHandled = true;
            _lastState = state;
            return ButtonEvent::LongPress;
        }

        _lastState = state;
        return ButtonEvent::None;
    }

private:
    static constexpr unsigned long LONG_PRESS_MS = 700;
    static constexpr unsigned long CLICK_MAX_MS  = 500;

    uint8_t       _pin;
    bool          _lastState        = HIGH;
    unsigned long _pressTime        = 0;
    bool          _longPressHandled = false;
};