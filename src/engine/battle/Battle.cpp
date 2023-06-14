#include "Battle.hpp"

#include "../../creature/Creature.hpp"
#include "../../opponent/Opponent.hpp"
#include "../../player/Player.hpp"
#include "../game/Gamestate.hpp"
#include "../game/Menu.hpp"
#include "Arduboy2.h"
//#include "../../lib/TypeTable.hpp"
#include "../../data/opponentsData.h"
#include "../../lib/Move.hpp"
#include "../../sprites/creatureSprites.h"

#define dbf __attribute__((optimize("-O0")))

BattleEngine::BattleEngine(Arduboy2 *arduboy, Player *player, Menu *menu,
                           GameState_t *state) {
  this->arduboy = arduboy;
  this->player = player;
  this->menu = menu;
  this->state = state;
  this->activeBattle = false;
}

//////////////////////////////////////////////////////////////////////////////
//
//        Entry Functions
//
//////////////////////////////////////////////////////////////////////////////

void BattleEngine::startFight(uint8_t optID) {
  this->loadOpponent(optID);
  this->loadPlayer(this->player);
  *this->state = GameState_t::BATTLE;
  this->activeBattle = true;
}

void BattleEngine::startEncounter(uint8_t creatureID, uint8_t level) {
  this->LoadCreature(creatureID, level);
  this->loadPlayer(this->player);
  *this->state = GameState_t::BATTLE;
  this->activeBattle = true;
}

//////////////////////////////////////////////////////////////////////////////
//
//        flow control Functions
//
//////////////////////////////////////////////////////////////////////////////

// Need to change something here for the flow of the game
void BattleEngine::encounter() {
  if (this->checkLoss()) {
    this->activeBattle = false;
    return;
  }

  if (this->checkWin()) {
    this->activeBattle = false;
    return;
  }

  this->menu->printMenu();
  this->drawScene();
  this->turnTick();
}

void BattleEngine::turnTick() {
  if (!this->getInput()) {
    return;
  }
  this->opponentInput();
  int8_t order = (int8_t)this->playerAction.priority -
                 (int8_t)this->opponentAction.priority;
  if (order > 0) {
    this->playerActionFirst();
  } else if (order < 0) {
    this->opponentActionFirst();
  } else {
    order = this->playerCur->statlist.speed - this->opponentCur->statlist.speed;
    if (order > 0 || order == 0) {
      this->playerActionFirst();
    } else if (order < 0) {
      this->opponentActionFirst();
    }
  }
}

bool BattleEngine::checkLoss() {
  // return uint8_t(this->awakeMons & 0b11100000) == uint8_t(0) ;
  if (this->playerHealths[0] <= 0 && this->playerHealths[1] <= 0 &&
      this->playerHealths[2] <= 0) {
    this->endEncounter();
    return true;
  }
  return false;
}

bool BattleEngine::checkWin() {
  // return uint8_t(this->awakeMons & 0b00000111) == uint8_t(0) ;
  if (this->opponentHealths[0] <= 0 && this->opponentHealths[1] <= 0 &&
      this->opponentHealths[2] <= 0) {
    this->endEncounter();
    return true;
  }
  return false;
}

// These are just place holders until menu & ai written for proper swapping
bool BattleEngine::checkPlayerFaint() {
  if (this->playerHealths[this->playerIndex] <= 0) {
    this->playerIndex++;
    this->playerCur = this->playerParty[this->playerIndex];
    // this->awakeMons &= ~(1 << this->playerIndex);
    this->menu->registerMoveList(
        this->playerCur->moves[0], this->playerCur->moves[1],
        this->playerCur->moves[2], this->playerCur->moves[3]);
    return true;
  }
  return false;
}

bool BattleEngine::checkOpponentFaint() {
  if (this->opponentHealths[this->opponentIndex] <= 0) {
    this->opponentIndex++;
    this->opponentCur = &(this->opponent.party[this->opponentIndex]);
    // this->awakeMons &= ~(1 << this->opponentIndex+5);
    return true;
  }
  return false;
}

