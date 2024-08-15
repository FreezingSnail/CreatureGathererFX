#pragma once

enum class Effect {
    NONE = 0xff,
    DPRSD = 0x00,
    SOAKED = 0x01,
    BUFTD = 0x02,
    SOILED = 0x03,
    SCRCHD = 0x04,
    ZAPPED = 0x05,
    TANGLD = 0x06,
    REDCD = 0x07,
    ENLTND = 0x10,
    DRNCHD = 0x11,
    AIRSWPT = 0x12,
    GRNDED = 0x13,
    KINDLD = 0x14,
    CHRGD = 0x15,
    ENRCHD = 0x16,
    EVOLVD = 0x17,
    ATKDWN = 0x21,
    DEFDWN = 0x22,
    SPCADWN = 0x23,
    SPCDDWN = 0x24,
    SPDDWN = 0x25,
    ATKUP = 0x31,
    DEFUP = 0x32,
    SPCAUP = 0x33,
    SPCDUP = 0x34,
    SPDUP = 0x35,
    SAPPD = 0x40,
    INFSED = 0x50
};

inline bool isStatEffect(Effect effect) {
    return uint8_t(effect) >= 0x20 && uint8_t(effect) <= 0x35;
}

inline bool isTypeStatusEffect(Effect effect) {
    return uint8_t(effect) >= 0x00 && uint8_t(effect) <= 0x17;
}

inline bool isTickEffect(Effect effect) {
    return uint8_t(effect) >= 0x40 && uint8_t(effect) <= 0x57;
}