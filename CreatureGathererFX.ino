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
GameState gameState;
MenuV2 menu = MenuV2();
BattleEngine engine;
Player player = Player();

// ARDUBOY_NO_USB

Arena arena = Arena();
WorldEngine world;
Animator animator = Animator();
PlantGameState plants;

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

    world.init();
    gameState.state = GameState_t::WORLD;
    engine.init();
    player.basic();
    engine.startArena(0);
    //     engine.startArena(arduboy, player, 6);
}

void run() {

    if (menu.dialogMenu.peek()) {
        menu.run(engine);
        return;
    }

    if (animator.playing) {
        return;
    }

    switch (gameState.state) {
    case GameState_t::BATTLE:
        engine.encounter();
        break;
    case GameState_t::WORLD:
        world.runMap();
        uint8_t index;
        // index = rand() % 18;
        // engine.startArena(player, index);
        break;
    case GameState_t::ARENA:
        arena.arenaLoop(menu, player, engine);
        break;
    }
    menu.run(engine);
}

void render() {
    switch (gameState.state) {
    case GameState_t::BATTLE:
        engine.drawScene();
        break;
    case GameState_t::WORLD:
        world.draw();
        break;
    case GameState_t::ARENA:
        arena.arenaLoop(menu, player, engine);
        break;
    }
    animator.play();
    if (menu.dialogMenu.peek()) {
        menu.dialogMenu.drawPopMenu();
    } else {
        menu.printMenu(engine);
    }
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
