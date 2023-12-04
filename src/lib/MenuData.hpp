#pragma once
#include <ArduboyFX.h>

struct MenuData {
    uint8_t horizontal;
    uint8_t optionCount;
    uint24_t *text[4];
    uint8_t indicies[4][2];
};

enum DialogType {
    TEXT,
    DAMAGE,
    NAME,
    ENEMY_DAMAGE,
    ENEMY_NAME,
    FAINT,
    SWITCH,
    WIN,
    LOSS,
};