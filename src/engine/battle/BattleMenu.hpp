#pragma once

#include "BattleView.hpp"
#include "../../lib/uint24.h"
#include "../menu/MenuIntent.hpp"
#include <stdint.h>

#ifndef CGFX_BATTLE_AUTO_REPLACE
#define CGFX_BATTLE_AUTO_REPLACE 0
#endif
#if CGFX_BATTLE_AUTO_REPLACE != 0 && CGFX_BATTLE_AUTO_REPLACE != 1
#error CGFX_BATTLE_AUTO_REPLACE must be 0 or 1
#endif

namespace battle {

// Compact battle-only menu contract. Payload storage is reused between pages.
// Text addresses are prepared on page transitions; update/draw are RAM-only.
// Pages: options root (MOVE/GATHER/CHANGE/ESCAPE), four move slots, a living
// bench party page, and the forced replacement page in choice builds.
class BattleMenu {
  public:
    BattleMenu();
    bool initialized() const;
    // Battle entry: caches move names and opens the options page.
    void beginOptions(const MoveSnapshot &moves, bool allowGather, bool allowEscape);
    // CHANGE transition: caches up to two living bench rows for switching.
    void beginParty(const battle::BattleView &view);
    bool partyPending() const;
#if !CGFX_BATTLE_AUTO_REPLACE
    void beginReplacement(const ReplacementChoices &choices);
#endif
    MenuIntent update(uint8_t freshButtonEdges);
    void draw() const;

  private:
    struct MovePayload {
        MoveSnapshot moves;
        uint24_t names[4];
        uint8_t widths[4];
    };
    // Shared by the voluntary party page and forced replacement rows.
    struct PartyPayload {
        uint8_t slots[2];
        uint8_t count;
        uint24_t names[2];
        uint8_t widths[2];
    };
    union Payload {
        MovePayload moves;
        PartyPayload party;
    } payload_;
    uint8_t cursor_;
    uint8_t flags_;
};

#ifdef __AVR__
static_assert(sizeof(BattleMenu) == 27, "battle menu must remain 27 bytes");
#endif

} // namespace battle
