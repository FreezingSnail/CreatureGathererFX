#pragma once

#include <stdint.h>

enum class EffectResults {
    NO_EFFECT,
    TARGET_SELF,
    SKIP_TURN
};

enum class Effect : uint8_t {
    NONE = 255,
    // type down
    DPRSD = 0,
    SOAKED,
    BUFTD,
    SOILED,
    SCRCHD,
    ZAPPED,
    TANGLD,
    REDCD,

    // type up
    ENLTND,
    DRNCHD,
    AIRSWPT,
    GRNDED,
    KINDLD,
    CHRGD,
    ENRCHD,
    EVOLVD,

    // stats
    ATKDWN,
    DEFDWN,
    SPCADWN,
    SPCDDWN,
    SPDDWN,
    ATKUP,
    DEFUP,
    SPCAUP,
    SPCDUP,
    SPDUP,

    // tick down
    SAPPD,

    // tick up
    INFSED,

    // 1/3 wont take turn
    PINNED,
    // 1/4 hurt self
    CONCUSED
};

constexpr bool isStatEffect(Effect effect) {
    return static_cast<uint8_t>(effect) >= static_cast<uint8_t>(Effect::ATKDWN) &&
           static_cast<uint8_t>(effect) <= static_cast<uint8_t>(Effect::SPDUP);
}

constexpr bool isTypeStatusEffect(Effect effect) {
    return static_cast<uint8_t>(effect) >= static_cast<uint8_t>(Effect::DPRSD) &&
           static_cast<uint8_t>(effect) <= static_cast<uint8_t>(Effect::EVOLVD);
}

constexpr bool isTickEffect(Effect effect) {
    return static_cast<uint8_t>(effect) >= static_cast<uint8_t>(Effect::SAPPD) &&
           static_cast<uint8_t>(effect) <= static_cast<uint8_t>(Effect::INFSED);
}

constexpr bool isSelfEffect(Effect effect) {
    return (static_cast<uint8_t>(effect) >= static_cast<uint8_t>(Effect::ENLTND) &&
            static_cast<uint8_t>(effect) <= static_cast<uint8_t>(Effect::EVOLVD)) ||
           (static_cast<uint8_t>(effect) >= static_cast<uint8_t>(Effect::ATKUP) &&
            static_cast<uint8_t>(effect) <= static_cast<uint8_t>(Effect::SPDUP)) ||
           effect == Effect::INFSED;
}
