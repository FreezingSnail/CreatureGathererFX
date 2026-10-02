#pragma once

#include "ItemIds.hpp"

namespace item {

enum class ConsumableKind : uint8_t { None = 0, Heal = 1, Cure = 2, Charge = 3 };

struct ConsumableDef {
    uint8_t kind;
    uint8_t arg;
};

static_assert(sizeof(ConsumableDef) == 2, "ConsumableDef is a 2-byte FX record");

// Reads one record when the item is used or its detail is opened.
ConsumableDef readConsumableDef(uint8_t id);

}  // namespace item
