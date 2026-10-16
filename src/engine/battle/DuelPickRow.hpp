#pragma once
// Duel compact pick row (CreatureGathererFX-d73i.2). Selector/model for the
// footer pick row of mock/duel-bonus.html: one cell per set attack-mask bit
// (0..5), then SWAP when a living bench exists, then a WAIT cell when the mask
// has no usable attack. No PP/power cells. Layout + draw live in the duel
// renderer; this header owns the cursor model and its frame assertions.
#ifdef CGFX_DUEL_BATTLE

#include <stdint.h>

namespace duel {

constexpr uint8_t PICK_ROW_MAX_OPTIONS = 5;

struct PickRow {
    uint8_t options; // usable attack cells (0..5)
    uint8_t swap;    // 1 when a living-bench picker cell is offered
    uint8_t wait;    // 1 when the mask has no usable attack (WAIT cell)
    uint8_t cells;   // total selectable cells (>=1)
    uint8_t cursor;  // 0..cells-1
};

// Build the row. SWAP is suppressed with no living bench; WAIT appears only for
// an unusable kit (both SWAP and WAIT are present then, per the pinned rules).
inline void pickRowInit(PickRow &row, uint8_t options, bool livingBench)
{
    row.options = options > PICK_ROW_MAX_OPTIONS ? PICK_ROW_MAX_OPTIONS : options;
    row.swap = livingBench ? 1 : 0;
    row.wait = row.options == 0 ? 1 : 0;
    row.cells = static_cast<uint8_t>(row.options + row.swap + row.wait);
    if (row.cells == 0) row.cells = 1; // defensive only; a cell always exists
    row.cursor = 0;
}

inline void pickRowMove(PickRow &row, int8_t direction)
{
    if (row.cells == 0 || direction == 0) return;
    int16_t next = static_cast<int16_t>(row.cursor) + direction;
    if (next < 0) next = row.cells - 1;
    if (next >= row.cells) next = 0;
    row.cursor = static_cast<uint8_t>(next);
}

// Cell identity: index < options is an attack option, then SWAP, then WAIT.
inline bool pickRowIsAttack(const PickRow &row)
{
    return row.cursor < row.options;
}

inline bool pickRowIsSwap(const PickRow &row)
{
    return row.swap != 0 && row.cursor == row.options;
}

inline bool pickRowIsWait(const PickRow &row)
{
    return row.wait != 0 && row.cursor == static_cast<uint8_t>(row.options + row.swap);
}

#ifdef __AVR__
static_assert(sizeof(PickRow) == 5, "pick row model stays five bytes");
#endif

} // namespace duel
#endif
