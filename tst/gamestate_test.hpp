#pragma once
#include "test.hpp"

#include "../src/GameState.hpp"

void GameStateFlagTest(TestSuite &t) {
    Test test = Test(__func__);
    GameState gs = GameState();

    // Test initial state - flags should be cleared
    test.assert(gs.getFlag(0), false, "Flag 0 initially false");
    test.assert(gs.getFlag(1), false, "Flag 1 initially false");
    test.assert(gs.getFlag(7), false, "Flag 7 initially false");
    test.assert(gs.getFlag(8), false, "Flag 8 initially false");
    test.assert(gs.getFlag(15), false, "Flag 15 initially false");
    test.assert(gs.getFlag(100), false, "Flag 100 initially false");

    // Test setting flags
    gs.setFlag(0);
    test.assert(gs.getFlag(0), true, "Flag 0 set to true");
    test.assert(gs.getFlag(1), false, "Flag 1 still false after setting flag 0");

    gs.setFlag(7);
    test.assert(gs.getFlag(7), true, "Flag 7 set to true");
    test.assert(gs.getFlag(0), true, "Flag 0 still true after setting flag 7");

    // Test setting flag in different byte
    gs.setFlag(8);
    test.assert(gs.getFlag(8), true, "Flag 8 set to true");
    test.assert(gs.getFlag(7), true, "Flag 7 still true after setting flag 8");

    gs.setFlag(15);
    test.assert(gs.getFlag(15), true, "Flag 15 set to true");

    gs.setFlag(100);
    test.assert(gs.getFlag(100), true, "Flag 100 set to true");

    // Test clearing flags
    gs.clearFlag(0);
    test.assert(gs.getFlag(0), false, "Flag 0 cleared to false");
    test.assert(gs.getFlag(7), true, "Flag 7 still true after clearing flag 0");

    gs.clearFlag(7);
    test.assert(gs.getFlag(7), false, "Flag 7 cleared to false");
    test.assert(gs.getFlag(8), true, "Flag 8 still true after clearing flag 7");

    gs.clearFlag(8);
    test.assert(gs.getFlag(8), false, "Flag 8 cleared to false");
    test.assert(gs.getFlag(15), true, "Flag 15 still true after clearing flag 8");

    gs.clearFlag(15);
    test.assert(gs.getFlag(15), false, "Flag 15 cleared to false");

    gs.clearFlag(100);
    test.assert(gs.getFlag(100), false, "Flag 100 cleared to false");

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
    gs.setFlag(50);
    test.assert(gs.getFlag(50), true, "Flag 50 set first time");
    gs.clearFlag(50);
    test.assert(gs.getFlag(50), false, "Flag 50 cleared first time");
    gs.setFlag(50);
    test.assert(gs.getFlag(50), true, "Flag 50 set second time");
    gs.setFlag(50);   // Set again - should remain true
    test.assert(gs.getFlag(50), true, "Flag 50 still true after double set");

    t.addTest(test);
}

void GameStateSuite(TestRunner &r) {
    TestSuite t = TestSuite("GameState Suite");
    GameStateFlagTest(t);
    GameStateBitManipulationTest(t);
    r.addTestSuite(t);
}