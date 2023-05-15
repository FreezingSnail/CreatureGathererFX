#pragma once
#include "Arduboy2.h"
#include <stdint.h>

//Notes: character center on map at all times
enum Direction {
    Up,
    Down,
    Left,
    Right
};

class WorldEngine{
    private:
    Arduboy2* arduboy; 
    Direction playerDirection;
    int mapx, mapy;

    public:
    WorldEngine(Arduboy2* arduboy);
    void input();
    void runMap();
    void drawMap();

};