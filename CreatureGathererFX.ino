#include "Arduboy2.h"
#include "src/engine/arena/Arena.hpp"
#include "src/engine/battle/Battle.hpp"
#include "src/engine/game/Gamestate.hpp"
#include "src/engine/game/Menu.hpp"
#include "src/engine/world/World.hpp"
#include "src/player/Player.hpp"

Arduboy2 arduboy;
Player player = Player();
GameState_t state = GameState_t::ARENA;
Menu menu = Menu(&arduboy, &state, &player);
BattleEngine engine = BattleEngine(&arduboy, &player, &menu, &state);
Arena arena = Arena(&menu, &player, &engine);
WorldEngine world = WorldEngine(&arduboy, &state, &engine);

void setup() {
  // initiate arduboy instance
  arduboy.begin();
  // engine.startFight(0);
  // engine.startEncounter(1, 1);
  //     here we set the framerate to 15, we do not need to run at
  //     default 60 and it saves us battery life
  arduboy.setFrameRate(30);
  arduboy.initRandomSeed();
}

void loop() {
  if (!arduboy.nextFrame()) {
    return;
  }
  arduboy.clear();

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

  arduboy.display();
}
