#pragma once

enum class Mode : int {
    Manual = 0,
    Sweep,
    Center,
    Step,
    _Count
};

inline Mode nextMode(Mode m) {
    int n = (static_cast<int>(m) + 1) % static_cast<int>(Mode::_Count);
    return static_cast<Mode>(n);
}

enum class MainMenuItem : int {
    Manual = 0, Sweep, Center, Step, Settings, _Count
};
inline constexpr int MainMenuCount = static_cast<int>(MainMenuItem::_Count);


enum class SettingsItem : int {
    MinPulse = 0, MaxPulse, Step, SweepSpeed, MinAngle, MaxAngle, Save, Back, _Count
};
inline constexpr int SettingsMenuCount = static_cast<int>(SettingsItem::_Count);