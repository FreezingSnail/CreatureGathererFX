#include "Menu.hpp"

#include "../../action/Action.hpp"
#include "../../data/Creatures.hpp"
#include "../../external/FlashStringHelper.h"
#include "../../lib/Move.hpp"
#include "../../lib/Text.hpp"
#include "../../lib/Type.hpp"
#include "Arduboy2.h"

Menu::Menu(Arduboy2* arduboy) {
  this->arduboy = arduboy;
  this->cursorIndex = 0;
  this->arduboy->setTextColor(BLACK);
}

void Menu::registerMoveList(uint8_t move1, uint8_t move2, uint8_t move3,
                            uint8_t move4) {
  this->moveList[0] = move1;
  this->moveList[1] = move2;
  this->moveList[2] = move3;
  this->moveList[3] = move4;
}

bool Menu::actionInput(Action* action) {
  if (this->queued) {
    this->queued = false;
    action->setActionType(this->queuedAction.actionType);
    action->actionIndex = this->queuedAction.actionIndex;
    return true;
  }
  return false;
}
void Menu::setState(State_t s) { this->state = s; }

// This doesnt work at all lol
void Menu::wait() {
  while (!this->arduboy->justPressed(A_BUTTON)) {
    this->arduboy->pollButtons();
  }
  uint8_t count = 0;
  while (count < 15) {
    if (!this->arduboy->nextFrame()) continue;
    count++;
  }
  // this->arduboy->clear();
}

void Menu::printMenu() {
  this->moveCursor();
  this->tansverseMenu();
  switch (this->state) {
    case State_t::BATTLE:
      this->printBattleMenu();
    case State_t::WORLD:
      this->printWorldMenu();
    default:
      this->printBattleMenu();
  };
}

void Menu::printBattleMenu() {
  this->arduboy->fillRect(0, 32, 128, 32);
  this->printCursor();
  switch (this->curMenu) {
    case MenuType::BMAIN:
      this->arduboy->setCursor(6, 45);
      this->arduboy->print(asFlashStringHelper(moveM));
      this->arduboy->setCursor(64, 45);
      this->arduboy->print(asFlashStringHelper(itemM));
      this->arduboy->setCursor(6, 55);
      this->arduboy->print(asFlashStringHelper(changeM));
      this->arduboy->setCursor(64, 55);
      this->arduboy->print(asFlashStringHelper(escapeM));

      break;
    case MenuType::BMOVE:
      this->printMoveMenu();
      break;
  }
}

void Menu::printMoveMenu() {
  this->arduboy->setCursor(6, 45);
  this->arduboy->print(readFlashStringPointer(&moveNames[this->moveList[0]]));
  this->arduboy->setCursor(64, 45);
  this->arduboy->print(readFlashStringPointer(&moveNames[this->moveList[1]]));
  this->arduboy->setCursor(6, 55);
  this->arduboy->print(readFlashStringPointer(&moveNames[this->moveList[2]]));
  this->arduboy->setCursor(64, 55);
  this->arduboy->print(readFlashStringPointer(&moveNames[this->moveList[3]]));
}

void Menu::printItemMenu() {}

void Menu::printWorldMenu() {}

void Menu::printCreatureMenu() {}

void Menu::printInventoryMenu() {}

void Menu::printCursor() {
  switch (this->cursorIndex) {
    case 0:
      this->arduboy->setCursor(0, 45);
      break;
    case 1:
      this->arduboy->setCursor(58, 45);
      break;
    case 2:
      this->arduboy->setCursor(0, 55);
      break;
    case 3:
      this->arduboy->setCursor(58, 55);
      break;
  }
  this->arduboy->print(F(">"));
}

void Menu::moveCursor() {
  if (this->arduboy->justPressed(LEFT_BUTTON)) {
    this->cursorIndex--;
  } else if (this->arduboy->justPressed(RIGHT_BUTTON)) {
    this->cursorIndex++;
  }
}

void Menu::tansverseMenu() {
  if (this->arduboy->justPressed(A_BUTTON)) {
    switch (this->curMenu) {
      case BMAIN:
        switch (this->cursorIndex) {
          case 0:
            this->curMenu = BMOVE;
            break;
          case 1:
            this->curMenu = BITEM;
            break;
          case 2:
            this->curMenu = BCHANGE;
            break;
          case 3:
            this->queueAction(ActionType::ESCAPE, 0);
            // need to redeisgn how an action is qued
            break;
        }
        this->cursorIndex = 0;
        break;

      case BMOVE:
        switch (this->cursorIndex) {
          case 0:
            this->queueAction(ActionType::ATTACK, 0);
            break;
          case 1:
            this->queueAction(ActionType::ATTACK, 1);
            break;
          case 2:
            this->queueAction(ActionType::ATTACK, 2);
            break;
          case 3:
            this->queueAction(ActionType::ATTACK, 3);
            break;
        }
        this->cursorIndex = 0;
        break;
    }
  } else if (this->arduboy->justPressed(B_BUTTON)) {
    if (this->curMenu <= BCHANGE && this->curMenu >= BMOVE) {
      this->curMenu = BMAIN;
    }
  }
}

void Menu::queueAction(ActionType type, uint8_t index) {
  this->queued = true;
  this->queuedAction.setActionType(type);
  this->queuedAction.actionIndex = index;
}

void Menu::printAttack(uint8_t creatureID, uint8_t attackID,
                       Modifier modifier) {
  // this->arduboy->clear();
  this->arduboy->setCursor(1, 45);
  this->arduboy->fillRect(0, 32, 128, 32, WHITE);
  this->arduboy->print(readFlashStringPointer(&creatureNames[creatureID]));
  this->arduboy->print(F(" used"));
  // need a list of move names
  this->arduboy->setCursor(1, 55);
  this->arduboy->print(readFlashStringPointer(&moveNames[attackID]));
  this->arduboy->display();
  // Need to come back to this its not easy to get the modifier rn
  //  switch (Modifier) {
  //      case: Modifier::None {
  //          this->arduboy->println(F("weak hit"));
  //      }
  //      case: Modifier::Half {
  //          this->arduboy->println(F("it fails"));
  //      }
  //      case Modifier::Same {
  //          //this->arduboy->println(F("it fails"));
  //          // prob just say nothign for a normal hit?
  //      }
  //      case Modifier::Double {
  //          this->arduboy->println(F("heavy hit"));
  //      }
  //  }

  this->wait();
}