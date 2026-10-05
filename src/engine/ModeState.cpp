#include "ModeState.hpp"

#include "../lib/random.hpp"
#include "world/World.hpp"

namespace {

uint8_t battleRandomRoll(uint8_t bound)
{
    return bound == 0 ? 0 : randomRoll(0, static_cast<uint8_t>(bound - 1));
}

} // namespace

void ModeState::enterBattle() {
    // BattleSession is trivially destructible; placement construction starts a
    // fresh active member without reading or clearing inactive world data.
    ::new (static_cast<void *>(&battle)) battle::BattleMode();
    battle.setRng({battleRandomRoll});
}

void ModeState::exitBattle() {
    // Value initialization clears the world buffers and makes the script slot
    // active. WorldEngine then activates its compact movement overlay.
    ::new (static_cast<void *>(&world)) WorldTransient{};
    WorldEngine::init(world);
    WorldEngine::loadMap(world, 0, 0);
    WorldEngine::syncFromLocation(world);
}
