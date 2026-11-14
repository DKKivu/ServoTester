#pragma once

enum class EscProtocol : int {
    PWM = 0,
    OneShot125,
    OneShot42,
    Multishot,
    DShot150,
    DShot300,
    DShot600,
    DShot1200,
    _Count
};

inline const char* protocolName(EscProtocol p) {
    switch (p) {
        case EscProtocol::PWM:        return "PWM";
        case EscProtocol::OneShot125: return "OneShot125";
        case EscProtocol::OneShot42:  return "OneShot42";
        case EscProtocol::Multishot:  return "Multishot";
        case EscProtocol::DShot150:   return "DShot150";
        case EscProtocol::DShot300:   return "DShot300";
        case EscProtocol::DShot600:   return "DShot600";
        case EscProtocol::DShot1200:  return "DShot1200";
        default: return "?";
    }
}

inline EscProtocol nextProtocol(EscProtocol p) {
    int n = (static_cast<int>(p) + 1) % static_cast<int>(EscProtocol::_Count);
    return static_cast<EscProtocol>(n);
}