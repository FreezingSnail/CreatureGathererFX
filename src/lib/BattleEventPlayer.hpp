#pragma once
#include <stdint.h>

#include "BattleEventStack.hpp"

// The player handles displaying pushing text to the dialog menu
// As well playing animations
struct BattleEventPlayer {
    int8_t stackPointer = -1;
    bool playing;
    uint8_t frame;
    uint8_t runlength;

    void push(BattleEvent event);
    void tick();
    void start();
    void play();
};