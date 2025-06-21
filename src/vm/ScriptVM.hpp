#pragma once
#include <stdint.h>

#include "../GameState.hpp"

#define MAP_WIDTH 256
#define MAP_HEIGHT 256
#define To1D(x, y) ((y) * MAP_WIDTH + (x))

/*

 based on https://github.com/tiberiusbrown/arduboy_grayscale_rpg/blob/master/src/script_commands.hpp

  Action domains:
  - player encounters
  - story events
  - puzzles


  Game manipulation:
    teleport to tiles
    pick up item
    interact with lure tile?





  <tile> is a 16bit number indicating the tile pos in a flat array, from left to right, top to bottom.



*/

enum class SciptCommands {
    cmd_msg,    // msg {text} - display text
    cmd_tmsg,   // tmsg <tile> {text} - display text when on tile

    cmd_tp,   // tp <tile> <tile> - transport from tile a to b

    cmd_end = 255,   // end - ends script
};

class ScriptVm {
  public:
    int8_t memory[8];
    uint8_t stack[8];
    uint16_t currentTile;
    // TODO: looses 16bits of mem over global but allowes testing injection?
    GameState *state;

    // buffer to load the script
    uint8_t *buffer;
    uint8_t *ptr;

    void initVM(uint8_t *buffer, GameState *state) {
        this->buffer = buffer;
        this->ptr = buffer;
        this->state = state;
    };

    void run();
    uint16_t readInt16();
};
