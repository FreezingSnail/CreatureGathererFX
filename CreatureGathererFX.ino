#include "src/Globals.hpp"
#include "src/engine/world/Event.hpp"

Event event;
void setup() {
    arduboy.begin();
    arduboy.setFrameRate(45);
    arduboy.initRandomSeed();

    FX::begin(FX_DATA_PAGE);
    FX::setFont(arduboyFont, dcmNormal);   // select default font
    FX::setCursorRange(0, 32767);

    world = WorldEngine(&arduboy, &state, &engine, &menu2);
    player = Player();
    state = GameState_t::ARENA;
    menu = Menu(&arduboy, &state, &player);
    engine = BattleEngine(&state);
    arena = Arena();
    menu2 = MenuV2(&arduboy);
    event = Event();
}

void loop() {
    if (!arduboy.nextFrame()) {
        return;
    }
    FX::display(CLEAR_BUFFER);
    arduboy.pollButtons();

    switch (state) {
    case GameState_t::BATTLE:
        engine.encounter(&arduboy, &player, &menu);
        break;
    case GameState_t::WORLD:
        world.runMap(&player, &menu);
        break;
    case GameState_t::ARENA:
        arena.arenaLoop(&arduboy, &menu, &player, &engine);
        break;
    }
}
