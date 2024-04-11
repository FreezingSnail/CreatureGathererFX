#define ABG_IMPLEMENTATION
#define SPRITESU_IMPLEMENTATION
#include "src/common.hpp"

#include "src/engine/arena/Arena.hpp"
#include "src/engine/battle/Battle.hpp"
#include "src/engine/game/Gamestate.hpp"
#include "src/engine/menu/MenuV2.hpp"
#include "src/engine/world/Event.hpp"
#include "src/engine/world/World.hpp"
#include "src/fxdata.h"
#include "src/player/Player.hpp"
#include "src/plants/PlantGamestate.hpp"

decltype(arduboy) arduboy;

uint8_t debug;

//ARDUBOY_NO_USB
PlantGameState plants;
Player player = Player();
GameState_t state;
BattleEngine engine;
Arena arena = Arena();
WorldEngine world;
MenuV2 menu2 = MenuV2();
Animator animator = Animator();

void setup() {
    // arduboy.begin();
    // arduboy.setFrameRate(45);

    arduboy.boot();
    arduboy.startGray();
    arduboy.initRandomSeed();
    plants.tick();

    FX::begin(FX_DATA_PAGE);
    FX::setFont(Font, dcmNormal);   // select default font
    FX::setCursorRange(0, 32767);

    world.init(&arduboy, &state, &engine, &menu2);
    state = GameState_t::WORLD;
    engine.init(&state, &menu2);
    player.basic();
    engine.startArena(player, 0);
    //     engine.startArena(arduboy, player, 6);
}

void run() {

    if (menu2.dialogMenu.peek()) {
        menu2.run(engine);
        return;
    }

    if (animator.playing) {
        return;
    }

    switch (state) {
    case GameState_t::BATTLE:
        engine.encounter(player);
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

void render() {
    switch (state) {
    case GameState_t::BATTLE:
        engine.drawScene();
        if (menu2.dialogMenu.peek()) {
            menu2.dialogMenu.drawPopMenu();
        } else {
            menu2.printMenu(engine);
        }
        break;
    case GameState_t::WORLD:
        world.draw();
        break;
    case GameState_t::ARENA:
        arena.arenaLoop(menu2, player, engine);
        break;
    }
    animator.play();
}

void loop() {
    FX::enableOLED();
    arduboy.waitForNextPlane();
    FX::disableOLED();

    if (arduboy.needsUpdate()) {
        arduboy.pollButtons();
        run();
    }
    render();
}
