#include "src/Globals.hpp"
#include "src/engine/world/Event.hpp"

Event event;
void setup() {
    // initiate arduboy instance
    arduboy.begin();
    // engine.startFight(0);
    // engine.startEncounter(1, 1);
    //     here we set the framerate to 15, we do not need to run at
    //     default 60 and it saves us battery life
    arduboy.setFrameRate(45);
    arduboy.initRandomSeed();
    FX::begin(FX_DATA_PAGE);
    FX::setFont(arduboyFont, dcmNormal);   // select default font
    FX::setCursorRange(0, 32767);
    world = WorldEngine(&arduboy, &state, &engine);
    player = Player();

    state = GameState_t::WORLD;
    menu = Menu(&arduboy, &state, &player);

    engine = BattleEngine(&arduboy, &player, &menu, &state);
    arena = Arena(&menu, &player, &engine);
    menu2 = MenuV2(&arduboy);
    event = Event();
    // event.loadEvent(0, 1, 1);

    //  menu2.pushEvent(event);
}

void loop() {
    if (!arduboy.nextFrame()) {
        return;
    }
    FX::display(CLEAR_BUFFER);
    /*
        arduboy.fillRect(0, 0, 120, 120, WHITE);
        arduboy.fillRect(10, 10, 40, 20, BLACK);
        FX::drawBitmap(20, 20, npc, 0, dbfMasked);

    if (arduboy.justPressed(B_BUTTON)) {
        menu2.popMenu();
    }
    if (arduboy.justPressed(A_BUTTON)) {
        menu2.pushEvent(event);
    }
    */

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
    // menu2.drawPopMenu();
}
