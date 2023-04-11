#include "gtest/gtest.h"
#include "engine/battle/Battle.hpp"

TEST(BattleEngineTests, VictorTestFalse) {
    //arrange
    BattleEngine eng = BattleEngine();
    eng.loadAwakeMons(0b11100111);
    ASSERT_EQ (eng.checkWin(),false);
    eng.loadAwakeMons(0b11100101);
    ASSERT_EQ (eng.checkWin(),false);
    eng.loadAwakeMons(0b11100100);
    ASSERT_EQ (eng.checkWin(),false);
    eng.loadAwakeMons(0b11100010);
    ASSERT_EQ (eng.checkWin(),false);
    eng.loadAwakeMons(0b11100001);
    ASSERT_EQ (eng.checkWin(),false);
    eng.loadAwakeMons(0b11100011);
    ASSERT_EQ (eng.checkWin(),false);
    eng.loadAwakeMons(0b11100110);
    ASSERT_EQ (eng.checkWin(),false);
}

TEST(BattleEngineTests, VictorTestTrue) {
    //arrange
    BattleEngine eng = BattleEngine();
    eng.loadAwakeMons(0b11100000);
    ASSERT_EQ (eng.checkWin(),true);
}

TEST(BattleEngineTests, checkLossFalse) {
    //arrange
    BattleEngine eng = BattleEngine();
    eng.loadAwakeMons(0b11100111);
    ASSERT_EQ (eng.checkLoss(),false);
    eng.loadAwakeMons(0b00100111);
    ASSERT_EQ (eng.checkLoss(),false);
    eng.loadAwakeMons(0b01000111);
    ASSERT_EQ (eng.checkLoss(),false);
    eng.loadAwakeMons(0b10000000);
    ASSERT_EQ (eng.checkLoss(),false);
    eng.loadAwakeMons(0b01100100);
    ASSERT_EQ (eng.checkLoss(),false);
    eng.loadAwakeMons(0b10100010);
    ASSERT_EQ (eng.checkLoss(),false);
    eng.loadAwakeMons(0b11000001);
    ASSERT_EQ (eng.checkLoss(),false);
}

TEST(BattleEngineTests, checkLossTrue) {
    //arrange
    BattleEngine eng = BattleEngine();
    eng.loadAwakeMons(0b00000000);
    ASSERT_EQ (eng.checkLoss(),true);
}

TEST(BattleEngineTests, ApplyDamage) {
    //arrange
    BattleEngine eng = BattleEngine();
}