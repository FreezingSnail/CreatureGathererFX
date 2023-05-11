#include "Arduboy2.h"
#include "src/engine/battle/Battle.hpp"
#include "src/engine/game/Menu.hpp"
#include "src/player/Player.hpp"


Arduboy2 arduboy;
Player player = Player();
Menu menu = Menu(&arduboy);
BattleEngine engine = BattleEngine(&arduboy, &menu);

void setup() {
  // initiate arduboy instance
  arduboy.begin();
  engine.startEncounter(&player, 0);
  // here we set the framerate to 15, we do not need to run at
  // default 60 and it saves us battery life
  arduboy.setFrameRate(15);
}

void loop() {
  if (!(arduboy.nextFrame())) {
    return;
  }
  arduboy.clear();

  arduboy.pollButtons();
  if (true) {
    engine.encounter();
  }
  // engine.encounter();

  arduboy.display();
}
