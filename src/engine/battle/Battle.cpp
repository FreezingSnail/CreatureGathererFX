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

//#define DEBUG

BattleEngine::BattleEngine(Arduboy2 *arduboy, Player *player, Menu *menu,
                           GameState *state) {
  this->arduboy = arduboy;
  this->player = player;
  this->menu = menu;
  this->state = state;
  this->activeBattle = false;
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

  // #ifdef DEBUG
  //     while(!this->arduboy->justPressed(B_BUTTON)){
  //         this->arduboy->setTextColor(WHITE);
  //         this->arduboy->pollButtons();
  //         this->arduboy->clear();
  //         this->arduboy->print(F("p:  atk:  def:\n"));
  //         this->arduboy->print((unsigned)power);  this->arduboy->print(F("
  //         "));
  //         this->arduboy->print((unsigned)committer->statlist.attack);this->arduboy->print(F("
  //         "));
  //         this->arduboy->print((unsigned)reciever->statlist.defense);this->arduboy->print(F("
  //         ")); this->arduboy->print(F("\n")); this->arduboy->print(F("move
  //         Type: rtype: mod: \n"));
  //         this->arduboy->print((unsigned)getMoveType(move));
  //         this->arduboy->print(F(" "));
  //        // this->arduboy->print((unsigned)reciever->type1);
  //        this->arduboy->print(F(" "));
  //         //this->arduboy->print((mod));this->arduboy->print(F(" "));
  //         this->arduboy->display();
  //     }
  // #endif

  uint16_t damage =
      (power + committer->statlist.attack) / reciever->statlist.defense;
  damage = applyModifier(damage, (Type)move.getMoveType(), reciever->types);
  // if ( damage == 0 ){
  //     if (mod == 0) {
  //         return 0;
  //     }
  //     return 1;
  // }

  this->menu->printAttack(committer->id, committer->moves[action->actionIndex],
                          Modifier::Same);

  // TODO (Snail) need to move this modifiers location in the formula
  if (bonus) {
    return damage * 2;
  }

  // going too need to balance this eventually
  return damage;
}

// battle loop

// Need to change something here for the flow of the game
void __attribute__((optimize("-O0"))) BattleEngine::encounter() {
  this->db = 3;
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
#ifdef DEBUG
  // this->printEncounter();
  //  this->playerCur->printMoves();
#endif
}

void BattleEngine::resetOpponent() {
  this->opponentIndex = 0;
  this->opponentCur = &(this->opponent.party[0]);
  this->opponentHealths[0] = this->opponent.party[0].statlist.hp;
  this->opponentHealths[1] = this->opponent.party[1].statlist.hp;
  this->opponentHealths[2] = this->opponent.party[2].statlist.hp;
}

void BattleEngine::loadOpponent(uint8_t optID) {
  OpponentSeed_t seed = OpponentSeed_t{0, 0, 1};
  memcpy_P(&seed, &opts[optID], sizeof(OpponentSeed_t));

  this->opponent.load(&seed);
  this->resetOpponent();
}

void __attribute__((optimize("-O0")))
BattleEngine::LoadCreature(uint8_t creatureID, uint8_t level) {
  this->db = 2;
  this->opponent.levels[2] = 12;
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
  this->playerHealths[0] = 10;
  this->playerHealths[0] = this->playerParty[0]->statlist.hp;
  this->playerHealths[1] = this->playerParty[1]->statlist.hp;
  this->playerHealths[2] = this->playerParty[2]->statlist.hp;

  this->playerIndex = 0;
  this->playerCur = this->playerParty[0];
  this->menu->registerMoveList(
      this->playerCur->moves[0], this->playerCur->moves[1],
      this->playerCur->moves[2], this->playerCur->moves[3]);
}

void BattleEngine::startFight(uint8_t optID) {
  // this->arduboy->print("starting encounter\n");
  this->loadOpponent(optID);
  this->loadPlayer(this->player);
  this->menu->setState(State_t::BATTLE);
  this->activeBattle = true;
  // this->encounter();
}

void __attribute__((optimize("-O0")))
BattleEngine::startEncounter(uint8_t creatureID, uint8_t level) {
  this->LoadCreature(creatureID, level);
  this->loadPlayer(this->player);
  this->menu->setState(State_t::BATTLE);
  this->activeBattle = true;
}

