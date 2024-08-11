#pragma once

#include <unordered_map>
#include <string>

std::unordered_map<std::string, int> creature_string_to_number = {
    {"none", 255},      {"squibblesnail", 0}, {"squablesnail", 1}, {"scramblesnail", 2}, {"skittercrab", 3}, {"scattercrab", 4},
    {"shattercrab", 5}, {"squid", 6},         {"bigsquid", 7},     {"biggestsquid", 8},  {"bell", 9},        {"ember", 10},
    {"cuircuit", 11},   {"hedge", 12},        {"cloud", 13},       {"rock", 14},         {"wiggleworm", 15}, {"waggleworm", 16},
    {"skimskate", 17},  {"skimray", 18},      {"billow", 19},      {"howl", 20},         {"item", 21},       {"item2", 22},
    {"zip", 23},        {"zap", 24},          {"suculent", 25},    {"cactus", 26},       {"flickerfly", 27}, {"flitfly", 28},
    {"dragon", 29},     {"skull", 30},        {"ardu", 31}};

std::unordered_map<std::string, int> move_string_to_number = {
    {"none", 255},    {"smite", 0},     {"thought", 1},     {"wisper", 2}, {"blast", 3},     {"splash", 4},       {"squirt", 5},
    {"soak", 6},      {"current", 7},   {"breeze", 8},      {"bellow", 9}, {"torrent", 10},  {"blow", 11},        {"dirtburst", 12},
    {"slipfall", 13}, {"dirtfall", 14}, {"shatter", 15},    {"burn", 16},  {"melt", 17},     {"torch", 18},       {"stoke", 19},
    {"bolt", 20},     {"plasma", 21},   {"coil", 22},       {"zap", 23},   {"pollen", 24},   {"pollenburst", 25}, {"seedfall", 26},
    {"root", 27},     {"burst", 28},    {"elderBurst", 29}, {"slam", 30},  {"elderSlam", 31}};

std::unordered_map<int, std::string> creature_number_to_string = {
    {255, "none"},      {0, "squibblesnail"}, {1, "squablesnail"}, {2, "scramblesnail"}, {3, "skittercrab"}, {4, "scattercrab"},
    {5, "shattercrab"}, {6, "squid"},         {7, "bigsquid"},     {8, "biggestsquid"},  {9, "bell"},        {10, "ember"},
    {11, "cuircuit"},   {12, "hedge"},        {13, "cloud"},       {14, "rock"},         {15, "wiggleworm"}, {16, "waggleworm"},
    {17, "skimskate"},  {18, "skimray"},      {19, "billow"},      {20, "howl"},         {21, "item"},       {22, "item2"},
    {23, "zip"},        {24, "zap"},          {25, "suculent"},    {26, "cactus"},       {27, "flickerfly"}, {28, "flitfly"},
    {29, "dragon"},     {30, "skull"},        {31, "ardu"}};

std::unordered_map<int, std::string> move_number_to_string = {
    {255, "none"},    {0, "smite"},     {1, "thought"},     {2, "wisper"}, {3, "blast"},     {4, "splash"},       {5, "squirt"},
    {6, "soak"},      {7, "current"},   {8, "breeze"},      {9, "bellow"}, {10, "torrent"},  {11, "blow"},        {12, "dirtburst"},
    {13, "slipfall"}, {14, "dirtfall"}, {15, "shatter"},    {16, "burn"},  {17, "melt"},     {18, "torch"},       {19, "stoke"},
    {20, "bolt"},     {21, "plasma"},   {22, "coil"},       {23, "zap"},   {24, "pollen"},   {25, "pollenburst"}, {26, "seedfall"},
    {27, "root"},     {28, "burst"},    {29, "elderBurst"}, {30, "slam"},  {31, "elderSlam"}};
