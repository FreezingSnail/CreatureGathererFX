#pragma once

#include <stdint.h>

namespace TileProps {

constexpr uint16_t TILE_ID_MASK = 0x03FF;
constexpr uint16_t TILE_PROP_MASK = 0xFC00;
constexpr uint8_t TILE_PROP_SHIFT = 10;

constexpr uint8_t PROP_WALKABLE = 1u << 0;
constexpr uint8_t PROP_WATER = 1u << 1;
constexpr uint8_t PROP_PLANTABLE = 1u << 2;
constexpr uint8_t PROP_LURE_BOX = 1u << 3;
constexpr uint8_t PROP_ENCOUNTER = 1u << 4;
constexpr uint8_t PROP_SPARE = 1u << 5;
constexpr uint8_t TILE_PROP_BITS = 0x3F;

// Callers that accept external IDs must validate them against TILE_ID_MASK
// before packing. Keeping the ID unmasked here makes a bad ID visible instead
// of silently wrapping it to an unrelated tile.
constexpr uint16_t packTile(uint16_t id, uint8_t properties) {
    return static_cast<uint16_t>(id |
                                 (static_cast<uint16_t>(properties & TILE_PROP_BITS)
                                  << TILE_PROP_SHIFT));
}

constexpr uint16_t tileId(uint16_t word) {
    return word & TILE_ID_MASK;
}

constexpr uint8_t tileProps(uint16_t word) {
    return static_cast<uint8_t>((word & TILE_PROP_MASK) >> TILE_PROP_SHIFT);
}

constexpr bool hasTileProp(uint16_t word, uint8_t property) {
    return (tileProps(word) & property) != 0;
}

constexpr bool isTileIdValid(uint16_t id) {
    return id <= TILE_ID_MASK;
}

} // namespace TileProps
