#include "src/Globals.hpp"
#include "src/engine/world/Event.hpp"

Arduboy2 arduboy;
Player player = Player();
GameState_t state;
Menu menu;
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
    menu.init(&arduboy, &state, &player);
    menu2.dialogMenu.init(&arduboy);
    engine.init(&state, &menu2);
    player.basic();
    engine.startArena(arduboy, player, 6);
}

void loop() {
    if (!arduboy.nextFrame()) {
        return;
    }
    FX::display(CLEAR_BUFFER);
    arduboy.pollButtons();

    switch (state) {
    case GameState_t::BATTLE:
        engine.encounter(arduboy, player);
        break;
    case GameState_t::WORLD:
        world.runMap(&player, &menu);
        break;
    case GameState_t::ARENA:
        arena.arenaLoop(arduboy, menu, menu2, player, engine);
        break;
    }

    menu2.run(arduboy, engine);
}
