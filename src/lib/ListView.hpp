#pragma once

#include <stdint.h>

// The cursor moves item by item; the window follows only when the cursor
// would leave it. Movement stops at either end and never wraps.
struct ListView {
    uint8_t itemCount;
    uint8_t rows;
    uint8_t windowStart;
    uint8_t cursor;
};

static_assert(sizeof(ListView) == 4, "ListView must remain a four-byte value");

void listViewMove(ListView &view, int8_t delta);
uint8_t listViewItemAt(const ListView &view, uint8_t row);
bool listViewVisible(const ListView &view, uint8_t row);
