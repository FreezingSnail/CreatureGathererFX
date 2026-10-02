#pragma once

#include "ItemIds.hpp"
#include "../lib/uint24.h"

namespace item {

struct ItemName {
    uint24_t part[2];
    uint8_t parts;
};

// Resolves FX index tables on menu open and scroll, never in a draw loop.
// Lures have tier then type; consumables have one part. Invalid/unsupported
// kinds and ids return zero parts.
ItemName itemNameAddr(ItemKind kind, uint8_t id);

} // namespace item
