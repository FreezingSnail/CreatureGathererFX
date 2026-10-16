#pragma once

#include "BattleView.hpp"
#include "../../lib/uint24.h"
#include "../menu/MenuIntent.hpp"
#include <stdint.h>
#ifdef CGFX_DUEL_BATTLE
#include "BattleState.hpp"
#endif

#ifndef CGFX_BATTLE_AUTO_REPLACE
#define CGFX_BATTLE_AUTO_REPLACE 0
#endif
#if CGFX_BATTLE_AUTO_REPLACE != 0 && CGFX_BATTLE_AUTO_REPLACE != 1
#error "CGFX_BATTLE_AUTO_REPLACE must be 0 or 1"
#endif

namespace battle {

// Compact battle-only menu contract. Payload storage is reused between pages.
// Text addresses are prepared on page transitions; update/draw are RAM-only.
// Pages: options root (MOVE/GATHER/CHANGE/ESCAPE), four move slots, a living
// bench party page, and the forced replacement page in choice builds.
//
// Under CGFX_DUEL_BATTLE the same 27-byte object drives the compact duel pages:
// a single pick row (attack cells + SWAP + WAIT) whose geometry the duel scene
// draws from resident state, a two-row living-bench picker, and a wild-only
// gather/escape action page. The legacy move/options pages are compiled out.
class BattleMenu {
  public:
    BattleMenu();
    bool initialized() const;

#ifndef CGFX_DUEL_BATTLE
    // Battle entry: caches move names and opens the options page.
    void beginOptions(const MoveSnapshot &moves, bool allowGather, bool allowEscape);
#if !CGFX_BATTLE_AUTO_REPLACE
    // CHANGE transition: caches up to two living bench rows for switching.
    void beginReplacement(const ReplacementChoices &choices);
#endif
#else
    // Duel entry: builds the pick row from resident state (cells render as type
    // icons, so no FX name reads) and opens the pick page.
    void beginDuel(const BattleState &state, bool allowGather, bool allowEscape);
    // Pick-cell cursor for the scene draw; -1 when the pick row is not shown.
    int8_t duelCursor() const;
#endif

    // CHANGE transition: caches up to two living bench rows for switching.
    void beginParty(const battle::BattleView &view);
    bool partyPending() const;
    MenuIntent update(uint8_t freshButtonEdges);
    void draw() const;

  private:
#ifndef CGFX_DUEL_BATTLE
    struct MovePayload {
        MoveSnapshot moves;
        uint24_t names[4];
        uint8_t widths[4];
    };
#endif
    // Shared by the voluntary party page and forced replacement rows.
    struct PartyPayload {
        uint8_t slots[2];
        uint8_t count;
        uint24_t names[2];
        uint8_t widths[2];
    };
#ifdef CGFX_DUEL_BATTLE
    // The duel pick row and bench picker share one payload so the cell model
    // survives a trip through the party page (options/slots stay intact).
    struct DuelPayload {
        uint8_t options;    // usable attack cells (0..5)
        uint8_t slots[5];   // ascending option index per attack cell
        uint8_t swap;       // 1 when the SWAP cell is offered
        uint8_t wait;       // 1 when the WAIT cell is offered
        PartyPayload party;
        // Pad to the fixed legacy menu payload so BattleUi stays 27 bytes (the
        // ModeState union ABI assert); ModeState is sized by WorldTransient.
        uint8_t reserved[6];
    };
    DuelPayload payload_;
#else
    union Payload {
        MovePayload moves;
        PartyPayload party;
    } payload_;
#endif
    uint8_t cursor_;
    uint8_t flags_;
};

#ifdef __AVR__
static_assert(sizeof(BattleMenu) == 27, "battle menu must remain 27 bytes");
#endif

} // namespace battle
