#include "Battle.hpp"

#include "../src/action/Action.hpp"
#include "creature/Creature.hpp"
#include "lib/Move.hpp"
#include "opponent/Opponent.hpp"
#include "player/Player.hpp"
#include <iostream>
#include <stdint.h>

#define XSTART 0
#define YSTART 43
#define MWIDTH 128
#define MHEIGHT 32

uint16_t static calculateDamage(Action *action, Creature *committer, Creature *reciever) {
    // need to do something here with atk def stats
    Move move = committer->moveList[action->actionIndex];
    // float mod = getMatchupModifier(getMoveType(move),
    // uint8_t(reciever->type1))*getMatchupModifier(getMoveType(move),
    // uint8_t(reciever->type2))/2;
    uint8_t power = move.getMovePower();
    bool bonus = committer->moveTypeBonus(move.getMoveType());
    uint16_t damage = (power + committer->statlist.attack) / reciever->statlist.defense;
    damage = applyModifier(damage, (Type)move.getMoveType(), reciever->types);
    damage = damage == 0 ? 1 : damage;
    // TODO (Snail) need to move this modifiers location in the formula

    // going too need to balance this eventually
    return damage == 0 ? 1 : damage;
}

static uint8_t choseMove(Creature *commiter, Creature *reciever) {
    uint8_t selected = 0;
    uint16_t maxDamage = 0;

    DualType type = reciever->types;
    // always choose the highest damage (hardest ai)
    for (uint8_t i = 0; i < 4; i++) {
        Action a;
        a.actionIndex = i;
        uint16_t damage = calculateDamage(&a, commiter, reciever);
        if (damage > maxDamage) {
            maxDamage = damage;
            selected = i;
        }
    }
    return selected;
}

void BattleEngine::init() {
    this->activeBattle = false;
}

uint8_t *BattleEngine::getPlayerCurCreatureMoves() {
    return this->playerCur->moves;
}

void BattleEngine::updateInactiveCreatures(uint8_t *list) {
    uint8_t index = 0;
    for (uint8_t i = 0; i < 3; i++) {
        if (this->playerCur != this->playerParty[i]) {
            list[index] = this->playerParty[i]->id;
            index++;
        }
    }
}

Creature *BattleEngine::getCreature(uint8_t index) {
    for (uint8_t i = 0; i < 3; i++) {
        if (this->playerParty[i]->id == index) {
            return this->playerParty[i];
        }
    }
    return nullptr;
}
//////////////////////////////////////////////////////////////////////////////
//
//        Entry Functions
//
//////////////////////////////////////////////////////////////////////////////

void BattleEngine::startFight(Player &player, uint8_t optID) {
    this->loadOpponent(optID);
    this->loadPlayer(player);
    this->activeBattle = true;
}
void BattleEngine::startArena(Player &player, uint8_t optID) {
    opponent.Read(optID);
    resetOpponent();

    loadPlayer(player);
    activeBattle = true;
}

void BattleEngine::startEncounter(Player &player, uint8_t creatureID, uint8_t level) {
    this->LoadCreature(creatureID, level);
    this->loadPlayer(player);
    this->activeBattle = true;
}

//////////////////////////////////////////////////////////////////////////////
//
//        flow control Functions
//
//////////////////////////////////////////////////////////////////////////////

// Need to change something here for the flow of the game
void BattleEngine::encounter(Player &player) {
    if (this->checkLoss()) {
        this->endEncounter();
        // menu2->dialogMenu.pushMenu(newDialogBox(LOSS, 0, 0));
        this->activeBattle = false;
        return;
    }

    if (this->checkWin()) {
        this->endEncounter();
        // menu2->dialogMenu.pushMenu(newDialogBox(WIN, 0, 0));
        this->activeBattle = false;
        return;
    }

    this->turnTick(player);
}

void BattleEngine::turnTick(Player &player) {
    // std::cout << "turn tick" << std::endl;
    this->playerAction.setActionType(ActionType::ATTACK, Priority::NORMAL);
    this->playerAction.actionIndex = choseMove(playerCur, opponentCur);
    // std::cout << "player using action " << int(playerAction.actionIndex) << std::endl;

    this->opponentInput();
    // std::cout << "opponent using action " << int(opponentAction.actionIndex) << std::endl;

    int8_t order = (int8_t)this->playerAction.priority - (int8_t)this->opponentAction.priority;
    if (order > 0) {
        this->playerActionFirst(player);
    } else if (order < 0) {
        this->opponentActionFirst(player);
    } else {
        order = this->playerCur->statlist.speed - this->opponentCur->statlist.speed;
        if (order > 0 || order == 0) {
            this->playerActionFirst(player);
        } else if (order < 0) {
            this->opponentActionFirst(player);
        }
    }
}

