#include "src/common.hpp"
#include "src/lib/random.hpp"
#include "src/globals.hpp"

#include "src/engine/battle/BattleSession.hpp"
#include "src/engine/battle/BattleFlow.hpp"
#include "src/engine/battle/BattleScene.hpp"
#include "src/engine/arena/ArenaDemo.hpp"
#include "src/engine/game/Gamestate.hpp"
#include "src/engine/menu/MenuNav.hpp"
#include "src/engine/menu/MenuV2.hpp"
#include "src/engine/world/Event.hpp"
#include "src/engine/world/World.hpp"
#include "src/engine/world/LureZone.hpp"
#include "src/item/Inventory.hpp"
#include "src/item/ItemIds.hpp"
#include "src/fxdata.h"
#include "src/audio/Audio.hpp"
#include "src/save/SaveController.hpp"
#include "src/save/Journal.hpp"
#include "src/player/Player.hpp"
#include "src/plants/PlantGamestate.hpp"
#include "src/engine/draw.h"
#include "src/vm/ScriptVm.hpp"
#include "src/lib/FxReadCounter.hpp"
// The generated authored presets are shared by the demo bootstraps and the
// ordinary new-game start, so the header is always present.
#include "src/engine/battle/BattlePresets.hpp"

#if defined(CGFX_ARENA_DEMO) && (defined(CGFX_TRAINER_DEMO) || defined(CGFX_WILD_DEMO))
#error "CGFX_ARENA_DEMO cannot be combined with another demo bootstrap"
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
Player player = Player();

PlantGameState plants;

#if CGFX_DIALOG_QUEUE_ENABLED
DialogMenu dialogMenu;
#endif
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

