#include <Arduboy2.h>
#include <ArduboyFX.h>

#include "src/engine/arena/Arena.hpp"
#include "src/engine/battle/Battle.hpp"
#include "src/engine/game/Gamestate.hpp"
#include "src/engine/game/Menu.hpp"
#include "src/engine/game/MenuV2.hpp"
#include "src/engine/world/Event.hpp"
#include "src/engine/world/World.hpp"
#include "src/fxdata/fxdata.h"
#include "src/player/Player.hpp"

Arduboy2 arduboy;
Player player;
GameState_t state;
Menu menu;
BattleEngine engine;
Arena arena;
WorldEngine world;
Event event;
MenuV2 menu2;

void setup() {
    // initiate arduboy instance
    arduboy.begin();
    // engine.startFight(0);
    // engine.startEncounter(1, 1);
    //     here we set the framerate to 15, we do not need to run at
    //     default 60 and it saves us battery life
    arduboy.setFrameRate(45);
    arduboy.initRandomSeed();
    FX::begin(FX_DATA_PAGE);
    FX::setFont(arduboyFont, dcmNormal);   // select default font
    FX::setCursorRange(0, 32767);

    world = WorldEngine(&arduboy, &state, &engine);
    player = Player();
    state = GameState_t::WORLD;
    menu = Menu(&arduboy, &state, &player);
    engine = BattleEngine(&arduboy, &player, &menu, &state);
    arena = Arena(&menu, &player, &engine);
    event.loadEvent(0);
    menu2 = MenuV2(&arduboy);
    menu2.pushEvent(event);
}

void loop() {
    if (!arduboy.nextFrame()) {
        return;
    }
    FX::display(CLEAR_BUFFER);

    arduboy.fillRect(0, 0, 120, 120, WHITE);
    arduboy.fillRect(10, 10, 40, 20, BLACK);
    FX::drawBitmap(20, 20, npc, 0, dbfMasked);

    arduboy.pollButtons();
    switch (state) {
    case GameState_t::BATTLE:
        engine.encounter();
        break;
    case GameState_t::WORLD:
        world.runMap();
        break;
    case GameState_t::ARENA:
        arena.arenaLoop(&arduboy);
        break;
    }

    if (arduboy.justPressed(B_BUTTON)) {
        menu2.popMenu();
    }
    if (arduboy.justPressed(A_BUTTON)) {
        menu2.pushEvent(event);
    }
    menu2.drawPopMenu();
}
