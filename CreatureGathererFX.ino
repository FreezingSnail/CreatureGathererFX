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
#include "src/lib/FxReadCounter.hpp"
#ifdef CGFX_BATTLE_PRESENTATION_SPIKE
#include "tst/fxdatatest/battlepresentation_fixture.hpp"
#endif
#if defined(CGFX_SHIPPING_NO_USB)
#include <avr/power.h>
#endif

// #include <HardwareSerial.h>

decltype(arduboy) arduboy;

GameState gameState;
ModeState modeState;
MenuV2 menu = MenuV2();
Player player = Player();

Arena arena = Arena();
Animator animator = Animator();
PlantGameState plants;

BattleEvent battleEventStack[10];
BattleEventPlayer battleEventPlayer;
MenuStack menuStack;
DialogMenu dialogMenu;
ScriptVm vm;
uint8_t *buffer;

// Arduboy2's ARDUBOY_NO_USB entry point removes USB attach and serialEventRun,
// but this installed mainNoUSB() omits initVariant(). This shipping entry point
// keeps the core's full startup order and its DOWN-at-bootloader recovery path.
#if defined(CGFX_SHIPPING_NO_USB)
void initVariant() __attribute__((weak));
int main(void) __attribute__((OS_main));
int main(void) {
    UDCON = _BV(DETACH);
    UDIEN = 0;
    UDINT = 0;
    USBCON = _BV(FRZCLK);
    UHWCON = 0;
    power_usb_disable();

    init();
    if (initVariant != nullptr) {
        initVariant();
    }

    bitSet(DOWN_BUTTON_PORT, DOWN_BUTTON_BIT);
    bitClear(DOWN_BUTTON_DDR, DOWN_BUTTON_BIT);
    Arduboy2Core::delayByte(10);
    if (bitRead(DOWN_BUTTON_PORTIN, DOWN_BUTTON_BIT) == 0) {
        Arduboy2Core::exitToBootloader();
    }

    setup();
    for (;;) {
        loop();
    }
}
#endif

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
#ifdef CGFX_BATTLE_PRESENTATION_SPIKE
    battle_presentation_fixture::shippingSpike();
#endif
    gameState.playerLocation = static_cast<uint16_t>(3) |
                               (static_cast<uint16_t>(2) << 8);
    enterBattle();
    exitBattle();

    gameState.state = GameState_t::WORLD;
    player.basic();

    // buffer = arduboy.sBuffer;
}


void run() {
    switch (gameState.state) {
    case GameState_t::BATTLE:
        drawScene(legacyBattle());
        break;
    case GameState_t::WORLD:
        WorldEngine::runMap(worldState());
        break;
    case GameState_t::ARENA:
        if (arena.arenaLoop(menu, player)) {
            enterBattle();
            arena.startBattle(legacyBattle(), player, menu);
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
        menu.printMenu(battle::legacyBattleView(legacyBattle()));
    }
}

uint8_t render() {
    // drawScriptText(1);

    switch (gameState.state) {
    case GameState_t::BATTLE:
        drawScene(legacyBattle());
        return 0;
    case GameState_t::WORLD:
    {
        const uint8_t rowsRead = drawMapFast(worldState());
        drawPlayer();
        return rowsRead;
    }
    case GameState_t::ARENA:
        arena.drawarenaLoop(menu, player);
        return 0;
    case GameState_t::SAVING:
        SaveController::drawStatus();
        return 0;
    }
    // animator.play();
    // if (dialogMenu.peek()) {
    //     dialogMenu.drawPopMenu();
    // } else {
    //     menu.printMenu(engine);
    // }
    return 0;
}

void loop() {
    if (!arduboy.nextFrame()) return;
#if defined(FX_READ_COUNTER) || defined(DEBUG)
    FxReadCounter::resetFrame();
#endif
    arduboy.pollButtons();
    run();
#if defined(FX_READ_COUNTER) || defined(DEBUG)
    FxReadCounter::markUpdate();
#endif
    const uint8_t expectedRenderReads = render();
#if defined(FX_READ_COUNTER) || defined(DEBUG)
    FxReadCounter::renderExact(expectedRenderReads);
    if (!FxReadCounter::framePassed()) {
        for (;;) {}
    }
#endif
    FX::display(CLEAR_BUFFER);
}
