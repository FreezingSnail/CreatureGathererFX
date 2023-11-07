#include "Battle.hpp"
#include <ArduboyFX.h>

#include "../../creature/Creature.hpp"
#include "../../fxdata/fxdata.h"
#include "../../opponent/Opponent.hpp"
#include "../../player/Player.hpp"
#include "../game/Gamestate.hpp"
#include "../game/Menu.hpp"

// #include "../../lib/TypeTable.hpp"
#include "../../data/opponentsData.h"
#include "../../lib/Move.hpp"

#define dbf __attribute__((optimize("-O0")))

BattleEngine::BattleEngine() {}
void BattleEngine::init(GameState_t *state) {
    this->state = state;
    this->activeBattle = false;
}

//////////////////////////////////////////////////////////////////////////////
//
//        Entry Functions
//
//////////////////////////////////////////////////////////////////////////////

void BattleEngine::startFight(Arduboy2 &arduboy, Player &player, Menu &menu, uint8_t optID) {
    this->loadOpponent(optID);
    this->loadPlayer(menu, player);
    *this->state = GameState_t::BATTLE;
    this->activeBattle = true;
    arduboy.setTextColor(BLACK);
}

void BattleEngine::startEncounter(Arduboy2 &arduboy, Player &player, Menu &menu, uint8_t creatureID, uint8_t level) {
    this->LoadCreature(creatureID, level);
    this->loadPlayer(menu, player);
    *this->state = GameState_t::BATTLE;
    this->activeBattle = true;
    arduboy.setTextColor(BLACK);
}

//////////////////////////////////////////////////////////////////////////////
//
//        flow control Functions
//
//////////////////////////////////////////////////////////////////////////////

// Need to change something here for the flow of the game
void BattleEngine::encounter(Arduboy2 &arduboy, Player &player, Menu &menu) {
    if (this->checkLoss()) {
        this->activeBattle = false;
        return;
    }

    if (this->checkWin()) {
        this->activeBattle = false;
        return;
    }

    this->drawScene(arduboy);
    menu.printMenu();
    this->turnTick(player, menu);
}

void BattleEngine::turnTick(Player &player, Menu &menu) {
    if (!this->getInput(menu)) {
        return;
    }
    this->opponentInput();
    int8_t order = (int8_t)this->playerAction.priority - (int8_t)this->opponentAction.priority;
    if (order > 0) {
        this->playerActionFirst(player, menu);
    } else if (order < 0) {
        this->opponentActionFirst(player, menu);
    } else {
        order = this->playerCur->statlist.speed - this->opponentCur->statlist.speed;
        if (order > 0 || order == 0) {
            this->playerActionFirst(player, menu);
        } else if (order < 0) {
            this->opponentActionFirst(player, menu);
        }
    }
}

bool BattleEngine::checkLoss() {
    // return uint8_t(this->awakeMons & 0b11100000) == uint8_t(0) ;
    if (this->playerHealths[0] <= 0 && this->playerHealths[1] <= 0 && this->playerHealths[2] <= 0) {
        this->endEncounter();
        return true;
    }
    return false;
}

bool BattleEngine::checkWin() {
    // return uint8_t(this->awakeMons & 0b00000111) == uint8_t(0) ;
    if (this->opponentHealths[0] <= 0 && this->opponentHealths[1] <= 0 && this->opponentHealths[2] <= 0) {
        this->endEncounter();
        return true;
    }
    return false;
}