// Need to add a win/loss check ejection
void BattleEngine::playerActionFirst() {
  this->commitAction(&this->playerAction, this->playerCur, this->opponentCur);
  if (this->checkOpponentFaint() || !this->activeBattle)
    return;
  this->commitAction(&this->opponentAction, this->opponentCur, this->playerCur);
  this->checkPlayerFaint();
}

void BattleEngine::opponentActionFirst() {
  this->commitAction(&this->opponentAction, this->opponentCur, this->playerCur);
  if (this->checkPlayerFaint() || !this->activeBattle)
    return;
  this->commitAction(&this->playerAction, this->playerCur, this->opponentCur);
  this->checkOpponentFaint();
}

void BattleEngine::changeCurMon(uint8_t id) {
  int index = 0;
  for (uint8_t i = 0; i < 3; i++) {
    if (this->playerParty[i]->id == id) {
      this->playerCur = this->playerParty[i]->id;
      index = i;
      break;
    }
  }
  this->playerCur = this->playerParty[index];
  this->menu->registerMoveList(
      this->playerCur->moves[0], this->playerCur->moves[1],
      this->playerCur->moves[2], this->playerCur->moves[3]);
  switch (index) {
  case 0:
    this->menu->registerCreatureList(this->playerParty[1]->id,
                                     this->playerParty[2]->id);
    break;
  case 1:
    this->menu->registerCreatureList(this->playerParty[0]->id,
                                     this->playerParty[2]->id);
    break;
  case 2:
    this->menu->registerCreatureList(this->playerParty[0]->id,
                                     this->playerParty[1]->id);
    break;
  }
}

bool BattleEngine::tryCapture() {
  int roll = random(1, 10);
  return roll < 5;
}

void BattleEngine::endEncounter() {
  this->arduboy->print(F("\nend encounter \n"));
  this->activeBattle = false;
  *this->state = GameState_t::WORLD;
}

//////////////////////////////////////////////////////////////////////////////
//
//        Input Functions
//
//////////////////////////////////////////////////////////////////////////////

bool BattleEngine::getInput() {
  // arduboy input from menu
  return this->menu->actionInput(&this->playerAction);
}

void BattleEngine::opponentInput() {
  // ai to select best move
  // For now just do the first slot attack
  this->opponentAction.setActionType(ActionType::ATTACK, Priority::NORMAL);
  this->opponentAction.actionIndex = 1;
}

//////////////////////////////////////////////////////////////////////////////
//
//        Event Execution Functions
//
//////////////////////////////////////////////////////////////////////////////

void BattleEngine::commitAction(Action *action, Creature *commiter,
                                Creature *reciever) {
  switch (action->actionType) {
  case ActionType::ATTACK: {
    uint16_t damage = calculateDamage(action, commiter, reciever);
    applyDamage(damage, reciever);
    break;
  }
  case ActionType::CATCH:
    if (this->tryCapture()) {
      this->player->storeCreature(0, this->opponentCur->id,
                                  this->opponentCur->level);
      endEncounter();
    }
    break;
  case ActionType::CHNGE:
    this->changeCurMon(action->actionIndex);
    break;
  case ActionType::ESCAPE:
    // should add a check in here for opponent vs random encounter
    this->endEncounter();
    break;

  default:
    break;
  }
}

void BattleEngine::applyDamage(uint16_t damage, Creature *reciever) {
  if (reciever == this->playerCur) {
    uint8_t hp = this->playerHealths[this->playerIndex];
    this->playerHealths[this->playerIndex] -= damage >= hp ? hp : damage;
  } else {
    uint8_t hp = this->opponentHealths[this->opponentIndex];
    this->opponentHealths[this->opponentIndex] -= damage >= hp ? hp : damage;
  }
}

