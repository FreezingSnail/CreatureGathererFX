#include "Arena.hpp"
#include "../../player/Player.hpp"
#include "../battle/Battle.hpp"
#include "../game/Menu.hpp"

Arena::Arena() {}
Arena::Arena(Menu *menu, Player *player, BattleEngine *engine) {
    this->menu = menu;
    this->player = player;
    this->engine = engine;
}

void
Arena::arenaLoop(Arduboy2 *arduboy) {
    this->menu->printMenu();
    if (this->registerIndex < 3) {
        this->registerRentals();
        this->displayRegisteredCount(arduboy);
    } else {
        this->startBattle();
    }
}

void
Arena::registerRentals() {
    int8_t creatureID = this->menu->registerCreature();
    if (creatureID >= 0) {
        this->player->loadreature(this->registerIndex, creatureID);
        this->registerIndex++;
    }
}

uint8_t
Arena::selectOpponent() {
    return 0;
}
void
Arena::startBattle() {
    this->engine->startFight(this->selectOpponent());
}

void
Arena::displayRegisteredCount(Arduboy2 *arduboy) {
    for (uint8_t i = 0; i < this->registerIndex; i++) {
        arduboy->drawCircle(100, 5 + (10 * i), 3, WHITE);
    }
}