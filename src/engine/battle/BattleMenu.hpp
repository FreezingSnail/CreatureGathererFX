#pragma once

#include "BattleView.hpp"
#include "../../lib/uint24.h"
#include "../menu/MenuIntent.hpp"
#include <stdint.h>

#ifndef CGFX_BATTLE_AUTO_REPLACE
#define CGFX_BATTLE_AUTO_REPLACE 0
#endif
#if CGFX_BATTLE_AUTO_REPLACE != 0 && CGFX_BATTLE_AUTO_REPLACE != 1
#error "CGFX_BATTLE_AUTO_REPLACE must be 0 or 1"
#endif

namespace battle {

// Compact battle-only menu contract. Payload storage is reused between pages.
// Text addresses are prepared on page transitions; update/draw are RAM-only.
class BattleMenu {
  public:
    BattleMenu();
    bool initialized() const;
    void beginMoves(const MoveSnapshot &moves, bool allowGather, bool allowEscape);
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
#if !CGFX_BATTLE_AUTO_REPLACE
    struct ReplacementPayload {
        uint8_t slots[2];
        uint8_t count;
        uint24_t names[2];
        uint8_t widths[2];
    };
#endif
    union Payload {
        MovePayload moves;
#if !CGFX_BATTLE_AUTO_REPLACE
        ReplacementPayload replacement;
#endif
    } payload_;
    uint8_t cursor_;
    uint8_t flags_;
};

#ifdef __AVR__
static_assert(sizeof(BattleMenu) == 27, "battle menu must remain 27 bytes");
#endif

} // namespace battle
