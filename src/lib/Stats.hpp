#pragma once
#include <stdint.h>

enum class StatType {
    ATTACK_M = 0,
    DEFENSE_M,
    SPEED_M,
    SPECIAL_ATTACK_M,
    SPECIAL_DEFENSE_M,
};

typedef struct stats {
    uint8_t attack;
    uint8_t defense;
    uint8_t hp;
    uint8_t speed;
    uint8_t spcAtk;
    uint8_t spcDef;
} stats_t;