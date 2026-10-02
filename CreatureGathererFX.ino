#define ABG_IMPLEMENTATION
#define SPRITESU_IMPLEMENTATION
#include "src/common.hpp"
#include "src/globals.hpp"

#include "src/engine/arena/Arena.hpp"
#include "src/engine/battle/Battle.hpp"
#include "src/engine/game/Gamestate.hpp"
#include "src/engine/menu/MenuV2.hpp"
#include "src/engine/world/Event.hpp"
#include "src/engine/world/World.hpp"
#include "src/fxdata.h"
#include "src/save/SaveController.hpp"
#include "src/save/Journal.hpp"
#include "src/player/Player.hpp"
#include "src/plants/PlantGamestate.hpp"
#include "src/engine/draw.h"
#include "src/vm/ScriptVm.hpp"

// #include <HardwareSerial.h>

decltype(arduboy) arduboy;

GameState gameState;
ModeState modeState;
MenuV2 menu = MenuV2();
Player player = Player();

// ARDUBOY_NO_USB

Arena arena = Arena();
Animator animator = Animator();
PlantGameState plants;

BattleEvent battleEventStack[10];
BattleEventPlayer battleEventPlayer;
MenuStack menuStack;
DialogMenu dialogMenu;
ScriptVm vm;
uint8_t *buffer;

void setup() {
    // Serial.begin(9600);
    //  arduboy.begin();
    //  arduboy.setFrameRate(45);
    arduboy.boot();
    arduboy.setFrameRate(52);
    arduboy.initRandomSeed();
    //  plants.tick();

    FX::begin(FX_DATA_PAGE, FX_SAVE_PAGE);
    journalInit();
    // FX::setFont(ArduFont, dcmNormal);   // select default font
    FX::setCursorRange(0, 32767);
    gameState.playerLocation = static_cast<uint16_t>(3) |
                               (static_cast<uint16_t>(2) << 8);
    enterBattle();
    exitBattle();

    gameState.state = GameState_t::WORLD;
    player.basic();
    vm.initVM();

    // buffer = arduboy.sBuffer;
}


void run() {
    switch (gameState.state) {
    case GameState_t::BATTLE:
        drawScene(battle());
        break;
    case GameState_t::WORLD:
        WorldEngine::runMap(worldState());
        break;
    case GameState_t::ARENA:
        if (arena.arenaLoop(menu, player)) {
            enterBattle();
            arena.startBattle(battle(), player, menu);
            gameState.state = GameState_t::BATTLE;
        }
        break;
    case GameState_t::SAVING:
        SaveController::advance();
        return;
    }
    animator.play();
    if (dialogMenu.peek()) {
        dialogMenu.drawPopMenu();
    } else if (gameState.state == GameState_t::BATTLE) {
        menu.printMenu(battle());
    }
}

void render() {
    // drawScriptText(1);

    switch (gameState.state) {
    case GameState_t::BATTLE:
        drawScene(battle());
        break;
    case GameState_t::WORLD:
        drawMapFast(worldState());
        drawPlayer();
        break;
    case GameState_t::ARENA:
        arena.drawarenaLoop(menu, player);
        break;
    case GameState_t::SAVING:
        SaveController::drawStatus();
        return;
    }
    // animator.play();
    // if (dialogMenu.peek()) {
    //     dialogMenu.drawPopMenu();
    // } else {
    //     menu.printMenu(engine);
    // }
}

void loop() {
    if (!arduboy.nextFrame()) return;
    arduboy.pollButtons();
    run();
    render();
    FX::display(CLEAR_BUFFER);
}
