#pragma once
#include "uint24.h"

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
    ESCAPE_ENCOUNTER,
    EFFECTIVENESS,
    GATHERING,
    TEAM_CHANGE,
    PLAYER_EFFECT,
    ENEMY_EFFECT,
};

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