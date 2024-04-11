#pragma once
#include <iostream>
#include "test.hpp"
#include "../src/lib/Move.hpp"

void powerTest() {
    Test test;
    Move move = Move({1, 2, 1});
    test.assert(int(move.getMovePower()), 2, "getMovePower");
    move = Move({1, 15, 1});
    test.assert(int(move.getMovePower()), 15, "getMovePower");
    test.printSummary("powerTest");
}

void moveTypeTest() {
    Test test;
    Move move = Move({1, 2, 1});
    test.assert(int(move.getMoveType()), 1, "getMoveType");
    move = Move({2, 2, 1});
    test.assert(int(move.getMoveType()), 2, "getMoveType");
    move = Move({3, 2, 0});
    test.assert(int(move.getMoveType()), 3, "getMoveType");
    move = Move({4, 2, 1});
    test.assert(int(move.getMoveType()), 4, "getMoveType");
    move = Move({7, 2, 0});
    test.assert(int(move.getMoveType()), 7, "getMoveType");
    test.printSummary("typeTest");
}

void physicalTest() {
    Test test;
    Move move = Move({1, 2, 1});
    test.assert(move.isPhysical(), true, "isPhysical");
    test.printSummary("physicalTest");
}

void effectTest() {
    Test test;
    Move move = Move({1, 2, 1, uint8_t(Accuracy::HUNDRED), uint8_t(Effect::NONE)});
    test.assert(int(move.getMoveEffect()), int(Effect::NONE), "getMoveEffect");
    move = Move({1, 2, 1, uint8_t(Accuracy::HUNDRED), uint8_t(Effect::EGOED)});
    test.assert(int(move.getMoveEffect()), int(Effect::EGOED), "getMoveEffect");
    move = Move({1, 2, 1, uint8_t(Accuracy::HUNDRED), uint8_t(Effect::DRENCHED)});
    test.assert(int(move.getMoveEffect()), int(Effect::DRENCHED), "getMoveEffect");
    move = Move({1, 2, 1, uint8_t(Accuracy::HUNDRED), uint8_t(Effect::BUFFETED)});
    test.assert(int(move.getMoveEffect()), int(Effect::BUFFETED), "getMoveEffect");
    move = Move({1, 2, 1, uint8_t(Accuracy::HUNDRED), uint8_t(Effect::STUMBLED)});
    test.assert(int(move.getMoveEffect()), int(Effect::STUMBLED), "getMoveEffect");
    move = Move({1, 2, 1, uint8_t(Accuracy::HUNDRED), uint8_t(Effect::BURNED)});
    test.assert(int(move.getMoveEffect()), int(Effect::BURNED), "getMoveEffect");
    test.printSummary("effectTest");
}

void accuracyTest() {
    Test test;
    Move move = Move({1, 2, 1, uint8_t(Accuracy::HUNDRED), uint8_t(Effect::NONE)});
    test.assert(int(move.getMoveAccuracy()), int(Accuracy::HUNDRED), "getMoveAccuracy");
    move = Move({1, 2, 1, uint8_t(Accuracy::NINETY), uint8_t(Effect::NONE)});
    test.assert(int(move.getMoveAccuracy()), int(Accuracy::NINETY), "getMoveAccuracy");
    move = Move({1, 2, 1, uint8_t(Accuracy::EIGHTY), uint8_t(Effect::NONE)});
    test.assert(int(move.getMoveAccuracy()), int(Accuracy::EIGHTY), "getMoveAccuracy");
    move = Move({1, 2, 1, uint8_t(Accuracy::SEVENTY), uint8_t(Effect::NONE)});
    test.assert(int(move.getMoveAccuracy()), int(Accuracy::SEVENTY), "getMoveAccuracy");
    test.printSummary("accuracyTest");
}

static void MoveSuite() {
    printHeader("Move Tests");
    moveTypeTest();
    powerTest();
    physicalTest();
    effectTest();
    accuracyTest();
}