#include "Arena.hpp"
#include "../../player/Player.hpp"
#include "../battle/Battle.hpp"
#include "../game/Menu.hpp"
#include <Arduboy2.h>
#include <ArduboyFX.h>

Arena::Arena() {}
Arena::Arena(Menu *menu, Player *player, BattleEngine *engine) {
    this->menu = menu;
    this->player = player;
    this->engine = engine;
}

void Arena::arenaLoop(Arduboy2 *arduboy) {
    if (this->start) {
        this->start = false;
        this->startBattle();
    }

    if (this->registerIndex < 3) {
        this->menu->printMenu();
        this->registerRentals();
        this->displayRegisteredCount(arduboy);
        //  } else if (this->moveIndex < 12) {
        //     this->cursorIndex = 0;
        //     this->registerMoves();
    } else {
        // this->start = true;
    }
}

void Arena::registerRentals() {
    int8_t creatureID = this->menu->registerCreature();
    if (creatureID >= 0) {
        this->player->loadreature(this->registerIndex, creatureID);
        this->registerIndex++;
    }
}

// void Arena::registerMoves() {
//     // load creature move pool
//     // uint32_t movePool = FX::readIndexedUInt32(MoveLists::moveList, this->player->party[this->moveCreature].id);
//     this->arduboy->setCursor(0, 0);
//     this->arduboy->print(this->moveIndex);

//     if (this->moveIndex < 4) {
//     } else if (this->moveIndex < 8) {
//     } else {
//     }
//     if (this->arduboy->justPressed(A_BUTTON)) {
//         this->moveIndex++;
//     }
// }

uint8_t Arena::selectOpponent() { return 0; }
void Arena::startBattle() { this->engine->startFight(this->selectOpponent()); }

void Arena::displayRegisteredCount(Arduboy2 *arduboy) {
    for (uint8_t i = 0; i < this->registerIndex; i++) {
        arduboy->drawCircle(100 + (10 * i), 55, 3, WHITE);
    }
}