void BattleEngine::turnTick() {
  if (!this->getInput()) {
    return;
  }
  this->opponentInput();
  int8_t order = this->playerAction.priority != this->opponentAction.priority;
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

// Need to add a win/loss check ejection
void BattleEngine::playerActionFirst() {
  this->commitAction(&this->playerAction, this->playerCur, this->opponentCur);
  if (this->checkOpponentFaint()) return;
  this->commitAction(&this->opponentAction, this->opponentCur, this->playerCur);
  this->checkPlayerFaint();
}

void BattleEngine::opponentActionFirst() {
  this->commitAction(&this->opponentAction, this->opponentCur, this->playerCur);
  if (this->checkPlayerFaint()) return;
  this->commitAction(&this->playerAction, this->playerCur, this->opponentCur);
  this->checkOpponentFaint();
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

bool BattleEngine::checkLoss() {
  // return uint8_t(this->awakeMons & 0b11100000) == uint8_t(0) ;
  if (this->playerHealths[0] <= 0 && this->playerHealths[1] <= 0 &&
      this->playerHealths[2] <= 0) {
    *this->state = GameState::OVERWORLD;
    return true;
  }
  return false;
}

bool BattleEngine::checkWin() {
  // return uint8_t(this->awakeMons & 0b00000111) == uint8_t(0) ;
  if (this->opponentHealths[0] <= 0 && this->opponentHealths[1] <= 0 &&
      this->opponentHealths[2] <= 0) {
    *this->state = GameState::OVERWORLD;
    return true;
  }
  return false;
}

void BattleEngine::endEncounter() {
  this->arduboy->print(F("\nend encounter \n"));
}

bool BattleEngine::getInput() {
  // arduboy input from menu
  return this->menu->actionInput(&this->playerAction);
}

void BattleEngine::opponentInput() {
  // ai to select best move

  // For now just do the first slot attack
  this->opponentAction.setActionType(ActionType::ATTACK);
  this->opponentAction.actionIndex = 1;
}

void BattleEngine::commitAction(Action *action, Creature *commiter,
                                Creature *reciever) {
  switch (action->actionType) {
    case ActionType::ATTACK: {
      uint16_t damage = calculateDamage(action, commiter, reciever);
      applyDamage(damage, reciever);
      break;
    }
    case ActionType::ITEM:
      break;
    case ActionType::CHNGE:
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
  this->arduboy->setCursor(32, 2);
  this->arduboy->print(F("lv:"));
  this->arduboy->print(this->playerCur->level);
}

void BattleEngine::drawOpponentHP() {
  this->arduboy->setCursor(2, 35);
  this->arduboy->print(F("HP: "));
  this->arduboy->print((unsigned)this->opponentHealths[this->opponentIndex]);
  this->arduboy->print(F("/"));
  this->arduboy->print((unsigned)this->opponentCur->statlist.hp);
  this->arduboy->setCursor(64, 2);
  this->arduboy->print(F("lv:"));
  this->arduboy->print(this->opponentCur->level);
}

#ifdef DEBUG

void BattleEngine::printEncounter() {
  // while (!this->arduboy->justPressed(B_BUTTON)) {
  //   this->arduboy->pollButtons();
  //   this->arduboy->clear();
  //   this->arduboy->setCursor(0, 0);
  //   this->arduboy->print(F("P: cur: "));
  //   this->arduboy->print((unsigned)this->playerIndex);
  //   this->arduboy->print(F(" lvl: "));
  //   this->arduboy->print((unsigned)this->playerCur->level);
  //   this->arduboy->print(F("\nhp:"));
  //   for (int i = 0; i < 3; i++) {
  //     this->arduboy->print(F(" "));
  //     this->arduboy->print((unsigned)this->playerHealths[i]);
  //   }
  //   this->arduboy->print(F("\nO: cur: "));
  //   this->arduboy->print((unsigned)this->opponentIndex);
  //   this->arduboy->print(F(" lvl: "));
  //   this->arduboy->print((unsigned)this->opponentCur->level);
  //   this->arduboy->print(F("\nhp:"));
  //   for (int i = 0; i < 3; i++) {
  //     this->arduboy->print(F(" "));
  //     this->arduboy->print((unsigned)this->opponentHealths[i]);
  //   }
  //   this->arduboy->display();
  // }
}

#endif