bool BattleEngine::checkLoss() {
    // return uint8_t(this->awakeMons & 0b11100000) == uint8_t(0) ;
    if (this->playerHealths[0] <= 0 && this->playerHealths[1] <= 0 && this->playerHealths[2] <= 0) {
        // std::cout << "player lost " << std::endl;
        return true;
    }
    return false;
}

bool BattleEngine::checkWin() {
    // return uint8_t(this->awakeMons & 0b00000111) == uint8_t(0) ;
    if (this->opponentHealths[0] <= 0 && this->opponentHealths[1] <= 0 && this->opponentHealths[2] <= 0) {
        // std::cout << "player won " << std::endl;

        return true;
    }
    return false;
}

// These are just place holders until menu & ai written for proper swapping
bool BattleEngine::checkPlayerFaint() {
    if (this->playerHealths[this->playerIndex] <= 0) {
        // std::cout << "player " << int(playerCur->id) << " lost to " << int(opponentCur->id) << std::endl;

        this->playerIndex++;
        this->playerCur = this->playerParty[this->playerIndex];
        //  this->awakeMons &= ~(1 << this->playerIndex);
        // menu2->dialogMenu.pushMenu(newDialogBox(FAINT, playerCur->id, 0));
        // menu2->push(BATTLE_CREATURE_SELECT);

        return true;
    }
    return false;
}

bool BattleEngine::checkOpponentFaint() {
    if (this->opponentHealths[this->opponentIndex] <= 0) {
        // std::cout << "opponent " << int(opponentCur->id) << " lost to " << int(playerCur->id) << std::endl;
        // menu2->dialogMenu.pushMenu(newDialogBox(FAINT, opponentCur->id, 0));
        this->opponentIndex++;
        this->opponentCur = &(this->opponent.party[this->opponentIndex]);
        if (!checkWin()) {
            // menu2->dialogMenu.pushMenu(newDialogBox(SWITCH, opponentCur->id, 0));
        }

        // this->awakeMons &= ~(1 << this->opponentIndex+5);
        return true;
    }
    return false;
}

// Need to add a win/loss check ejection
void BattleEngine::playerActionFirst(Player &player) {
    this->commitAction(player, &this->playerAction, this->playerCur, this->opponentCur, true);
    if (this->checkOpponentFaint() || !this->activeBattle)
        return;
    this->commitAction(player, &this->opponentAction, this->opponentCur, this->playerCur, false);
    this->checkPlayerFaint();
}

void BattleEngine::opponentActionFirst(Player &player) {
    this->commitAction(player, &this->opponentAction, this->opponentCur, this->playerCur, false);
    if (this->checkPlayerFaint() || !this->activeBattle)
        return;
    this->commitAction(player, &this->playerAction, this->playerCur, this->opponentCur, true);
    this->checkOpponentFaint();
}

void BattleEngine::setMoveList(uint8_t **pointer) {
    *pointer = this->playerCur->moves;
}

void BattleEngine::changeCurMon(uint8_t id) {
    for (uint8_t i = 0; i < 3; i++) {
        if (this->playerParty[i]->id == id) {
            this->playerCur = this->playerParty[i];
            break;
        }
    }
    // this->playerCur = this->playerParty[index];
    //  menu.registerMoveList(this->playerCur->moves[0], this->playerCur->moves[1], this->playerCur->moves[2],
    //  this->playerCur->moves[3]); switch (index) { case 0:
    //      menu.registerCreatureList(this->playerParty[1]->id, this->playerParty[2]->id);
    //      break;
    //  case 1:
    //      menu.registerCreatureList(this->playerParty[0]->id, this->playerParty[2]->id);
    //      break;
    //  case 2:
    //      menu.registerCreatureList(this->playerParty[0]->id, this->playerParty[1]->id);
    //      break;
    //  }
}

bool BattleEngine::tryCapture() {
    // uint8_t roll = random(1, 10);
    return 5 < 5;
}

void BattleEngine::endEncounter() {
    this->activeBattle = false;
    // this->menu2->pop();
}

//////////////////////////////////////////////////////////////////////////////
//
//        Input Functions
//
//////////////////////////////////////////////////////////////////////////////

void ::BattleEngine::queueAction(ActionType type, uint8_t index) {

    this->queued = true;
    Priority p = Priority::NORMAL;
    switch (type) {
    case ActionType::CHNGE:
    case ActionType::GATHER:
    case ActionType::ESCAPE:
        p = Priority::FAST;
    }
    this->playerAction.setActionType(type, p);
    this->playerAction.actionIndex = index;
}

