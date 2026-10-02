#pragma once
#include "test.hpp"

#include "../src/GameState.hpp"
#include "../src/macros.hpp"
#include "../src/flags/flag_bit_array.hpp"

void GameStateFlagTest(TestSuite &t) {
    Test test = Test(__func__);
    GameState gs = GameState();
    for (uint8_t i = 0; i < sizeof(FLAG_BIT_ARRAY); ++i) {
        FLAG_BIT_ARRAY[i] = 0;
    }
    const uint16_t lastFlag = sizeof(FLAG_BIT_ARRAY) * 8 - 1;

    // Test initial state - flags should be cleared
    test.assert(gs.getFlag(0), false, "Flag 0 initially false");
    test.assert(gs.getFlag(1), false, "Flag 1 initially false");
    test.assert(gs.getFlag(7), false, "Flag 7 initially false");
    test.assert(gs.getFlag(lastFlag), false, "last allocated flag initially false");

    // Test setting flags
    gs.setFlag(0);
    test.assert(gs.getFlag(0), true, "Flag 0 set to true");
    test.assert(gs.getFlag(1), false, "Flag 1 still false after setting flag 0");

    gs.setFlag(7);
    test.assert(gs.getFlag(7), true, "Flag 7 set to true");
    test.assert(gs.getFlag(0), true, "Flag 0 still true after setting flag 7");

    gs.setFlag(lastFlag);
    test.assert(gs.getFlag(lastFlag), true, "last allocated flag set to true");

    // Test clearing flags
    gs.clearFlag(0);
    test.assert(gs.getFlag(0), false, "Flag 0 cleared to false");
    test.assert(gs.getFlag(7), true, "Flag 7 still true after clearing flag 0");

    gs.clearFlag(7);
    test.assert(gs.getFlag(7), false, "Flag 7 cleared to false");
    if (lastFlag != 7) {
        test.assert(gs.getFlag(lastFlag), true,
                    "last allocated flag still true after clearing flag 7");
    }
    gs.clearFlag(lastFlag);
    test.assert(gs.getFlag(lastFlag), false, "last allocated flag cleared to false");

    t.addTest(test);
}

void GameStateBitManipulationTest(TestSuite &t) {
    Test test = Test(__func__);
    GameState gs = GameState();

    // Test setting multiple flags in same byte
    gs.setFlag(0);
    gs.setFlag(1);
    gs.setFlag(2);
    gs.setFlag(7);

    test.assert(gs.getFlag(0), true, "Flag 0 set in multi-flag byte");
    test.assert(gs.getFlag(1), true, "Flag 1 set in multi-flag byte");
    test.assert(gs.getFlag(2), true, "Flag 2 set in multi-flag byte");
    test.assert(gs.getFlag(3), false, "Flag 3 not set in multi-flag byte");
    test.assert(gs.getFlag(7), true, "Flag 7 set in multi-flag byte");

    // Clear one flag and verify others remain
    gs.clearFlag(1);
    test.assert(gs.getFlag(0), true, "Flag 0 still set after clearing flag 1");
    test.assert(gs.getFlag(1), false, "Flag 1 cleared");
    test.assert(gs.getFlag(2), true, "Flag 2 still set after clearing flag 1");
    test.assert(gs.getFlag(7), true, "Flag 7 still set after clearing flag 1");

    // Test setting and clearing same flag multiple times
    const uint16_t validFlag = sizeof(FLAG_BIT_ARRAY) * 8 > 50 ? 50 : 6;
    gs.setFlag(validFlag);
    test.assert(gs.getFlag(validFlag), true, "valid flag set first time");
    gs.clearFlag(validFlag);
    test.assert(gs.getFlag(validFlag), false, "valid flag cleared first time");
    gs.setFlag(validFlag);
    test.assert(gs.getFlag(validFlag), true, "valid flag set second time");
    gs.setFlag(validFlag);   // Set again - should remain true
    test.assert(gs.getFlag(validFlag), true, "valid flag still true after double set");

    t.addTest(test);
}

void GameControlFlagTest(TestSuite &t) {
    Test test = Test(__func__);
    GameState gs = GameState();
    test.assert(IS_SET_FLAG(WALKINGFLAG, gs.gameControlFlags), 0, "walking flag shouldn't be set");
    SET_FLAG(WALKINGFLAG, gs.gameControlFlags);
    test.assert(IS_SET_FLAG(WALKINGFLAG, gs.gameControlFlags), 1, "walking flag should be set");
    CLEAR_FLAG(WALKINGFLAG, gs.gameControlFlags);
    test.assert(IS_SET_FLAG(WALKINGFLAG, gs.gameControlFlags), 0, "walking flag shouldn't be set");
}

void GameStateSuite(TestRunner &r) {
    TestSuite t = TestSuite("GameState Suite");
    GameStateFlagTest(t);
    GameStateBitManipulationTest(t);
    GameControlFlagTest(t);
    r.addTestSuite(t);
}
