#include "ModeState.hpp"

#include "world/World.hpp"

void ModeState::enterBattle() {
    // BattleSession is trivially destructible; placement construction starts a
    // fresh active member without reading or clearing inactive world data.
    ::new (static_cast<void *>(&battle)) battle::BattleMode();
}

void ModeState::exitBattle() {
    // Value initialization clears the world buffers and makes the script slot
    // active. WorldEngine then activates its compact movement overlay.
    ::new (static_cast<void *>(&world)) WorldTransient{};
    WorldEngine::init(world);
    WorldEngine::loadMap(world, 0, 0);
    WorldEngine::syncFromLocation(world);
}
