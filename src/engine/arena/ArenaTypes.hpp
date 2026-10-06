#pragma once

#include <stdint.h>

#include "../../lib/ListView.hpp"
#include "../../lib/uint24.h"
#include "../battle/ActionResult.hpp"

namespace arena {

enum class ArenaScreen : uint8_t { PlayerTeam, OpponentTeam, Result };
enum class ArenaIntent : uint8_t { None, PreviewChanged, StartBattle };

struct ArenaContext {
    uint8_t playerTeam;
    uint8_t opponentTeam;
};

struct ArenaPreview {
    uint24_t labelAddress;
    uint8_t labelWidth;
    uint8_t species[3];
    uint24_t nameAddress[3];
    uint8_t nameWidth[3];
};

struct ArenaUiState {
    ArenaScreen screen;
    ListView list;
    ArenaPreview preview;
};

static_assert(sizeof(ArenaContext) == 2, "arena selection context must remain two bytes");
#ifdef __AVR__
static_assert(sizeof(ArenaPreview) == 19, "arena preview must remain 19 bytes");
static_assert(sizeof(ArenaUiState) == 24, "arena UI state must remain 24 bytes");
static_assert(alignof(ArenaUiState) == 1, "arena UI state must remain byte aligned");
#endif

} // namespace arena
