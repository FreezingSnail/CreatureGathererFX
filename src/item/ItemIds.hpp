#pragma once

#include <stdint.h>

// Lure ids are type * 3 + tier, so type and tier are derived rather than
// stored; charge and rate are arithmetic, which is why lures have no FX table.
namespace item {

enum class ItemKind : uint8_t { Lure = 0, Consumable = 1, Key = 2 };

constexpr uint8_t LURE_TYPE_COUNT = 8;
constexpr uint8_t LURE_TIER_COUNT = 3;
constexpr uint8_t LURE_COUNT = LURE_TYPE_COUNT * LURE_TIER_COUNT;
constexpr uint8_t CONSUMABLE_COUNT = 8;
constexpr uint8_t KEY_ITEM_COUNT = 8;
constexpr uint8_t COUNT_MAX = 99;

constexpr uint8_t lureId(uint8_t type, uint8_t tier) {
    return type * LURE_TIER_COUNT + tier;
}

constexpr uint8_t lureTypeOf(uint8_t id) { return id / LURE_TIER_COUNT; }
constexpr uint8_t lureTierOf(uint8_t id) { return id % LURE_TIER_COUNT; }
constexpr uint8_t lureChargeOf(uint8_t tier) { return 16 << tier; }
constexpr uint8_t lureRateOf(uint8_t tier) { return 1 << tier; }

static_assert(LURE_COUNT == 24, "lure id space must remain 8 types by 3 tiers");
static_assert(lureChargeOf(2) == 64, "tier 2 lure charge must fit in uint8_t");

}  // namespace item
