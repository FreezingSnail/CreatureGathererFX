#include "SaveController.hpp"

#include "../common.hpp"
#include "../globals.hpp"
#include "../plants/PlantGamestate.hpp"
#include "Compaction.hpp"
#include "SaveFile.hpp"

namespace SaveController {
namespace {

SaveFile saveState = {};
GameState_t stateBeforeSaving = GameState_t::WORLD;
bool savingStarted = false;

// SaveFile is a compaction buffer, not a second authoritative game state.
// Snapshot RAM immediately before saveBegin; Compaction restores only the
// journal-owned party baseline before replay and preserves these other fields.
void captureLiveSaveState()
{
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

    // Glyphs are drawn from RAM bitmaps instead of the FX font: FX reads while
    // save flash is busy corrupt it.
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

} // namespace

void begin(GameState_t returnState)
{
    captureLiveSaveState();
    stateBeforeSaving = returnState;
    savingStarted = true;
    saveBegin();
    gameState.state = GameState_t::SAVING;
}

void advance()
{
    if (!savingStarted) {
        begin(GameState_t::WORLD);
    }

    if (saveStepAdvance(saveState) == SaveStep::Done) {
        savingStarted = false;
        gameState.state = stateBeforeSaving;
    }
}

void drawStatus()
{
    drawSavingStatus(!saveInProgress());
}

} // namespace SaveController
