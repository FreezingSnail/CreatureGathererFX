#include <Arduboy2.h>
#include <ArduboyFX.h>

#include "src/engine/arena/Arena.hpp"
#include "src/engine/battle/Battle.hpp"
#include "src/engine/game/Gamestate.hpp"
#include "src/engine/game/Menu.hpp"
#include "src/engine/world/World.hpp"
#include "src/fxdata/fxdata.h"
#include "src/player/Player.hpp"

Arduboy2 arduboy;
Player player;
GameState_t state;
Menu menu;
BattleEngine engine;
Arena arena;
WorldEngine world; 

void setup() {
  // initiate arduboy instance
  arduboy.begin();
  // engine.startFight(0);
  // engine.startEncounter(1, 1);
  //     here we set the framerate to 15, we do not need to run at
  //     default 60 and it saves us battery life
  arduboy.setFrameRate(30);
  arduboy.initRandomSeed();
  FX::begin(FX_DATA_PAGE);
  FX::setFont(arduboyFont, dcmNormal); // select default font
  FX::setCursorRange(0,32767); 
  
  world = WorldEngine(&arduboy, &state, &engine);
  player = Player();
  state = GameState_t::ARENA;
  menu = Menu(&arduboy, &state, &player);
  engine = BattleEngine(&arduboy, &player, &menu, &state);
  arena = Arena(&menu, &player, &engine);
}

void loop() {
  if (!arduboy.nextFrame()) {
    return;
  }

  arduboy.pollButtons();
  switch (state) {
  case GameState_t::BATTLE:
    engine.encounter();
    break;
  case GameState_t::WORLD:
    world.runMap();
    break;
  case GameState_t::ARENA:
    arena.arenaLoop(&arduboy);
    break;
  }

  FX::display(CLEAR_BUFFER);
}
