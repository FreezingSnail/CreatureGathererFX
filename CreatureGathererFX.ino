#include "Arduboy2.h"
//#include "src/lib/TypeTable.hpp"
#include "src/engine/battle/Battle.hpp"  
#include "src/player/Player.hpp"
#include "src/engine/game/Menu.hpp"
// make an instance of arduboy used for many functions
Arduboy2 arduboy;
Player player = Player();
Menu menu = Menu(&arduboy);
BattleEngine engine = BattleEngine(&arduboy, &menu);
bool state = true;


void setup() {
  // initiate arduboy instance
  arduboy.begin();
  //engine.startEncounter(&player, 0);
  // here we set the framerate to 15, we do not need to run at
  // default 60 and it saves us battery life
  arduboy.setFrameRate(15);
}


void loop() {
  // pause render until it's time for the next frame
  if (!(arduboy.nextFrame()))
    return;
  arduboy.clear();

  arduboy.pollButtons();
  if(state) {
  engine.startEncounter(&player, 0);
  }
  state = false;
  arduboy.print("The end");
  //engine.encounter();

  arduboy.display();
}
