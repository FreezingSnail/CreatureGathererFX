#pragma once

#include "../src/engine/battle/BattleFeedback.hpp"
#include "../src/engine/battle/BattleMenu.hpp"
#include "test.hpp"

namespace battle_slim_fixture {

// Shared compile-time ABI checks for the paired native and device fixtures.
static_assert(sizeof(battle::MoveSnapshot) == 9, "move snapshot contract");
static_assert(sizeof(battle::PartySnapshot) == 8, "party snapshot contract");
#ifdef __AVR__
static_assert(sizeof(battle::BattleMenu) == 27, "menu storage contract");
static_assert(sizeof(battle::BattleFeedback) == 10, "feedback storage contract");
#endif

inline void BattleSlimScaffoldTest(TestSuite &suite)
{
    Test test(__func__);
    test.assert(sizeof(battle::MoveSnapshot), static_cast<size_t>(9),
                "slim fixture sees the frozen move snapshot ABI");
    test.assert(sizeof(battle::PartySnapshot), static_cast<size_t>(8),
                "slim fixture sees the frozen party snapshot ABI");
    suite.addTest(test);
}

inline void BattleSlimScaffoldSuite(TestRunner &runner)
{
    TestSuite suite("Slim battle scaffold (behavior pending)");
    BattleSlimScaffoldTest(suite);
    runner.addTestSuite(suite);
}

} // namespace battle_slim_fixture
