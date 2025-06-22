#include "GameState.hpp"
#include "flags/flag_bit_array.hpp"

GameState::GameState() {
    // Player player = Player();
    GameState_t state = GameState_t::BATTLE;
    // BattleEngine engine;
    // Arena arena = Arena();
    // Animator animator = Animator();
    flags = FLAG_BIT_ARRAY;
}

void GameState::setFlag(uint16_t index) {
    // the index is the bit index of the byte array
    uint8_t byteIdx = index / 8;
    uint8_t bitIdx = index % 8;
    uint8_t &byte = flags[byteIdx];
    byte |= (1 << bitIdx);
}

void GameState::clearFlag(uint16_t index) {
    uint8_t byteIdx = index / 8;
    uint8_t bitIdx = index % 8;
    uint8_t &byte = flags[byteIdx];
    byte &= ~(1 << bitIdx);
}

bool GameState::getFlag(uint16_t index) {
    uint8_t byteIdx = index / 8;
    uint8_t bitIdx = index % 8;
    uint8_t &byte = flags[byteIdx];
    return (byte & (1 << bitIdx)) != 0;
}