#ifdef CGFX_LURE_WEDGE
// Playtest seeding: stock the world inventory so the box picker offers a spread
// of lure types and tiers immediately. Wiped/replaced when Player item
// ownership lands in CreatureGathererFX-jp8.5.13.
static void seedLureWedge() {
    item::Inventory &inv = LureZone::inventory();
    item::inventoryAdd(inv, item::ItemKind::Lure, item::lureId(0, 0), 3);
    item::inventoryAdd(inv, item::ItemKind::Lure, item::lureId(1, 1), 2);
    item::inventoryAdd(inv, item::ItemKind::Lure, item::lureId(2, 2), 1);
    item::inventoryAdd(inv, item::ItemKind::Lure, item::lureId(3, 0), 2);
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
    FX::setCursorRange(0, 32767);
#ifdef CGFX_BATTLE_PRESENTATION_SPIKE
    battle_presentation_fixture::shippingSpike();
#endif
#ifdef CGFX_TRAINER_DEMO
    gameState.playerLocation = static_cast<uint16_t>(3) |
                               (static_cast<uint16_t>(2) << 8);
#if defined(CGFX_TRAINER_DEMO_UTILITY)
    battle::applyPlayerPreset(player, BattlePresets::utility);
    const BattlePresets::Preset preset =
        BattlePresets::copyPreset(BattlePresets::utility);
#elif defined(CGFX_TRAINER_DEMO_EXPANSION)
    battle::applyPlayerPreset(player, BattlePresets::opening);
    const BattlePresets::Preset preset =
        BattlePresets::copyPreset(BattlePresets::opening);
#elif defined(CGFX_TRAINER_DEMO_SWITCH_DRILL)
    battle::applyPlayerPreset(player, BattlePresets::switch_drill);
    const BattlePresets::Preset preset =
        BattlePresets::copyPreset(BattlePresets::switch_drill);
#else
    battle::applyPlayerPreset(player, BattlePresets::opening);
    const BattlePresets::Preset preset =
        BattlePresets::copyPreset(BattlePresets::opening);
#endif
    dialogMenu.clear();
    enterBattle();
    battleSession().beginTrainer(preset.trainerId);
    gameState.state = GameState_t::BATTLE;
#if CGFX_AUDIO
    // This demo bootstrap returns before the ordinary audio start block.
    // Enter the fight with the combat theme already armed.
    arduboy.audio.on();
    audio::begin();
    audio::playTrack(MusicTrack_Combat);
#endif
    return;
#elif defined(CGFX_ARENA_DEMO)
    arena::boot();
    return;
#else
#ifdef CGFX_LURE_WEDGE
    // The lure playtest boots inside the pocket without touching a user save.
    const bool restored = false;
#elif defined(CGFX_WILD_DEMO)
    // This preset starts in the normal overworld and keeps the demo independent
    // of a user's save. Its first step is the opt-in encounter trigger.
    battle::applyPlayerPreset(player, BattlePresets::opening);
    gameState.playerLocation = static_cast<uint16_t>(3) |
                               (static_cast<uint16_t>(2) << 8);
    const bool restored = false;
#else
    const bool restored = SaveController::load();
#endif
    if (SaveController::bootRecoveryFailed()) { for (;;) {} }
    if (!restored) {
        // First-beta authored start (data/beta-area.toml start_x/start_y). A
        // restored save keeps its own playerLocation; only a fresh ordinary
        // game enters at the pocket clearing. Byte order stays x low, y high.
        gameState.playerLocation = static_cast<uint16_t>(11) |
                                   (static_cast<uint16_t>(5) << 8);
    }
    enterBattle();
    exitBattle();

    gameState.state = GameState_t::WORLD;
#ifdef CGFX_LURE_WEDGE
    gameState.playerLocation = static_cast<uint16_t>(11) |
                               (static_cast<uint16_t>(5) << 8);
    player.basic();
    seedLureWedge();
    LureZone::showIntro();
#elif !defined(CGFX_WILD_DEMO)
    if (!restored) {
        // Ordinary new game: the authored first-beta party (data/beta-area.toml,
        // generated into data/battle-presets.toml's "beta" preset) replaces the
        // legacy species-only default load. species 0/3/6 at level 3, full HP,
        // authored moves; the one Player bag becomes the battle inventory. A
        // valid restore skips this branch and keeps its own party.
        battle::applyPlayerPreset(player, BattlePresets::beta);
        item::setBattleInventory(&player.inventory);
        SaveController::seedParty();
    }
#endif
#endif

#if CGFX_AUDIO
    // Start the overworld theme after the save load / new-game seed is settled.
    // Music service and save-pause hooks arrive with the next audio bead.
    arduboy.audio.on();
    audio::begin();
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
#ifdef CGFX_ARENA_DEMO
        if (BattleFlow::update(battleEdgeButtons(), arena::finishBattle)) return;
#else
        if (BattleFlow::update(battleEdgeButtons())) return;
#endif
        break;
#if defined(CGFX_ARENA_DEMO) || defined(FX_READ_COUNTER) || defined(TEST)
    case GameState_t::ARENA:
        arena::update(battleEdgeButtons());
        return;
#endif
    case GameState_t::WORLD:
        WorldEngine::runMap(worldState());
        break;
    case GameState_t::SAVING:
        SaveController::advance();
        return;
    }
#if defined(CGFX_FULL_WORLD_VM) || defined(TEST) || defined(FX_READ_COUNTER)
    if (dialogMenu.peek()) {
        dialogMenu.drawPopMenu();
    } else
#endif
    if (gameState.state == GameState_t::BATTLE) {
    }
}

uint8_t render() {
    // drawScriptText(1);

    switch (gameState.state) {
#if defined(CGFX_ARENA_DEMO) || defined(FX_READ_COUNTER) || defined(TEST)
    case GameState_t::ARENA:
        arena::draw();
        return 0;
#endif
    case GameState_t::BATTLE: {
        battle::BattleMode &mode = modeState.battle;
        // The duel scene owns the pick row, centre and result footer, all drawn
        // from resident state; the menu paints only its picker/action pages.
        battle::BattleScene::draw(battleSession().state(),
                                  mode.ui.menu.duelCursor());
        if (battleSession().state().feedback == 0 && mode.ui.menu.initialized())
            mode.ui.menu.draw();
        return 0;
    }
    case GameState_t::WORLD:
    {
        const uint8_t rowsRead = drawMapFast(worldState());
        drawPlayer(worldState());
        LureZone::drawOverlay();
        return rowsRead;
    }
    case GameState_t::SAVING:
        SaveController::drawStatus();
        return 0;
    }
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
#if CGFX_AUDIO
    // Music refill runs after the OLED transaction so its FX fetch never shares
    // SPI with the display, and outside update/render so its reads stay out of
    // the metadata frame budget.
    audio::service();
#endif
}
