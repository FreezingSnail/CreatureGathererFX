#include "gtest/gtest.h"
#include "lib/Move.hpp"
#include "lib/Moves.hpp"

//move 
// 00    000    000    
// id#   type   power 

TEST(MoveTests, getMoveID) {
    ASSERT_EQ (getMoveID(0b00000111),  0);
    ASSERT_EQ (getMoveID(0b01000111),  1);
    ASSERT_EQ (getMoveID(0b10000111),  2);
    ASSERT_EQ (getMoveID(0b11000111),  3);
    ASSERT_EQ (getMoveID(0b00001111),  4);
    ASSERT_EQ (getMoveID(0b01001111),  5);
    ASSERT_EQ (getMoveID(0b10001111),  6);
    ASSERT_EQ (getMoveID(0b11001111),  7);
    ASSERT_EQ (getMoveID(0b00010111),  8);
    ASSERT_EQ (getMoveID(0b01010111),  9);
    ASSERT_EQ (getMoveID(0b10010111),  10);
    ASSERT_EQ (getMoveID(0b11010111),  11);



}

TEST(MoveTests, getMovePower) {

    ASSERT_EQ (getMovePower(0b00000111),  0b00000111);
    ASSERT_EQ (getMovePower(0b10000111),  0b00000111);
    ASSERT_EQ (getMovePower(0b00100111),  0b00000111);
    ASSERT_EQ (getMovePower(0b11111111),  0b00000111);

    ASSERT_EQ (getMovePower(0b00000001),  0b00000001);
    ASSERT_EQ (getMovePower(0b10000001),  0b00000001);
    ASSERT_EQ (getMovePower(0b00100001),  0b00000001);
    ASSERT_EQ (getMovePower(0b11111001),  0b00000001);

}

TEST(MoveTests, getMoveType) {

    ASSERT_EQ (getMoveType(0b00000111),  0b00000000);
    ASSERT_EQ (getMoveType(0b10001111),  0b00000001);
    ASSERT_EQ (getMoveType(0b10010111),  0b00000010);
    ASSERT_EQ (getMoveType(0b00100111),  0b00000100);
    ASSERT_EQ (getMoveType(0b11111111),  0b00000111);

}