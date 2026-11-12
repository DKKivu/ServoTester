#pragma once
#include <Preferences.h>
#include "Settings.h"

class SettingsStorage {
public:
    virtual ~SettingsStorage() = default;
    virtual void load(Settings& s) = 0;
    virtual void save(const Settings& s) = 0;
};

class NvsSettingsStorage : public SettingsStorage {
public:
    void load(Settings& s) override {
        _prefs.begin("servo", true);
        s.minPulse   = _prefs.getInt("minPulse",   s.minPulse);
        s.maxPulse   = _prefs.getInt("maxPulse",   s.maxPulse);
        s.step       = _prefs.getInt("step",       s.step);
        s.sweepSpeed = _prefs.getInt("sweepSpeed", s.sweepSpeed);
        s.maxAngle   = _prefs.getInt("maxAngle",   s.maxAngle); // <-- Новая строка
        _prefs.end();
}

    void save(const Settings& s) override {
        _prefs.begin("servo", false);
        _prefs.putInt("minPulse",   s.minPulse);
        _prefs.putInt("maxPulse",   s.maxPulse);
        _prefs.putInt("step",       s.step);
        _prefs.putInt("sweepSpeed", s.sweepSpeed);
        _prefs.putInt("maxAngle",   s.maxAngle); // <-- Новая строка
        _prefs.end();
}


private:
    Preferences _prefs;
};