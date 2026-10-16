#pragma once

#include <stdint.h>

#include "../lib/FxRead.hpp"
#include "../fxdata.h"

// jp8.11.12: resolve the authored first-stage evolution mapping for one stable
// instance. Only the explicitly authored edges are consulted; species id
// adjacency is never inferred. The mapping is three parallel packed arrays
// (source, target, level) emitted contiguously in fxlayout declaration order, so
// the address delta between the first two is the authored edge count. That
// declaration order is a permanent ABI, and the native authoring tool rejects
// duplicate sources, self targets, missing targets and cycles before packing.
namespace CreatureEvolution {

constexpr uint8_t NO_TARGET = 0xFF;

constexpr uint8_t kEdgeCount =
    static_cast<uint8_t>(Evolution::evolutionTarget - Evolution::evolutionSource);

static_assert(kEdgeCount > 0, "first-beta evolution mapping must not be empty");

// Returns the explicit target species for `species` at its already-derived
// `level`, or NO_TARGET when the species has no authored edge or has not reached
// its authored threshold. The whole source column is read once, then the matched
// row's threshold and target; this runs only on a growth/collection transition,
// never per frame or per step.
inline uint8_t targetFor(uint8_t species, uint8_t level) {
    uint8_t sources[kEdgeCount];
    FxRead::bytes(Evolution::evolutionSource, sources, kEdgeCount);
    for (uint8_t i = 0; i < kEdgeCount; ++i) {
        if (sources[i] != species) continue;
        uint8_t threshold = 0;
        FxRead::bytes(Evolution::evolutionLevel + i, &threshold, 1);
        if (level < threshold) return NO_TARGET;
        uint8_t target = 0;
        FxRead::bytes(Evolution::evolutionTarget + i, &target, 1);
        return target;
    }
    return NO_TARGET;
}

} // namespace CreatureEvolution
