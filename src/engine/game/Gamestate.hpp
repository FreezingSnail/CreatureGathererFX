#pragma once

typedef enum State {
    WORLD = 0,
    BATTLE = 1,
    ARENA = 2,
    SAVING = 3,
} GameState_t;

// class GameState {
// private:
//   Creature OwnedMons[32];
//   Creature *currentParty[3];
//   uint8_t trophies; // bitarray for competeion tracking
//   Item items[8];
// };