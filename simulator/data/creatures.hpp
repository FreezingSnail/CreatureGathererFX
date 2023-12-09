#pragma once
#include <string>

typedef struct CreatureD {
    unsigned char id;
    unsigned char type1;
    unsigned char type2;
    unsigned char evoLevel;
    unsigned char atkSeed;
    unsigned char defSeed;
    unsigned char spcAtkSeed;
    unsigned char spcDefSeed;
    unsigned char hpSeed;
    unsigned char spdSeed;
    unsigned char move1;
    unsigned char move2;
    unsigned char move3;
    unsigned char move4;
} CreatureData_t;

namespace CreatureData {
static std::string creature0 = "SquibbleSnail";
static std::string creature1 = "SquableSnail";
static std::string creature2 = "ScrambleSnail";
static std::string creature3 = "SkitterCrab";
static std::string creature4 = "ScatterCrab";
static std::string creature5 = "ShatterCrab";
static std::string creature6 = "squid";
static std::string creature7 = "bigsquid";
static std::string creature8 = "BiggestSquid";
static std::string creature9 = "bell";
static std::string creature10 = "spark";
static std::string creature11 = "cuircuit";
static std::string creature12 = "hedge";
static std::string creature13 = "cloud";
static std::string creature14 = "rock";
static std::string creature15 = "wiggleworm";
static std::string creature16 = "waggleworm";
static std::string creature17 = "skimskate";
static std::string creature18 = "skimray";
static std::string creature19 = "billow";
static std::string creature20 = "howl";
static std::string creature21 = "item";
static std::string creature22 = "item2";
static std::string creature23 = "zip";
static std::string creature24 = "zap";
static std::string creature25 = "suculent";
static std::string creature26 = "cactus";
static std::string creature27 = "flickerfly";
static std::string creature28 = "flitfly";
static std::string creature29 = "dragon?";
static std::string creature30 = "skull";
static std::string creature31 = "ardu";

static CreatureData_t creatureData[] = {
    {0, 2, 8, 8, 3, 3, 2, 2, 3, 3, 8, 32, 32, 32},         {1, 2, 8, 16, 8, 8, 7, 7, 8, 8, 8, 9, 32, 32},
    {2, 2, 6, 24, 12, 12, 10, 10, 12, 12, 8, 9, 26, 10},   {3, 3, 8, 8, 2, 3, 2, 3, 4, 2, 13, 32, 32, 32},
    {4, 3, 8, 16, 6, 9, 7, 9, 9, 6, 13, 20, 32, 32},       {5, 3, 5, 24, 10, 13, 11, 13, 13, 8, 13, 20, 21, 12},
    {6, 1, 8, 8, 2, 2, 3, 3, 3, 3, 4, 32, 32, 32},         {7, 1, 8, 16, 7, 7, 8, 8, 8, 8, 4, 7, 32, 32},
    {8, 1, 4, 24, 10, 10, 12, 12, 12, 12, 4, 7, 17, 6},    {9, 0, 8, 0, 8, 8, 3, 3, 9, 6, 1, 32, 32, 32},
    {10, 4, 8, 0, 3, 3, 8, 8, 4, 9, 19, 32, 32, 32},       {11, 5, 8, 0, 3, 3, 8, 8, 4, 7, 20, 32, 32, 32},
    {12, 6, 8, 0, 8, 8, 3, 3, 6, 4, 8, 32, 32, 32},        {13, 2, 8, 0, 8, 8, 3, 3, 8, 8, 10, 23, 32, 32},
    {14, 3, 8, 0, 4, 10, 6, 12, 11, 2, 13, 2, 32, 32},     {15, 3, 8, 14, 2, 2, 7, 4, 4, 8, 13, 17, 32, 32},
    {16, 3, 4, 0, 4, 4, 10, 6, 6, 12, 12, 17, 9, 32},      {17, 1, 8, 14, 8, 3, 7, 8, 6, 11, 4, 24, 32, 32},
    {18, 1, 6, 0, 11, 4, 9, 12, 8, 13, 4, 24, 6, 27},      {19, 2, 8, 14, 10, 8, 7, 6, 8, 8, 11, 20, 32, 32},
    {20, 2, 5, 0, 13, 10, 9, 8, 10, 12, 11, 20, 23, 10},   {21, 0, 8, 14, 8, 8, 8, 8, 8, 8, 2, 28, 32, 32},
    {22, 0, 7, 0, 12, 13, 12, 12, 12, 10, 2, 28, 4, 30},   {23, 5, 8, 14, 8, 8, 8, 8, 8, 8, 23, 0, 32, 32},
    {24, 5, 1, 0, 12, 12, 13, 12, 10, 12, 23, 0, 22, 6},   {25, 6, 8, 14, 8, 8, 8, 8, 8, 8, 24, 28, 32, 32},
    {26, 6, 3, 0, 12, 12, 12, 13, 12, 10, 24, 28, 25, 12}, {27, 4, 8, 14, 8, 8, 8, 8, 8, 8, 16, 8, 32, 32},
    {28, 4, 2, 0, 8, 8, 12, 11, 8, 13, 16, 8, 17, 10},     {29, 7, 6, 0, 13, 13, 13, 13, 13, 12, 29, 10, 3, 25},
    {30, 7, 4, 0, 10, 10, 15, 14, 14, 14, 29, 17, 12, 0},  {31, 7, 5, 0, 15, 15, 10, 10, 14, 14, 31, 23, 15, 27},
};
}   // namespace CreatureData