void BattleEngine::opponentInput() {
    // ai to select best move
    // For now just do the first slot attack
    this->opponentAction.setActionType(ActionType::ATTACK, Priority::NORMAL);
    this->opponentAction.actionIndex = choseMove(opponentCur, playerCur);
}

//////////////////////////////////////////////////////////////////////////////
//
//        Event Execution Functions
//
//////////////////////////////////////////////////////////////////////////////

void BattleEngine::commitAction(Player &player, Action *action, Creature *commiter, Creature *reciever, bool isPlayer) {
    switch (action->actionType) {
    case ActionType::ATTACK: {
        uint16_t damage = calculateDamage(action, commiter, reciever);
        applyDamage(damage, reciever);
        if (isPlayer) {
            // menu2->dialogMenu.pushMenu(newDialogBox(NAME, commiter->id, 0));
            // menu2->dialogMenu.pushMenu(newDialogBox(DAMAGE, uint24_t(damage), damage));
        } else {
            // menu2->dialogMenu.pushMenu(newDialogBox(ENEMY_NAME, commiter->id, 0));
            // menu2->dialogMenu.pushMenu(newDialogBox(ENEMY_DAMAGE, uint24_t(damage), damage));
        }
        break;
    }
    case ActionType::GATHER:
        if (this->tryCapture()) {
            // idk if this is staying at all
            // player.storeCreature(0, this->opponentCur->id, this->opponentCur->level);
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

//////////////////////////////////////////////////////////////////////////////
//
//        Load Functions
//
//////////////////////////////////////////////////////////////////////////////

void BattleEngine::loadOpponent(uint8_t optID) {
    // OpponentSeed_t seed = OpponentSeed_t{0, 0, 1};
    // uint24_t rowAddress = FX::readIndexedUInt24(opts, optID);
    // FX::readDataObject(rowAddress, seed);

    // this->opponent.load(&seed);
    this->resetOpponent();
}

void BattleEngine::LoadCreature(uint8_t creatureID, uint8_t level) {
    // this->opponent.loadEncounterOpt(creatureID, level);
    this->resetOpponent();
}

void BattleEngine::loadPlayer(Player &player) {
    this->playerParty[0] = &(player.party[0]);
    this->playerParty[1] = &(player.party[1]);
    this->playerParty[2] = &(player.party[2]);
    // for now the hp will refil every encounter so we dont need to use the player
    // field
    // this->playerHealths[i] = player.creatureHPs[i];
    this->playerHealths[0] = this->playerParty[0]->statlist.hp;
    this->playerHealths[1] = this->playerParty[1]->statlist.hp;
    this->playerHealths[2] = this->playerParty[2]->statlist.hp;

    this->playerIndex = 0;
    this->playerCur = this->playerParty[0];
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
    // this->drawPlayer(arduboy);
    // this->drawOpponent(arduboy);
}

void BattleEngine::drawOpponent() {
    // would be nice to flip this sprite
    // Sprites::drawSelfMasked(0, 0, creatureSprites, this->opponentCur->id);
    // FX::drawBitmap(0, 0, creatureSprites, opponentCur->id, dbmWhite);
    // this->drawOpponentHP(arduboy);
}

void BattleEngine::drawPlayer() {
    // Sprites::drawSelfMasked(96, 0, creatureSprites, this->playerCur->id);
    // FX::drawBitmap(96, 0, creatureSprites, this->playerCur->id, dbmWhite);
    // this->drawPlayerHP(arduboy);
}

void BattleEngine::drawPlayerHP() {
    // arduboy.setTextColor(WHITE);
    // arduboy.setCursor(70, 35);
    // arduboy.print(F("HP: "));
    // arduboy.print((unsigned)this->playerHealths[this->playerIndex]);
    // arduboy.print(F("/"));
    // arduboy.print((unsigned)this->playerCur->statlist.hp);
    // arduboy.setCursor(32, 2);
    // arduboy.print(F("lv:"));
    // arduboy.print(this->playerCur->level);
    // arduboy.setTextColor(BLACK);
}

void BattleEngine::drawOpponentHP() {
    // arduboy.setTextColor(WHITE);
    // arduboy.setCursor(2, 35);
    // arduboy.print(F("HP: "));
    // arduboy.print((unsigned)this->opponentHealths[this->opponentIndex]);
    // arduboy.print(F("/"));
    // arduboy.print((unsigned)this->opponentCur->statlist.hp);
    // // arduboy.setCursor(64, 2);
    // // arduboy.print(F("lv:"));
    // // arduboy.print(this->opponentCur->level);
    // arduboy.setTextColor(BLACK);
}