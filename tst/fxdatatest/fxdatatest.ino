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

// #include <HardwareSerial.h>

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

BattleEvent battleEventStack[10];
BattleEventPlayer battleEventPlayer;
MenuStack menuStack;
DialogMenu dialogMenu;

#include "opponents_test.hpp"

void setup() {
    Serial.begin(9600);
    arduboy.begin();

    FX::begin(FX_DATA_PAGE);
    // FX::setFont(ArduFont, dcmNormal);   // select default font
    FX::setCursorRange(0, 32767);
}

void loop() {
    Serial.println("Hello, world!");
    test_opponents();
    exit(0);
}