// These are just place holders until menu & ai written for proper swapping
bool BattleEngine::checkPlayerFaint(Menu &menu) {
    if (this->playerHealths[this->playerIndex] <= 0) {
        this->playerIndex++;
        this->playerCur = this->playerParty[this->playerIndex];
        // this->awakeMons &= ~(1 << this->playerIndex);
        menu.registerMoveList(this->playerCur->moves[0], this->playerCur->moves[1], this->playerCur->moves[2], this->playerCur->moves[3]);
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
void BattleEngine::playerActionFirst(Player &player, Menu &menu) {
    this->commitAction(player, menu, &this->playerAction, this->playerCur, this->opponentCur);
    if (this->checkOpponentFaint() || !this->activeBattle)
        return;
    this->commitAction(player, menu, &this->opponentAction, this->opponentCur, this->playerCur);
    this->checkPlayerFaint(menu);
}

void BattleEngine::opponentActionFirst(Player &player, Menu &menu) {
    this->commitAction(player, menu, &this->opponentAction, this->opponentCur, this->playerCur);
    if (this->checkPlayerFaint(menu) || !this->activeBattle)
        return;
    this->commitAction(player, menu, &this->playerAction, this->playerCur, this->opponentCur);
    this->checkOpponentFaint();
}

void BattleEngine::changeCurMon(Menu &menu, uint8_t id) {
    uint8_t index = 0;
    for (uint8_t i = 0; i < 3; i++) {
        if (this->playerParty[i]->id == id) {
            this->playerCur = this->playerParty[i];
            index = i;
            break;
        }
    }
    this->playerCur = this->playerParty[index];
    menu.registerMoveList(this->playerCur->moves[0], this->playerCur->moves[1], this->playerCur->moves[2], this->playerCur->moves[3]);
    switch (index) {
    case 0:
        menu.registerCreatureList(this->playerParty[1]->id, this->playerParty[2]->id);
        break;
    case 1:
        menu.registerCreatureList(this->playerParty[0]->id, this->playerParty[2]->id);
        break;
    case 2:
        menu.registerCreatureList(this->playerParty[0]->id, this->playerParty[1]->id);
        break;
    }
}

bool BattleEngine::tryCapture() {
    uint8_t roll = random(1, 10);
    return roll < 5;
}

void BattleEngine::endEncounter() {
    this->activeBattle = false;
    *this->state = GameState_t::WORLD;
}

//////////////////////////////////////////////////////////////////////////////
//
//        Input Functions
//
//////////////////////////////////////////////////////////////////////////////

bool BattleEngine::getInput(Menu &menu) {
    // arduboy input from menu
    return menu.actionInput(&this->playerAction);
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

void BattleEngine::commitAction(Player &player, Menu &menu, Action *action, Creature *commiter, Creature *reciever) {
    switch (action->actionType) {
    case ActionType::ATTACK: {
        uint16_t damage = calculateDamage(menu, action, commiter, reciever);
        applyDamage(damage, reciever);
        break;
    }
    case ActionType::CATCH:
        if (this->tryCapture()) {
            player.storeCreature(0, this->opponentCur->id, this->opponentCur->level);
            endEncounter();
        }
        break;
    case ActionType::CHNGE:
        this->changeCurMon(menu, action->actionIndex);
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

uint16_t BattleEngine::calculateDamage(Menu &menu, Action *action, Creature *committer, Creature *reciever) {
    // need to do something here with atk def stats
    Move move = committer->moveList[action->actionIndex];
    // float mod = getMatchupModifier(getMoveType(move),
    // uint8_t(reciever->type1))*getMatchupModifier(getMoveType(move),
    // uint8_t(reciever->type2))/2;
    uint8_t power = move.getMovePower();
    bool bonus = committer->moveTypeBonus(committer->moves[action->actionIndex]);
    uint16_t damage = (power + committer->statlist.attack) / reciever->statlist.defense;
    damage = applyModifier(damage, (Type)move.getMoveType(), reciever->types);
    menu.printAttack(committer->id, committer->moves[action->actionIndex], Modifier::Same);

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
    uint24_t rowAddress = FX::readIndexedUInt24(opts, optID);
    FX::readDataObject(rowAddress, seed);

    this->opponent.load(&seed);
    this->resetOpponent();
}

void BattleEngine::LoadCreature(uint8_t creatureID, uint8_t level) {
    this->opponent.loadEncounterOpt(creatureID, level);
    this->resetOpponent();
}

void BattleEngine::loadPlayer(Menu &menu, Player &player) {
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
    menu.registerMoveList(this->playerCur->moves[0], this->playerCur->moves[1], this->playerCur->moves[2], this->playerCur->moves[3]);
    menu.registerCreatureList(this->playerParty[1]->id, this->playerParty[2]->id);
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

void BattleEngine::drawScene(Arduboy2 &arduboy) {
    this->drawPlayer(arduboy);
    this->drawOpponent(arduboy);
}

void BattleEngine::drawOpponent(Arduboy2 &arduboy) {
    // would be nice to flip this sprite
    // Sprites::drawSelfMasked(0, 0, creatureSprites, this->opponentCur->id);
    FX::drawBitmap(0, 0, creatureSprites, 1, dbmWhite);
    this->drawOpponentHP(arduboy);
}

void BattleEngine::drawPlayer(Arduboy2 &arduboy) {
    // Sprites::drawSelfMasked(96, 0, creatureSprites, this->playerCur->id);
    FX::drawBitmap(96, 0, creatureSprites, this->playerCur->id, dbmWhite);
    this->drawPlayerHP(arduboy);
}

void BattleEngine::drawPlayerHP(Arduboy2 &arduboy) {
    arduboy.setTextColor(WHITE);
    arduboy.setCursor(70, 35);
    arduboy.print(F("HP: "));
    arduboy.print((unsigned)this->playerHealths[this->playerIndex]);
    arduboy.print(F("/"));
    arduboy.print((unsigned)this->playerCur->statlist.hp);
    // arduboy.setCursor(32, 2);
    // arduboy.print(F("lv:"));
    // arduboy.print(this->playerCur->level);
    arduboy.setTextColor(BLACK);
}

void BattleEngine::drawOpponentHP(Arduboy2 &arduboy) {
    arduboy.setTextColor(WHITE);
    arduboy.setCursor(2, 35);
    arduboy.print(F("HP: "));
    arduboy.print((unsigned)this->opponentHealths[this->opponentIndex]);
    arduboy.print(F("/"));
    arduboy.print((unsigned)this->opponentCur->statlist.hp);
    // arduboy.setCursor(64, 2);
    // arduboy.print(F("lv:"));
    // arduboy.print(this->opponentCur->level);
    arduboy.setTextColor(BLACK);
}