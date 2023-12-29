#define ABG_IMPLEMENTATION
#define SPRITESU_IMPLEMENTATION
#include "src/external/SpritesU.hpp"

#include "src/engine/arena/Arena.hpp"
#include "src/engine/battle/Battle.hpp"
#include "src/engine/game/Gamestate.hpp"
#include "src/engine/menu/MenuV2.hpp"
#include "src/engine/world/Event.hpp"
#include "src/engine/world/World.hpp"
#include "src/fxdata/fxdata.h"
#include "src/player/Player.hpp"

#define ABG_TIMER1
#define ABG_SYNC_PARK_ROW

#include "src/common.hpp"
decltype(arduboy) arduboy;

#define SPRITESU_OVERWRITE
#define SPRITESU_PLUSMASK
#define SPRITESU_RECT
uint8_t debug;

// ARDUBOY_NO_USB

Player player = Player();
GameState_t state;
BattleEngine engine;
Arena arena = Arena();
WorldEngine world;
MenuV2 menu2 = MenuV2();
Font4x6 font = Font4x6();

void setup() {
    arduboy.boot();
    arduboy.startGray();
    arduboy.initRandomSeed();

    FX::begin(FX_DATA_PAGE);
    FX::setFont(font4x6, dcmNormal);   // select default font
    FX::setCursorRange(0, 32767);

    world.init(&arduboy, &state, &engine, &menu2);
    state = GameState_t::BATTLE;
    engine.init(&state, &menu2);
    player.basic();
    engine.startArena(player, 0);
    // engine.startArena(arduboy, player, 6);
}

void loop() {

    FX::enableOLED();
    arduboy.waitForNextPlane();
    FX::disableOLED();
    arduboy.pollButtons();
    debug = arduboy.currentPlane();
    // if (a.needsUpdate())

    if (menu2.dialogMenu.peek()) {
        menu2.run(engine);
        menu2.dialogMenu.drawPopMenu();
        // FX::display();
        return;
    } else {
        // FX::display(CLEAR_BUFFER);
    }

    switch (state) {
    case GameState_t::BATTLE:
        engine.encounter(player);
        engine.drawScene();
        break;
    case GameState_t::WORLD:
        world.runMap(&player);
        uint8_t index;
        // index = rand() % 18;
        // engine.startArena(player, index);
        break;
    case GameState_t::ARENA:
        arena.arenaLoop(menu2, player, engine);
        break;
    }

    menu2.run(engine);
}
