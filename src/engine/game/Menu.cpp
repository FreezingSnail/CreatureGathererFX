#include "Menu.hpp"
#include "../../action/Action.hpp"
#include "../../data/Creatures.hpp"
#include "../../external/FlashStringHelper.h"
#include "../../lib/Move.hpp"
#include "../../lib/Text.hpp"
#include "../../lib/Type.hpp"
#include "../draw.h"
#include "../game/Gamestate.hpp"
#include "Arduboy2.h"

#define dbf __attribute__((optimize("-O0")))

Menu::Menu(Arduboy2 *arduboy, GameState_t *state) {
  this->arduboy = arduboy;
  this->cursorIndex = 0;
  this->arduboy->setTextColor(BLACK);
  this->state = state;
}

void Menu::registerMoveList(uint8_t move1, uint8_t move2, uint8_t move3,
                            uint8_t move4) {
  this->moveList[0] = move1;
  this->moveList[1] = move2;
  this->moveList[2] = move3;
  this->moveList[3] = move4;
}

void Menu::registerCreatureList(uint8_t c1, uint8_t c2) {
  this->creatureList[0] = c1;
  this->creatureList[1] = c2;
}

bool Menu::actionInput(Action *action) {
  if (this->queued) {
    this->queued = false;
    action->setActionType(this->queuedAction.actionType,
                          this->queuedAction.priority);
    action->actionIndex = this->queuedAction.actionIndex;
    return true;
  }
  return false;
}

// This doesnt work at all lol
void Menu::wait() {
  while (!this->arduboy->justPressed(A_BUTTON)) {
    this->arduboy->pollButtons();
  }
  uint8_t count = 0;
  while (count < 15) {
    if (!this->arduboy->nextFrame())
      continue;
    count++;
  }
  // this->arduboy->clear();
}

void Menu::moveCursor() {
  if (*this->state == ARENA) {
    if (this->arduboy->justPressed(DOWN_BUTTON)) {
      this->cursorIndex++;
    } else if (this->arduboy->justPressed(UP_BUTTON)) {
      this->cursorIndex--;
    }
    if (this->cursorIndex < 0)
      this->cursorIndex = 0;
    if (this->cursorIndex > 31)
      this->cursorIndex = 31;
    return;
  }
  if (this->curMenu == BCHANGE) {
    if (this->arduboy->justPressed(DOWN_BUTTON)) {
      this->cursorIndex += 2;
    } else if (this->arduboy->justPressed(UP_BUTTON)) {
      this->cursorIndex -= 2;
    }
    return;
  }
  if (this->arduboy->justPressed(LEFT_BUTTON)) {
    this->cursorIndex--;
  } else if (this->arduboy->justPressed(RIGHT_BUTTON)) {
    this->cursorIndex++;
  }
  if (this->arduboy->justPressed(DOWN_BUTTON)) {
    this->cursorIndex += 2;
  } else if (this->arduboy->justPressed(UP_BUTTON)) {
    this->cursorIndex -= 2;
  }

  if (this->cursorIndex > 3) {
    this->cursorIndex = 0;
  }
  if (this->cursorIndex < 0) {
    this->cursorIndex = 3;
  }
}

void Menu::transverseMenu() {
  switch (*this->state) {
  case WORLD:
    break;
  case BATTLE:
    this->tansverseBattleMenu();
    break;
  case ARENA:
    break;
  }
}

void Menu::tansverseBattleMenu() {
  if (this->arduboy->justPressed(A_BUTTON)) {
    switch (this->curMenu) {
    case BMAIN:
      switch (this->cursorIndex) {
      case 0:
        this->curMenu = BMOVE;
        break;
      case 1:
        this->curMenu = BCATCH;
        this->queueAction(ActionType::CATCH, 0);
        this->curMenu = BMAIN;
        break;
      case 2:
        this->curMenu = BCHANGE;
        break;
      case 3:
        this->queueAction(ActionType::ESCAPE, 0);
        this->curMenu = BMAIN;
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
      this->curMenu = BMAIN;
      this->cursorIndex = 0;
      break;

    case BCHANGE:
      switch (this->cursorIndex) {
      case 0:
        this->queueAction(ActionType::CHNGE, this->creatureList[0]);
        break;
      case 2:
        this->queueAction(ActionType::CHNGE, this->creatureList[1]);
        break;
      }
      this->curMenu = BMAIN;
      this->cursorIndex = 0;
      break;
    }
  } else if (this->arduboy->justPressed(B_BUTTON)) {
    if (this->curMenu <= BCHANGE && this->curMenu >= BMOVE) {
      this->curMenu = BMAIN;
    }
  }
}

void dbf Menu::creatureRental() {
  this->arduboy->setCursor(0, 0);
  this->arduboy->print(F(">"));
  for (uint8_t i = 0; i < 6; i++) {
    this->arduboy->setCursor(10, i * 10);
    if (this->cursorIndex + i < 32)
      this->arduboy->print(
          readFlashStringPointer(&creatureNames[this->cursorIndex + i]));
  }

  if (this->arduboy->justPressed(A_BUTTON)) {
    // Register the creature
  }
}

int8_t Menu::registerCreature() {
  if (this->arduboy->justPressed(A_BUTTON)) {
    return this->cursorIndex;
  }
  return -1;
}

void Menu::queueAction(ActionType type, uint8_t index) {
  this->queued = true;
  Priority p = Priority::NORMAL;
  switch (type) {
  case ActionType::CHNGE:
  case ActionType::CATCH:
  case ActionType::ESCAPE:
    p = Priority::FAST;
  }
  this->queuedAction.setActionType(type, p);
  this->queuedAction.actionIndex = index;
}

//////////////////////////////////////////////////////////////////////////////
//
//        Draw Functions
//
//////////////////////////////////////////////////////////////////////////////

void Menu::printMenu() {
  this->moveCursor();
  this->transverseMenu();
  switch (*this->state) {
  case GameState_t::BATTLE:
    this->printBattleMenu();
  case GameState_t::WORLD:
    this->printWorldMenu();
    break;
  case GameState_t::ARENA:
    this->arduboy->setTextColor(WHITE);
    this->creatureRental();
  };
}

void Menu::drawInfoRec() {
  this->arduboy->fillRect(35, 1, 60, 30, WHITE);
  this->arduboy->drawRect(36, 2, 58, 28, BLACK);
}

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
  case MenuType::BCHANGE:
    this->printCreatureMenu();
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
  this->drawInfoRec();
  printMoveInfo(this->arduboy, this->moveList[cursorIndex]);
}

void Menu::printItemMenu() {}

void Menu::printWorldMenu() {}

void Menu::printCreatureMenu() {
  this->arduboy->setCursor(6, 45);
  this->arduboy->print(
      readFlashStringPointer(&creatureNames[this->creatureList[0]]));
  this->arduboy->setCursor(6, 55);
  this->arduboy->print(
      readFlashStringPointer(&creatureNames[this->creatureList[1]]));
}

void Menu::printInventoryMenu() {}

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