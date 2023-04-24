#include "Arduboy2.h"
//#include "src/lib/TypeTable.hpp"
#include "src/engine/battle/Battle.hpp"  
#include "src/player/Player.hpp"
#include "src/engine/game/menu.hpp"
// make an instance of arduboy used for many functions
Arduboy2 arduboy;
Player player = Player();
Menu menu = Menu(&arduboy);
BattleEngine engine = BattleEngine(&arduboy, &menu);
bool state = true;


// This function runs once in your game.
// use it for anything that needs to be set only once in your game.
void setup() {
  // initiate arduboy instance
  arduboy.begin();
  //engine.startEncounter(&player, 0);
  // here we set the framerate to 15, we do not need to run at
  // default 60 and it saves us battery life
  arduboy.setFrameRate(15);
}


// our main game loop, this runs once every cycle/frame.
// this is where our game logic goes.
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
  


  // then we finaly we tell the arduboy to display what we just wrote to the display
}
