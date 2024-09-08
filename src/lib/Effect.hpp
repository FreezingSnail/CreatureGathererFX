#pragma once
#include <stdint.h>

enum class EffectResults {
    NO_EFFECT,
    TARGET_SELF,
    SKIP_TURN
};

enum class Effect {
    NONE,
    // type down
    DPRSD,
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

inline bool isStatEffect(Effect effect) {
    return effect >= Effect::ATKDWN && effect <= Effect::SPDUP;
}

inline bool isTypeStatusEffect(Effect effect) {
    return effect >= Effect::DPRSD && effect <= Effect::EVOLVD;
}

inline bool isTickEffect(Effect effect) {
    return effect >= Effect::SAPPD && effect <= Effect::INFSED;
}
