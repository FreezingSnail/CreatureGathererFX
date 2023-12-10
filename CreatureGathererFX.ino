#include "src/Globals.hpp"
#include "src/engine/arena/Arena.hpp"
#include "src/engine/battle/Battle.hpp"
#include "src/engine/game/Gamestate.hpp"
#include "src/engine/menu/MenuV2.hpp"
#include "src/engine/world/Event.hpp"
#include "src/engine/world/World.hpp"
#include "src/fxdata/fxdata.h"
#include "src/player/Player.hpp"

// ARDUBOY_NO_USB

Arduboy2 arduboy;
Player player = Player();
GameState_t state;
BattleEngine engine;
Arena arena = Arena();
WorldEngine world;
MenuV2 menu2 = MenuV2();

void setup() {
    arduboy.begin();
    arduboy.setFrameRate(45);
    arduboy.initRandomSeed();

    FX::begin(FX_DATA_PAGE);
    FX::setFont(arduboyFont, dcmNormal);   // select default font
    FX::setCursorRange(0, 32767);

    world.init(&arduboy, &state, &engine, &menu2);
    state = GameState_t::ARENA;
    menu2.dialogMenu.init(&arduboy);
    engine.init(&state, &menu2);
    // player.basic();
    // engine.startArena(arduboy, player, 6);
}

void loop() {
    if (!arduboy.nextFrame()) {
        return;
    }
    arduboy.pollButtons();

    if (menu2.dialogMenu.peek()) {
        menu2.run(arduboy, engine);
        menu2.dialogMenu.drawPopMenu();
        FX::display();
        return;
    } else {
        FX::display(CLEAR_BUFFER);
    }

    switch (state) {
    case GameState_t::BATTLE:

        engine.encounter(arduboy, player);
        break;
    case GameState_t::WORLD:
        // world.runMap(&player, &menu);
        uint8_t index;
        index = rand() % 18;
        engine.startArena(arduboy, player, index);
        break;
    case GameState_t::ARENA:
        arena.arenaLoop(arduboy, menu2, player, engine);
        break;
    }
    menu2.run(arduboy, engine);
}
