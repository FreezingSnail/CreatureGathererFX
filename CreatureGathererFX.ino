#include "src/common.hpp"
#include "src/lib/random.hpp"
#include "src/globals.hpp"

#include "src/engine/battle/BattleSession.hpp"
#include "src/engine/battle/BattleFlow.hpp"
#include "src/engine/game/Gamestate.hpp"
#include "src/engine/menu/MenuNav.hpp"
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
#ifdef CGFX_TRAINER_DEMO
#include "src/engine/battle/BattlePresets.hpp"
#include "tst/fxdatatest/generated/battle_preset_data.hpp"
#endif
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

Animator animator = Animator();
PlantGameState plants;

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
    rngSeed(static_cast<uint16_t>(Arduboy2Core::generateRandomSeed()));
    //  plants.tick();

    FX::begin(FX_DATA_PAGE, FX_SAVE_PAGE);
#ifndef CGFX_TRAINER_DEMO
    journalInit();
#endif
    FX::setCursorRange(0, 32767);
#ifdef CGFX_BATTLE_PRESENTATION_SPIKE
    battle_presentation_fixture::shippingSpike();
#endif
#ifdef CGFX_TRAINER_DEMO
    gameState.playerLocation = static_cast<uint16_t>(3) |
                               (static_cast<uint16_t>(2) << 8);
#ifdef CGFX_TRAINER_DEMO_SWITCH_DRILL
    battle::applyPlayerPreset(player, BattlePresets::switch_drill);
    const BattlePresets::Preset preset =
        BattlePresets::copyPreset(BattlePresets::switch_drill);
#else
    battle::applyPlayerPreset(player, BattlePresets::opening);
    const BattlePresets::Preset preset =
        BattlePresets::copyPreset(BattlePresets::opening);
#endif
    menu.clear();
    dialogMenu.clear();
    enterBattle();
    battleSession().beginTrainer(preset.trainerId);
    gameState.state = GameState_t::BATTLE;
    return;
#else
    const bool restored = SaveController::load();
    if (!restored) {
        gameState.playerLocation = static_cast<uint16_t>(3) |
                                   (static_cast<uint16_t>(2) << 8);
    }
    enterBattle();
    exitBattle();

    gameState.state = GameState_t::WORLD;
    if (!restored) {
        player.basic();
    }
#endif

    // buffer = arduboy.sBuffer;
}

namespace {
uint8_t battleEdgeButtons() {
    uint8_t buttons = 0;
    if (arduboy.justPressed(LEFT_BUTTON)) buttons |= MENU_NAV_LEFT;
    if (arduboy.justPressed(RIGHT_BUTTON)) buttons |= MENU_NAV_RIGHT;
    if (arduboy.justPressed(DOWN_BUTTON)) buttons |= MENU_NAV_DOWN;
    if (arduboy.justPressed(UP_BUTTON)) buttons |= MENU_NAV_UP;
    if (arduboy.justPressed(A_BUTTON)) buttons |= MENU_EDGE_A;
    if (arduboy.justPressed(B_BUTTON)) buttons |= MENU_EDGE_B;
    return buttons;
}

} // namespace

void run() {
    switch (gameState.state) {
    case GameState_t::BATTLE:
        if (BattleFlow::update(battleEdgeButtons())) return;
        break;
    case GameState_t::WORLD:
        WorldEngine::runMap(worldState());
        break;
    case GameState_t::SAVING:
        SaveController::advance();
        return;
    }
    animator.play();
    if (dialogMenu.peek()) {
        dialogMenu.drawPopMenu();
    } else if (gameState.state == GameState_t::BATTLE) {
        menu.printMenu(battleSession().view());
    }
}

uint8_t render() {
    // drawScriptText(1);

    switch (gameState.state) {
    case GameState_t::BATTLE: {
        battle::BattleView view = battleSession().view();
        battlePresenter().overlay(view);
        drawScene(view);
        battlePresenter().draw();
        return 0;
    }
    case GameState_t::WORLD:
    {
        const uint8_t rowsRead = drawMapFast(worldState());
        drawPlayer();
        return rowsRead;
    }
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
