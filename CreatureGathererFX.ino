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
#include "src/player/Player.hpp"
#include "src/plants/PlantGamestate.hpp"
#include "src/engine/draw.h"
#include "src/vm/ScriptVm.hpp"

// #include <HardwareSerial.h>

decltype(arduboy) arduboy;

uint8_t debug;
GameState gameState;
MenuV2 menu = MenuV2();
BattleEngine engine;
Player player = Player();

// ARDUBOY_NO_USB

Arena arena = Arena();
// WorldEngine world;
Animator animator = Animator();
PlantGameState plants;

BattleEvent battleEventStack[10];
BattleEventPlayer battleEventPlayer;
MenuStack menuStack;
DialogMenu dialogMenu;
ScriptVm vm;
uint8_t *buffer;
// TODO: these can be condesed to 1 byte since its 0-16
int8_t xStepOffset = 0;
int8_t yStepOffset = 0;
uint8_t walkingMask = 0;
uint8_t stepTicker = 0;
uint8_t ticker = 0;
uint16_t rowbuf[9];

void setup() {
    // Serial.begin(9600);
    //  arduboy.begin();
    //  arduboy.setFrameRate(45);
    arduboy.boot();
    arduboy.startGray();
    arduboy.initRandomSeed();
    //  plants.tick();

    FX::begin(FX_DATA_PAGE);
    // FX::setFont(ArduFont, dcmNormal);   // select default font
    FX::setCursorRange(0, 32767);
    gameState.playerLocation = 3 + (256 * 2);

    gameState.state = GameState_t::BATTLE;
    engine.init();
    player.basic();
    engine.startArena(0);
    // engine.startArena(3);
    menu.push(MenuEnum::BATTLE_OPTIONS);
    vm.initVM(arduboy.sBuffer, &gameState);

    // buffer = arduboy.sBuffer;
}

void run() {
    ticker++;
    if (walkingMask == 0) {
        if (arduboy.pressed(LEFT_BUTTON)) {
            walkingMask |= 0b10000000;
        } else if (arduboy.pressed(RIGHT_BUTTON)) {
            walkingMask |= 0b01000000;
        } else if (arduboy.pressed(UP_BUTTON)) {
            walkingMask |= 0b00100000;
        } else if (arduboy.pressed(DOWN_BUTTON)) {
            walkingMask |= 0b00001000;
        }
    }

    if (walkingMask != 0) {
        stepTicker += 1;
        switch (walkingMask) {
        case 0b10000000:
            xStepOffset += 1;
            break;
        case 0b01000000:
            xStepOffset -= 1;
            break;
        case 0b00100000:
            yStepOffset += 1;
            break;
        case 0b00001000:
            yStepOffset -= 1;
            break;
        }
    }

    if (stepTicker > 15) {
        stepTicker = 0;
        switch (walkingMask) {
        case 0b10000000:
            gameState.playerLocation -= 1;
            break;
        case 0b01000000:
            gameState.playerLocation += 1;
            break;
        case 0b00100000:
            gameState.playerLocation -= 256;
            break;
        case 0b00001000:
            gameState.playerLocation += 256;
            break;
        }
        walkingMask = 0;
        xStepOffset = 0;
        yStepOffset = 0;
    }
}

void render() {

    drawMapFast();

    //  switch (gameState.state) {
    //   case GameState_t::BATTLE:
    //       drawScene(engine);
    //       break;
    //   case GameState_t::WORLD:
    //       break;
    //   case GameState_t::ARENA:
    //       arena.drawarenaLoop(menu, player, engine);
    //       //  return;
    //       break;
    //   }
    //   animator.play();
    //   if (dialogMenu.peek()) {
    //       dialogMenu.drawPopMenu();
    //   } else {
    //       menu.printMenu(engine);
    //   }
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