uint16_t BattleEngine::calculateDamage(Action *action, Creature *committer,
                                       Creature *reciever) {
  // need to do something here with atk def stats
  Move move = committer->moveList[action->actionIndex];
  // float mod = getMatchupModifier(getMoveType(move),
  // uint8_t(reciever->type1))*getMatchupModifier(getMoveType(move),
  // uint8_t(reciever->type2))/2;
  uint8_t power = move.getMovePower();
  bool bonus = committer->moveTypeBonus(committer->moves[action->actionIndex]);
  uint16_t damage =
      (power + committer->statlist.attack) / reciever->statlist.defense;
  damage = applyModifier(damage, (Type)move.getMoveType(), reciever->types);
  this->menu->printAttack(committer->id, committer->moves[action->actionIndex],
                          Modifier::Same);

  // TODO (Snail) need to move this modifiers location in the formula
  if (bonus) {
    return damage * 2;
  }
  // going too need to balance this eventually
  return damage;
}

//////////////////////////////////////////////////////////////////////////////
//
//        Load Functions
//
//////////////////////////////////////////////////////////////////////////////

void BattleEngine::loadOpponent(uint8_t optID) {
  OpponentSeed_t seed = OpponentSeed_t{0, 0, 1};
  memcpy_P(&seed, &opts[optID], sizeof(OpponentSeed_t));

  this->opponent.load(&seed);
  this->resetOpponent();
}

void BattleEngine::LoadCreature(uint8_t creatureID, uint8_t level) {
  this->opponent.loadEncounterOpt(creatureID, level);
  this->resetOpponent();
}

void BattleEngine::loadPlayer(Player *player) {
  this->playerParty[0] = &(player->party[0]);
  this->playerParty[1] = &(player->party[1]);
  this->playerParty[2] = &(player->party[2]);
  // for now the hp will refil every encounter so we dont need to use the player
  // field
  // this->playerHealths[i] = player->creatureHPs[i];
  this->playerHealths[0] = this->playerParty[0]->statlist.hp;
  this->playerHealths[1] = this->playerParty[1]->statlist.hp;
  this->playerHealths[2] = this->playerParty[2]->statlist.hp;

  this->playerIndex = 0;
  this->playerCur = this->playerParty[0];
  this->menu->registerMoveList(
      this->playerCur->moves[0], this->playerCur->moves[1],
      this->playerCur->moves[2], this->playerCur->moves[3]);
  this->menu->registerCreatureList(this->playerParty[1]->id,
                                   this->playerParty[2]->id);
}

void BattleEngine::resetOpponent() {
  this->opponentIndex = 0;
  this->opponentCur = &(this->opponent.party[0]);
  this->opponentHealths[0] = this->opponent.party[0].statlist.hp;
  this->opponentHealths[1] = this->opponent.party[1].statlist.hp;
  this->opponentHealths[2] = this->opponent.party[2].statlist.hp;
}

//////////////////////////////////////////////////////////////////////////////
//
//        Draw Functions
//
//////////////////////////////////////////////////////////////////////////////

void BattleEngine::drawScene() {
  this->drawPlayer();
  this->drawOpponent();
}

void BattleEngine::drawOpponent() {
  // would be nice to flip this sprite
  Sprites::drawSelfMasked(0, 0, creatureSprites, this->opponentCur->id);
  this->drawOpponentHP();
}

void BattleEngine::drawPlayer() {
  Sprites::drawSelfMasked(96, 0, creatureSprites, this->playerCur->id);
  this->drawPlayerHP();
}

void BattleEngine::drawPlayerHP() {
  this->arduboy->setCursor(70, 35);
  this->arduboy->print(F("HP: "));
  this->arduboy->print((unsigned)this->playerHealths[this->playerIndex]);
  this->arduboy->print(F("/"));
  this->arduboy->print((unsigned)this->playerCur->statlist.hp);
  // this->arduboy->setCursor(32, 2);
  // this->arduboy->print(F("lv:"));
  // this->arduboy->print(this->playerCur->level);
}

void BattleEngine::drawOpponentHP() {
  this->arduboy->setCursor(2, 35);
  this->arduboy->print(F("HP: "));
  this->arduboy->print((unsigned)this->opponentHealths[this->opponentIndex]);
  this->arduboy->print(F("/"));
  this->arduboy->print((unsigned)this->opponentCur->statlist.hp);
  // this->arduboy->setCursor(64, 2);
  // this->arduboy->print(F("lv:"));
  // this->arduboy->print(this->opponentCur->level);
}