#include "Resolve.hpp"

namespace battle {
namespace {

constexpr uint8_t SIDE_COUNT = 2;
constexpr uint8_t PARTY_LIMIT = 3;

bool validSide(Side side)
{
    return static_cast<uint8_t>(side) < SIDE_COUNT;
}

uint8_t sideIndex(Side side)
{
    return static_cast<uint8_t>(side);
}

uint8_t benchIndexForOriginalSlot(const BattleState &state, Side side,
                                  uint8_t originalSlot)
{
    const uint8_t index = sideIndex(side);
    uint8_t bench = 0;
    for (uint8_t slot = 0; slot < state.partyCount[index] &&
         slot < PARTY_LIMIT; ++slot) {
        if (slot == state.activeSlot[index]) continue;
        if (slot == originalSlot) return bench;
        ++bench;
    }
    return PARTY_LIMIT;
}

} // namespace

bool sideDefeated(const BattleState &state, Side side)
{
    if (!validSide(side)) return true;
    const uint8_t index = sideIndex(side);
    const uint8_t count = state.partyCount[index];
    if (count == 0 || count > PARTY_LIMIT ||
        state.activeSlot[index] >= count) {
        return true;
    }
    if (state.active[index].hp != 0) return false;

    const uint8_t benchCount = static_cast<uint8_t>(count - 1);
    for (uint8_t slot = 0; slot < benchCount; ++slot) {
        if (state.bench[index][slot].hp != 0) return false;
    }
    return true;
}

bool canSwitch(const BattleState &state, Side side, uint8_t originalSlot)
{
    if (!validSide(side) || state.over) return false;
    const uint8_t index = sideIndex(side);
    const uint8_t count = state.partyCount[index];
    if (count < 2 || count > PARTY_LIMIT || originalSlot >= count ||
        state.activeSlot[index] >= count ||
        originalSlot == state.activeSlot[index]) {
        return false;
    }
    const uint8_t benchSlot = benchIndexForOriginalSlot(state, side, originalSlot);
    return benchSlot < static_cast<uint8_t>(count - 1) &&
           state.bench[index][benchSlot].hp != 0;
}

} // namespace battle
