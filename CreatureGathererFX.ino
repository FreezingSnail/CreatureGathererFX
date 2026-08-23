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
#include "src/save/Compaction.hpp"
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

namespace {
SaveFile saveState = {};
GameState_t stateBeforeSaving = GameState_t::WORLD;
bool savingStarted = false;

void drawSavingStatus(bool failed)
{
    static const uint8_t saving[6][5] = {
        {0x0f, 0x10, 0x0e, 0x01, 0x1e}, // S
        {0x0e, 0x11, 0x1f, 0x11, 0x11}, // A
        {0x11, 0x11, 0x11, 0x0a, 0x04}, // V
        {0x1f, 0x04, 0x04, 0x04, 0x1f}, // I
        {0x11, 0x19, 0x15, 0x13, 0x11}, // N
        {0x0e, 0x10, 0x17, 0x11, 0x0e}, // G
    };
    static const uint8_t failedText[6][5] = {
        {0x1f, 0x10, 0x1e, 0x10, 0x10}, // F
        {0x0e, 0x11, 0x1f, 0x11, 0x11}, // A
        {0x1f, 0x04, 0x04, 0x04, 0x1f}, // I
        {0x10, 0x10, 0x10, 0x10, 0x1f}, // L
        {0x1f, 0x10, 0x1e, 0x10, 0x1f}, // E
        {0x1e, 0x11, 0x11, 0x11, 0x1e}, // D
    };
    const uint8_t (*text)[5] = failed ? failedText : saving;

    // This branch must remain RAM-only: FX reads while save flash is busy corrupt it.
    SpritesU::fillRect(0, 0, 128, 64, WHITE);
    for (uint8_t glyph = 0; glyph < 6; ++glyph) {
        for (uint8_t row = 0; row < 5; ++row) {
            for (uint8_t column = 0; column < 5; ++column) {
                if ((text[glyph][row] & (1 << (4 - column))) != 0) {
                    SpritesU::fillRect(34 + (glyph * 10) + (column * 2),
                                       27 + (row * 2), 2, 2, BLACK);
                }
            }
        }
    }
}

void captureLiveSaveState()
{
    // SaveFile is a compaction buffer, not a second authoritative game state.
    // Snapshot RAM immediately before saveBegin; Compaction restores only the
    // journal-owned party baseline before replay and preserves these other fields.
    saveState.version = SAVE_VERSION;
    saveState.reserved = 0;
    saveState.playerLocation = gameState.playerLocation;
    for (uint8_t i = 0; i < FLAG_BYTES; ++i) {
        saveState.flags[i] = gameState.flags[i];
    }
    for (uint8_t i = 0; i < PARTY_MAX; ++i) {
        saveState.party[i] = player.party[i];
    }
    saveState.plants.plantStages = plants.plantStages;
    saveState.plants.plantPairs[0] = plants.plantPairs[0];
    saveState.plants.plantPairs[1] = plants.plantPairs[1];
    saveState.plants.ticker = plants.ticker;
    for (uint8_t i = 0; i < INVENTORY_BYTES; ++i) {
        saveState.inventory[i] = reinterpret_cast<const uint8_t *>(player.items)[i];
    }
}

void beginSaving(GameState_t returnState)
{
    captureLiveSaveState();
    stateBeforeSaving = returnState;
    savingStarted = true;
    saveBegin();
    gameState.state = GameState_t::SAVING;
}

void advanceSaving()
{
    if (!savingStarted) {
        beginSaving(GameState_t::WORLD);
    }

    if (saveStepAdvance(saveState) == SaveStep::Done) {
        savingStarted = false;
        gameState.state = stateBeforeSaving;
    }
}
} // namespace

// TODO: only needs 4 bits
uint8_t stepTicker = 0;
// TODO: unsure if need
uint8_t ticker = 0;

void setup() {
    // Serial.begin(9600);
    //  arduboy.begin();
    //  arduboy.setFrameRate(45);
    arduboy.boot();
    arduboy.startGray();
    arduboy.initRandomSeed();
    //  plants.tick();

    FX::begin(FX_DATA_PAGE, FX_SAVE_PAGE);
    // FX::setFont(ArduFont, dcmNormal);   // select default font
    FX::setCursorRange(0, 32767);
    gameState.playerLocation = 3 + (256 * 2);

    gameState.state = GameState_t::WORLD;
    engine.init();
    player.basic();
    engine.startArena(0);
    // engine.startArena(3);
    menu.push(MenuEnum::BATTLE_OPTIONS);
    vm.initVM();

    // buffer = arduboy.sBuffer;
}


void handleMovement() {
    ticker++;
    if (gameState.walkingMask == 0) {
        if (arduboy.pressed(LEFT_BUTTON)) {
            gameState.walkingMask |= 0b10000000;
        } else if (arduboy.pressed(RIGHT_BUTTON)) {
            gameState.walkingMask |= 0b01000000;
        } else if (arduboy.pressed(UP_BUTTON)) {
            gameState.walkingMask |= 0b00100000;
        } else if (arduboy.pressed(DOWN_BUTTON)) {
            gameState.walkingMask |= 0b00001000;
        }
    }

    if (gameState.walkingMask != 0) {
        stepTicker += 1;
        switch (gameState.walkingMask) {
        case 0b10000000:
            gameState.xStepOffset += 1;
            break;
        case 0b01000000:
            gameState.xStepOffset -= 1;
            break;
        case 0b00100000:
            gameState.yStepOffset += 1;
            break;
        case 0b00001000:
            gameState.yStepOffset -= 1;
            break;
        }
    }

    if (stepTicker > 15) {
        stepTicker = 0;
        switch (gameState.walkingMask) {
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
        gameState.walkingMask = 0;
        gameState.xStepOffset = 0;
        gameState.yStepOffset = 0;
    }
}

void run() {
    switch (gameState.state) {
    case GameState_t::BATTLE:
        drawScene(engine);
        break;
    case GameState_t::WORLD:
        handleMovement();
        break;
    case GameState_t::ARENA:
        arena.drawarenaLoop(menu, player, engine);
        break;
    case GameState_t::SAVING:
        advanceSaving();
        return;
    }
    animator.play();
    if (dialogMenu.peek()) {
        dialogMenu.drawPopMenu();
    } else {
        menu.printMenu(engine);
    }
}

void render() {
    // drawScriptText(1);

    switch (gameState.state) {
    case GameState_t::BATTLE:
        drawScene(engine);
        break;
    case GameState_t::WORLD:
        drawMapFast();
        drawPlayer();
        break;
    case GameState_t::ARENA:
        arena.drawarenaLoop(menu, player, engine);
        break;
    case GameState_t::SAVING:
        drawSavingStatus(!saveInProgress());
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
    FX::enableOLED();
    arduboy.waitForNextPlane();
    FX::disableOLED();
    if (arduboy.needsUpdate()) {
        arduboy.pollButtons();
        run();
    }
    render();
}

