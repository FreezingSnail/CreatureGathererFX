#pragma once
#include <stdint.h>
#include <string>
struct MoveBitSet {
    uint8_t type;
    uint8_t power;
    uint8_t physical;
};

namespace MoveData {
static std::string move0 = "smite";
static std::string move1 = "thought";
static std::string move2 = "wisper";
static std::string move3 = "blast";
static std::string move4 = "splash";
static std::string move5 = "squirt";
static std::string move6 = "soak";
static std::string move7 = "current";
static std::string move8 = "breeze";
static std::string move9 = "bellow";
static std::string move10 = "torrent";
static std::string move11 = "blow";
static std::string move12 = "dirtbeam";
static std::string move13 = "slipfall";
static std::string move14 = "dirtfall";
static std::string move15 = "shatter";
static std::string move16 = "burn";
static std::string move17 = "melt";
static std::string move18 = "torch";
static std::string move19 = "stoke";
static std::string move20 = "bolt";
static std::string move21 = "plasma";
static std::string move22 = "coil";
static std::string move23 = "zap";
static std::string move24 = "pollen";
static std::string move25 = "pollenbeam";
static std::string move26 = "seedfall";
static std::string move27 = "root";
static std::string move28 = "beam";
static std::string move29 = "elderBeam";
static std::string move30 = "slam";
static std::string move31 = "elderSlam";
static std::string move32 = "_";

static std::string moveNames[] = {move0,  move1,  move2,  move3,  move4,  move5,  move6,  move7,  move8,  move9,  move10,
                                  move11, move12, move13, move14, move15, move16, move17, move18, move19, move20, move21,
                                  move22, move23, move24, move25, move26, move27, move28, move29, move30, move31, move32};

static MoveBitSet movePack[] = {
    {0, 10, 0}, {0, 5, 1}, {0, 7, 1}, {0, 10, 1}, {1, 4, 0},  {1, 7, 0}, {1, 10, 0}, {1, 7, 1},  {2, 5, 0},  {2, 10, 0}, {2, 10, 1},
    {2, 7, 1},  {3, 7, 0}, {3, 5, 1}, {3, 7, 1},  {3, 10, 1}, {4, 8, 0}, {4, 12, 0}, {4, 12, 1}, {4, 6, 1},  {5, 7, 0},  {5, 12, 0},
    {5, 12, 1}, {5, 7, 1}, {6, 6, 0}, {6, 12, 0}, {6, 12, 1}, {6, 8, 1}, {7, 8, 0},  {7, 12, 0}, {7, 10, 1}, {7, 14, 1},
};
}   // namespace MoveData