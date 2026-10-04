#include "src/common.hpp"
#include "src/lib/random.hpp"
#include "src/globals.hpp"

#include "src/engine/battle/BattleSession.hpp"
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
    journalInit();
    FX::setCursorRange(0, 32767);
#ifdef CGFX_BATTLE_PRESENTATION_SPIKE
    battle_presentation_fixture::shippingSpike();
#endif
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

    // buffer = arduboy.sBuffer;
}

namespace {

bool battleMenuAt(MenuEnum menuType) {
    return menu.menuPointer >= 0 && menu.stack[menu.menuPointer] == menuType;
}

void openBattleChoiceMenu() {
    if (battleMenuAt(BATTLE_OPTIONS) || battleMenuAt(BATTLE_MOVE_SELECT) ||
        battleMenuAt(BATTLE_CREATURE_SELECT)) {
        return;
    }
    menu.clear();
    menu.push(BATTLE_OPTIONS);
}

void openForcedReplacementMenu(battle::BattleSession &session) {
    if (battleMenuAt(BATTLE_CREATURE_SELECT)) return;
    menu.clear();
    const battle::BattleView view = session.view();
    menu.openMenu(BATTLE_CREATURE_SELECT, view);
    menu.setPartySnapshot(session.partyChoices());
}

void beginBattleResult(battle::BattleSession &session,
                       battle::BattlePresenter &presenter) {
    if (!session.advance()) return;
    presenter.begin(session.result());
}

bool runBattleUpdate() {
    battle::BattleSession &session = battleSession();
    battle::BattlePresenter &presenter = battlePresenter();

    // Presenter owns the entire frame while a result is resident. The edge
    // which completes the last stage cannot reach a newly opened menu.
    if (presenter.stage() != battle::PresenterStage::Idle) {
        if (!presenter.done()) {
            presenter.update(arduboy.justPressed(A_BUTTON));
        } else {
            session.finishPresentation();
            presenter.reset();
            if (session.exitReady()) {
                menu.clear();
                dialogMenu.clear();
                exitBattle();
                gameState.state = GameState_t::WORLD;
                return true;
            }
        }
        return false;
    }

    if (!session.isActive()) return false;
    if (dialogMenu.peek()) dialogMenu.clear();

    if (session.awaitingReplacement()) {
        openForcedReplacementMenu(session);
        const MenuIntent intent = menu.run(session);
        if (intent.kind != MenuIntentKind::None && session.submitIntent(intent)) {
            menu.clear();
            (void)beginBattleResult(session, presenter);
        }
        return false;
    }

    if (session.awaitingPlayer()) {
        openBattleChoiceMenu();
        const MenuIntent intent = menu.run(session);
        if (intent.kind != MenuIntentKind::None && session.submitIntent(intent)) {
            menu.clear();
            (void)beginBattleResult(session, presenter);
        }
        return false;
    }

    (void)beginBattleResult(session, presenter);
    return false;
}

} // namespace

void run() {
    switch (gameState.state) {
    case GameState_t::BATTLE:
        if (runBattleUpdate()) return;
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
