#pragma once
#include <iostream>
#include "test.hpp"
#include "../src/lib/Move.hpp"

void powerTest(TestSuite &t) {
    Test test = Test(__func__);
    Move move = Move({1, 2, 1});
    test.assert(int(move.getMovePower()), 2, "getMovePower");
    move = Move({1, 15, 1});
    test.assert(int(move.getMovePower()), 15, "getMovePower");
    t.addTest(test);
}

void moveTypeTest(TestSuite &t) {
    Test test = Test(__func__);
    Move move = Move({1, 2, 1});
    test.assert(move.getMoveType(), 1, "getMoveType");
    move = Move({2, 2, 1});
    test.assert(move.getMoveType(), 2, "getMoveType");
    move = Move({3, 2, 0});
    test.assert(move.getMoveType(), 3, "getMoveType");
    move = Move({4, 2, 1});
    test.assert(move.getMoveType(), 4, "getMoveType");
    move = Move({7, 2, 0});
    test.assert(move.getMoveType(), 7, "getMoveType");
    t.addTest(test);
}

void physicalTest(TestSuite &t) {
    Test test = Test(__func__);
    Move move = Move({1, 2, 1});
    test.assert(move.isPhysical(), true, "isPhysical");
    t.addTest(test);
}

void effectTest(TestSuite &t) {
    // TODO: rewrite
    //  Test test = Test(__func__);
    //  Move move = Move({1, 2, 1, uint8_t(Accuracy::HUNDRED), uint8_t(Effect::NONE)});
    //  test.assert((move.getMoveEffect()), int(Effect::NONE), "getMoveEffect");
    //  move = Move({1, 2, 1, uint8_t(Accuracy::HUNDRED), uint8_t(Effect::EGOED)});
    //  test.assert((move.getMoveEffect()), int(Effect::EGOED), "getMoveEffect");
    //  move = Move({1, 2, 1, uint8_t(Accuracy::HUNDRED), uint8_t(Effect::DRENCHED)});
    //  test.assert((move.getMoveEffect()), int(Effect::DRENCHED), "getMoveEffect");
    //  move = Move({1, 2, 1, uint8_t(Accuracy::HUNDRED), uint8_t(Effect::BUFFETED)});
    //  test.assert((move.getMoveEffect()), int(Effect::BUFFETED), "getMoveEffect");
    //  move = Move({1, 2, 1, uint8_t(Accuracy::HUNDRED), uint8_t(Effect::STUMBLED)});
    //  test.assert((move.getMoveEffect()), int(Effect::STUMBLED), "getMoveEffect");
    //  move = Move({1, 2, 1, uint8_t(Accuracy::HUNDRED), uint8_t(Effect::BURNED)});
    //  test.assert((move.getMoveEffect()), int(Effect::BURNED), "getMoveEffect");
    //  t.addTest(test);
}

void accuracyTest(TestSuite &t) {
    Test test = Test(__func__);
    Move move = Move({1, 2, 1, uint8_t(Accuracy::HUNDRED), uint8_t(Effect::NONE)});
    test.assert((move.getMoveAccuracy()), int(Accuracy::HUNDRED), "getMoveAccuracy");
    move = Move({1, 2, 1, uint8_t(Accuracy::NINETY), uint8_t(Effect::NONE)});
    test.assert((move.getMoveAccuracy()), int(Accuracy::NINETY), "getMoveAccuracy");
    move = Move({1, 2, 1, uint8_t(Accuracy::EIGHTY), uint8_t(Effect::NONE)});
    test.assert((move.getMoveAccuracy()), int(Accuracy::EIGHTY), "getMoveAccuracy");
    move = Move({1, 2, 1, uint8_t(Accuracy::SEVENTY), uint8_t(Effect::NONE)});
    test.assert((move.getMoveAccuracy()), int(Accuracy::SEVENTY), "getMoveAccuracy");
    t.addTest(test);
}

static void MoveSuite(TestRunner &r) {
    TestSuite t = TestSuite("Move Tests");
    moveTypeTest(t);
    powerTest(t);
    physicalTest(t);
    effectTest(t);
    accuracyTest(t);
    r.addTestSuite(t);
}