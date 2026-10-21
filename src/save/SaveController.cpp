#include "SaveController.hpp"

#include <avr/pgmspace.h>
#include <stddef.h>
#include <string.h>

#include "../audio/Audio.hpp"
#include "../common.hpp"
#include "../globals.hpp"
#include "../plants/PlantGamestate.hpp"
#include "Compaction.hpp"
#include "SaveFile.hpp"
#include "../creature/LevelCurve.hpp"
#include "../flags/flag_bit_array.hpp"

namespace SaveController {
namespace {

// SaveFile.lures[24] + consumables[8] are contiguous and match item::Inventory
// exactly, so one 32-byte copy is the whole snapshot inventory block.
static_assert(offsetof(SaveFile, lures) + sizeof(item::Inventory) ==
                  offsetof(SaveFile, keyItems),
              "snapshot inventory block must be contiguous before keyItems");

SaveFile saveState = {};
static_assert(sizeof(FLAG_BIT_ARRAY) <= sizeof(saveState.flags),
              "SaveFile flags must fit generated game flags");
GameState_t stateBeforeSaving = GameState_t::WORLD;
bool savingStarted = false;

// SaveFile is a compaction buffer, not a second authoritative game state.
// Snapshot RAM immediately before saveBegin; the current party and other
// RAM-resident fields remain in this snapshot.
void captureLiveSaveState()
{
    if (gameState.state == GameState_t::BATTLE) {
        // BattleState owns current HP while the battle union is active. Copy it
        // back before SaveFile snapshots Player, then leave the session live.
        battleSession().syncPlayerHp();
    }
    memset(&saveState, 0, sizeof(saveState));
    saveState.playerLocation = gameState.playerLocation;
    memcpy(saveState.flags, gameState.flags, sizeof(FLAG_BIT_ARRAY));
    for (uint8_t i = 0; i < 3; ++i) {
        const uint8_t slot = storePartySlot(i);
        const uint8_t hp = slot == 255 ? 0 : player.creatureHPs[i];
        saveState.partySlots[i] = slot;
        saveState.partyHP[i] = hp;
        storeSetPartyHP(i, hp);
        if (slot == 255) continue;
        StoreRecord record;
        if (!storeRead(slot, record)) continue;
        bool changed = record.id != player.party[i].id;
        record.id = player.party[i].id;
        if (levelFromExp(record.exp) != player.party[i].level) {
            record.exp = levelExpThreshold(player.party[i].level);
            changed = true;
        }
        {
            const uint8_t mask = static_cast<uint8_t>(player.party[i].attackMask & 0x1F);
            if (record.attackMask != mask) changed = true;
            record.attackMask = mask;
        }
        // HP-only changes belong to the checkpoint snapshot. Avoid consuming
        // journal capacity before commit; the compaction pass mirrors HP into
        // the matching StoreRecord in the same checkpoint publication.
        if (changed) {
            record.hpEncoded = hp + 1;
            storeWrite(slot, record);
        }
    }
    plants.plantStages.serialize(saveState.plantStages);
    for (uint8_t i = 0; i < 2; ++i)
        saveState.plantPairs[i] = plants.plantPairs[i].serialize();
    saveState.plantTicker = plants.ticker;

    // The one Player bag is the snapshot authority. keyItems[1..3] stay the
    // canonical zero extension bytes from the memset above; only runtime key
    // byte 0 is written.
    memcpy(saveState.lures, &player.inventory, sizeof(player.inventory));
    saveState.keyItems[0] = player.keyItems.bits[0];
}

void drawSavingStatus(bool failed)
{
    static const uint8_t saving[6][5] PROGMEM = {
        {0x0f, 0x10, 0x0e, 0x01, 0x1e}, // S
        {0x0e, 0x11, 0x1f, 0x11, 0x11}, // A
        {0x11, 0x11, 0x11, 0x0a, 0x04}, // V
        {0x1f, 0x04, 0x04, 0x04, 0x1f}, // I
        {0x11, 0x19, 0x15, 0x13, 0x11}, // N
        {0x0e, 0x10, 0x17, 0x11, 0x0e}, // G
    };
    static const uint8_t failedText[6][5] PROGMEM = {
        {0x1f, 0x10, 0x1e, 0x10, 0x10}, // F
        {0x0e, 0x11, 0x1f, 0x11, 0x11}, // A
        {0x1f, 0x04, 0x04, 0x04, 0x1f}, // I
        {0x10, 0x10, 0x10, 0x10, 0x1f}, // L
        {0x1f, 0x10, 0x1e, 0x10, 0x1f}, // E
        {0x1e, 0x11, 0x11, 0x11, 0x1e}, // D
    };
    const uint8_t (*text)[5] = failed ? failedText : saving;

    // Keep using internal-flash glyphs here: the FX font shares SPI with the
    // save flash, but pgm_read_byte uses AVR LPM and does not access that bus.
    Blit::fillRect(0, 0, 128, 64, WHITE);
    for (uint8_t glyph = 0; glyph < 6; ++glyph) {
        for (uint8_t row = 0; row < 5; ++row) {
            const uint8_t rowBits = pgm_read_byte(&text[glyph][row]);
            for (uint8_t column = 0; column < 5; ++column) {
                if ((rowBits & (1 << (4 - column))) != 0) {
                    Blit::fillRect(34 + (glyph * 10) + (column * 2),
                                       27 + (row * 2), 2, 2, BLACK);
                }
            }
        }
    }
}

} // namespace

void begin(GameState_t returnState)
{
    // Mute and unregister the music timer before the snapshot mutates the
    // journal/store, so no audio FX fetch overlaps a save flash operation.
    audio::pause();
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

    const SaveStep step = saveStepAdvance(saveState);
    if (step == SaveStep::Done || step == SaveStep::Failed) {
        // Both terminal steps end the flash-busy window; failure keeps the
        // status screen, but music must not stay muted behind it.
        audio::resume();
    }
    if (step == SaveStep::Done) {
        savingStarted = false;
        gameState.state = stateBeforeSaving;
    }
}

void drawStatus()
{
    drawSavingStatus(!saveInProgress());
}

void seedParty()
{
    for (uint8_t i = 0; i < 3; ++i) {
        StoreRecord record = {};
        record.id = player.party[i].id;
        record.exp = levelExpThreshold(player.party[i].level);
        record.hpEncoded = player.creatureHPs[i] + 1;
        record.attackMask = static_cast<uint8_t>(player.party[i].attackMask & 0x1F);
        storeWrite(i, record);
        storeAssign(i, i, player.creatureHPs[i]);
    }
}
bool bootRecoveryFailed() { return storeBootFailed(); }
bool load()
{
    if (!saveFileLoad(saveState)) return false;
    gameState.playerLocation = saveState.playerLocation;
    memcpy(gameState.flags, saveState.flags, sizeof(FLAG_BIT_ARRAY));
    for (uint8_t i = 0; i < 3; ++i) {
        StoreRecord record;
        const uint8_t slot = saveState.partySlots[i];
        if (slot == 255 || !storeRead(slot, record)) {
            player.party[i].id = 255;
            player.creatureHPs[i] = 0;
            continue;
        }
        player.party[i].load(record);
        const uint8_t maxHP = player.party[i].hp;
        player.creatureHPs[i] = saveState.partyHP[i] > maxHP ? maxHP : saveState.partyHP[i];
    }
    plants.plantStages.restore(saveState.plantStages);
    for (uint8_t i = 0; i < 2; ++i)
        plants.plantPairs[i].restore(saveState.plantPairs[i]);
    plants.ticker = saveState.plantTicker;
    // Restore the single Player bag from the snapshot and re-point any future
    // battle item use at that same live bag.
    memcpy(&player.inventory, saveState.lures, sizeof(player.inventory));
    player.keyItems.bits[0] = saveState.keyItems[0];
    item::setBattleInventory(&player.inventory);
    return true;
}
} // namespace SaveController
