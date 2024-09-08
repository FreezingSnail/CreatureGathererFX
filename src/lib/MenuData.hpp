#pragma once
#include "uint24.h"

struct MenuData {
    uint8_t horizontal;
    uint8_t optionCount;
    uint24_t *text[4];
    uint8_t indicies[4][2];
};

enum DialogType {
    TEXT,               // 0
    DAMAGE,             // 1
    NAME,               // 2
    ENEMY_DAMAGE,       // 3
    ENEMY_NAME,         // 4
    FAINT,              // 5
    SWITCH,             // 6
    WIN,                // 7
    LOSS,               // 8
    ESCAPE_ENCOUNTER,   // 9
    EFFECTIVENESS,      // 10
    GATHERING,          // 11
    TEAM_CHANGE,        // 12
    PLAYER_EFFECT,      // 13
    ENEMY_EFFECT,       // 14
};
;

enum class BattleState {
    TURN_INPUT,
    PLAYER_ATTACK,
    OPPONENT_ATTACK,
    PLAYER_RECEIVE_DAMAGE,
    OPPONENT_RECEIVE_DAMAGE,
    PLAYER_RECEIVE_EFFECT_APPLICATION,
    OPPONENT_RECEIVE_EFFECT_APPLICATION,
    END_TURN,
};