#include "src/Globals.hpp"
#include "src/engine/world/Event.hpp"

Arduboy2 arduboy;
Player player = Player();
GameState_t state;
Menu menu;
BattleEngine engine;
Arena arena = Arena();
WorldEngine world;
MenuV2 menu2;

void setup() {
    arduboy.begin();
    arduboy.setFrameRate(45);
    arduboy.initRandomSeed();

    FX::begin(FX_DATA_PAGE);
    FX::setFont(arduboyFont, dcmNormal);   // select default font
    FX::setCursorRange(0, 32767);

    world.init(&arduboy, &state, &engine, &menu2);
    state = GameState_t::ARENA;
    menu.init(&arduboy, &state, &player);
    engine.init(&state);
    menu2.init(&arduboy);
}

void loop() {
    if (!arduboy.nextFrame()) {
        return;
    }
    FX::display(CLEAR_BUFFER);
    arduboy.pollButtons();

    switch (state) {
    case GameState_t::BATTLE:
        engine.encounter(arduboy, player, menu);
        break;
    case GameState_t::WORLD:
        world.runMap(&player, &menu);
        break;
    case GameState_t::ARENA:
        arena.arenaLoop(arduboy, menu, player, engine);
        break;
    }
}
