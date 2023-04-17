#include "Arduboy2.h"
//#include "src/lib/TypeTable.hpp"
#include "src/engine/battle/Battle.hpp"  
#include "src/player/Player.hpp"
#include "src/data/opponentsData.h"

// make an instance of arduboy used for many functions
Arduboy2 arduboy;
Player player = Player();
BattleEngine engine = BattleEngine(&arduboy);


// This function runs once in your game.
// use it for anything that needs to be set only once in your game.
void setup() {
  // initiate arduboy instance
  arduboy.begin();

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
  arduboy.pollButtons();

  // first we clear our screen to black
  arduboy.clear();

  // we set our cursor 5 pixels to the right and 10 down from the top
  // (positions start at 0, 0) 
  //arduboy.setCursor(4, 9);

  // then we print to screen what is in the Quotation marks ""
  //arduboy.print(F("Hello, world!"));
  engine.encounter(&player, &op1);

  // then we finaly we tell the arduboy to display what we just wrote to the display
  arduboy.display();
}
