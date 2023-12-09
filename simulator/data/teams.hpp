#pragma once
#include <stdint.h>
#include <string>

struct CreatureMoveset {
    uint8_t id;
    uint8_t m1;
    uint8_t m2;
    uint8_t m3;
    uint8_t m4;
};

struct OppTeam {
    CreatureMoveset a;
    CreatureMoveset b;
    CreatureMoveset c;
    uint8_t l1;
    uint8_t l2;
    uint8_t l3;
    std::string name;
};

namespace Teams {
namespace StockCreatures {
static CreatureMoveset null = {0, 0, 0, 0, 0};
static CreatureMoveset SquibbleSnail = {0, 1, 4, 10, 9};
static CreatureMoveset SquableSnail = {1, 1, 5, 10, 27};
static CreatureMoveset ScrambleSnail = {2, 5, 10, 19, 26};
static CreatureMoveset SkitterCrab = {3, 1, 11, 12, 15};
static CreatureMoveset ScatterCrab = {4, 1, 11, 12, 20};
static CreatureMoveset ShatterCrab = {5, 0, 12, 21, 24};
static CreatureMoveset squid = {6, 1, 13, 6, 7};
static CreatureMoveset bigsquid = {7, 6, 7, 16, 14};
static CreatureMoveset BiggestSquid = {8, 16, 6, 15, 23};
static CreatureMoveset bell = {9, 3, 10, 14, 23};
static CreatureMoveset ember = {10, 9, 12, 17, 21};
static CreatureMoveset cuircuit = {11, 12, 17, 21, 2};
static CreatureMoveset hedge = {12, 26, 18, 14, 10};
static CreatureMoveset cloud = {13, 10, 7, 23, 26};
static CreatureMoveset rock = {14, 28, 16, 15, 2};
static CreatureMoveset wiggleworm = {15, 17, 12, 24, 2};
static CreatureMoveset waggleworm = {16, 17, 25, 12, 2};
static CreatureMoveset skimskate = {17, 6, 11, 24, 3};
static CreatureMoveset skimray = {18, 0, 6, 10, 26};
static CreatureMoveset billow = {19, 2, 10, 20, 24};
static CreatureMoveset howl = {20, 24, 22, 10, 15};
static CreatureMoveset item = {21, 3, 6, 23, 25};
static CreatureMoveset item2 = {22, 29, 25, 23, 18};
static CreatureMoveset zip = {23, 7, 11, 21, 1};
static CreatureMoveset zap = {24, 3, 6, 25, 22};
static CreatureMoveset suculent = {25, 26, 12, 1, 14};
static CreatureMoveset cactus = {26, 25, 20, 15, 3};
static CreatureMoveset flickerfly = {27, 17, 8, 11, 1};
static CreatureMoveset flitfly = {28, 26, 23, 17, 5};
static CreatureMoveset dragon = {29, 31, 29, 25, 6};
static CreatureMoveset skull = {30, 29, 17, 0, 12};
static CreatureMoveset ardu = {31, 3, 31, 22, 10};
}   // namespace StockCreatures

static std::string jimbo = "jimbo";

static OppTeam teamList[] = {
    {StockCreatures::SquibbleSnail, StockCreatures::SkitterCrab, StockCreatures::squid, 32, 32, 32, jimbo},
    {StockCreatures::SquableSnail, StockCreatures::ScatterCrab, StockCreatures::bigsquid, 32, 32, 32, jimbo},
    {StockCreatures::ScrambleSnail, StockCreatures::ShatterCrab, StockCreatures::BiggestSquid, 32, 32, 32, jimbo},
    {StockCreatures::SquibbleSnail, StockCreatures::SquableSnail, StockCreatures::ScrambleSnail, 32, 32, 32, jimbo},
    {StockCreatures::SkitterCrab, StockCreatures::ScatterCrab, StockCreatures::ShatterCrab, 32, 32, 32, jimbo},
    {StockCreatures::squid, StockCreatures::bigsquid, StockCreatures::BiggestSquid, 32, 32, 32, jimbo},
    {StockCreatures::cuircuit, StockCreatures::zip, StockCreatures::zap, 32, 32, 32, jimbo},
    {StockCreatures::ember, StockCreatures::flickerfly, StockCreatures::flitfly, 32, 32, 32, jimbo},
    {StockCreatures::hedge, StockCreatures::suculent, StockCreatures::cactus, 32, 32, 32, jimbo},
    {StockCreatures::cloud, StockCreatures::billow, StockCreatures::howl, 32, 32, 32, jimbo},
    {StockCreatures::rock, StockCreatures::wiggleworm, StockCreatures::waggleworm, 32, 32, 32, jimbo},
    {StockCreatures::waggleworm, StockCreatures::skimray, StockCreatures::howl, 32, 32, 32, jimbo},
    {StockCreatures::zap, StockCreatures::cactus, StockCreatures::flitfly, 32, 32, 32, jimbo},
    {StockCreatures::BiggestSquid, StockCreatures::skimray, StockCreatures::zap, 32, 32, 32, jimbo},
    {StockCreatures::ShatterCrab, StockCreatures::waggleworm, StockCreatures::cactus, 32, 32, 32, jimbo},
    {StockCreatures::ScrambleSnail, StockCreatures::howl, StockCreatures::flitfly, 32, 32, 32, jimbo},
    {StockCreatures::bell, StockCreatures::item, StockCreatures::item2, 32, 32, 32, jimbo},
    {StockCreatures::skimray, StockCreatures::skimray, StockCreatures::skimray, 32, 32, 32, jimbo},
    {StockCreatures::dragon, StockCreatures::skull, StockCreatures::ardu, 32, 32, 32, jimbo},
};

}   // namespace Teams
