#include "Battle.hpp"
#include <ArduboyFX.h>

#include "../../creature/Creature.hpp"
#include "../../engine/menu/MenuV2.hpp"
#include "../../fxdata/fxdata.h"
#include "../../opponent/Opponent.hpp"
#include "../../player/Player.hpp"
#include "../game/Gamestate.hpp"

// #include "../../lib/TypeTable.hpp"
#include "../../lib/Move.hpp"

#define dbf __attribute__((optimize("-O0")))

BattleEngine::BattleEngine() {}

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

void BattleEngine::init(GameState_t *state, MenuV2 *menu2) {
    this->state = state;
    this->activeBattle = false;
    this->menu2 = menu2;
}

uint8_t *BattleEngine::getPlayerCurCreatureMoves() { return this->playerCur->moves; }

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
}
//////////////////////////////////////////////////////////////////////////////
//
//        Entry Functions
//
//////////////////////////////////////////////////////////////////////////////

void BattleEngine::startFight(Player &player, uint8_t optID) {
    this->loadOpponent(optID);
    this->loadPlayer(player);
    *this->state = GameState_t::BATTLE;
    this->activeBattle = true;
    FX::setFontMode(dcmBlack);   // select default font
    this->menu2->push(BATTLE_OPTIONS);
}
void BattleEngine::startArena(Player &player, uint8_t optID) {
    opponent.Read(optID);
    resetOpponent();
    loadPlayer(player);
    *this->state = GameState_t::BATTLE;
    activeBattle = true;
    FX::setFontMode(dcmBlack);   // select default font
    menu2->push(BATTLE_OPTIONS);
}

void BattleEngine::startEncounter(Player &player, uint8_t creatureID, uint8_t level) {
    this->LoadCreature(creatureID, level);
    this->loadPlayer(player);
    *this->state = GameState_t::BATTLE;
    this->activeBattle = true;
    FX::setFontMode(dcmBlack);   // select default font
    this->menu2->push(BATTLE_OPTIONS);
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
        menu2->dialogMenu.pushMenu(newDialogBox(LOSS, 0, 0));
        this->activeBattle = false;
        return;
    }

    if (this->checkWin()) {
        this->endEncounter();
        menu2->dialogMenu.pushMenu(newDialogBox(WIN, 0, 0));
        this->activeBattle = false;
        return;
    }

    this->turnTick(player);
    this->drawScene();
}

void BattleEngine::turnTick(Player &player) {
    if (!this->queued) {
        return;
    }
    this->opponentInput();
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
    this->queued = false;
}

bool BattleEngine::checkLoss() {
    // return uint8_t(this->awakeMons & 0b11100000) == uint8_t(0) ;
    if (this->playerHealths[0] <= 0 && this->playerHealths[1] <= 0 && this->playerHealths[2] <= 0) {
        return true;
    }
    return false;
}

bool BattleEngine::checkWin() {
    // return uint8_t(this->awakeMons & 0b00000111) == uint8_t(0) ;
    if (this->opponentHealths[0] <= 0 && this->opponentHealths[1] <= 0 && this->opponentHealths[2] <= 0) {
        return true;
    }
    return false;
}

// These are just place holders until menu & ai written for proper swapping
bool BattleEngine::checkPlayerFaint() {
    if (this->playerHealths[this->playerIndex] <= 0) {
        // this->playerIndex++;
        // this->playerCur = this->playerParty[this->playerIndex];
        //  this->awakeMons &= ~(1 << this->playerIndex);
        menu2->dialogMenu.pushMenu(newDialogBox(FAINT, playerCur->id, 0));
        menu2->push(BATTLE_CREATURE_SELECT);

        return true;
    }
    return false;
}

bool BattleEngine::checkOpponentFaint() {
    if (this->opponentHealths[this->opponentIndex] <= 0) {
        menu2->dialogMenu.pushMenu(newDialogBox(FAINT, opponentCur->id, 0));
        this->opponentIndex++;
        this->opponentCur = &(this->opponent.party[this->opponentIndex]);
        if (!checkWin()) {
            menu2->dialogMenu.pushMenu(newDialogBox(SWITCH, opponentCur->id, 0));
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

void BattleEngine::setMoveList(uint8_t **pointer) { *pointer = this->playerCur->moves; }

void BattleEngine::changeCurMon(uint8_t id) {
    // TODO this breaks with repeate creatures on roster
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
    uint8_t roll = random(1, 10);
    return roll < 5;
}

void BattleEngine::endEncounter() {
    this->activeBattle = false;
    *this->state = GameState_t::WORLD;
    this->menu2->pop();
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

void dbf BattleEngine::commitAction(Player &player, Action *action, Creature *commiter, Creature *reciever, bool isPlayer) {
    switch (action->actionType) {
    case ActionType::ATTACK: {
        uint16_t damage = calculateDamage(action, commiter, reciever);
        Move move = commiter->moveList[action->actionIndex];
        debug = move.getMoveType();
        Modifier m1 = getModifier(Type(move.getMoveType()), reciever->types.getType1());
        debug = uint8_t(reciever->types.getType1());

        Modifier m2 = getModifier(Type(move.getMoveType()), reciever->types.getType2());
        Modifier mod = combineModifier(m1, m2);
        debug = uint8_t(reciever->types.getType2());

        applyDamage(damage, reciever);
        if (isPlayer) {
            menu2->dialogMenu.pushMenu(newDialogBox(NAME, commiter->id, 0));
            menu2->dialogMenu.pushMenu(newDialogBox(DAMAGE, uint24_t(damage), damage));
        } else {
            menu2->dialogMenu.pushMenu(newDialogBox(ENEMY_NAME, commiter->id, 0));
            menu2->dialogMenu.pushMenu(newDialogBox(ENEMY_DAMAGE, uint24_t(damage), damage));
        }
        switch (mod) {
        case Modifier::Quarter:
            menu2->dialogMenu.pushMenu(newDialogBox(EFFECTIVENESS, uint24_t(Modifier::Quarter), 0));
            break;
        case Modifier::Half:
            menu2->dialogMenu.pushMenu(newDialogBox(EFFECTIVENESS, uint24_t(Modifier::Half), 0));
            break;
        case Modifier::Double:
            menu2->dialogMenu.pushMenu(newDialogBox(EFFECTIVENESS, uint24_t(Modifier::Double), 0));
            break;
        case Modifier::Quadruple:
            menu2->dialogMenu.pushMenu(newDialogBox(EFFECTIVENESS, uint24_t(Modifier::Quadruple), 0));
            break;
        }
        break;
    }
    case ActionType::GATHER:

        // idk if this is staying at all
        // player.storeCreature(0, this->opponentCur->id, this->opponentCur->level);
        // menu2->dialogMenu.pushMenu(newDialogBox(GATHERING, 0, 0));

        break;
    case ActionType::CHNGE:
        menu2->dialogMenu.pushMenu(newDialogBox(TEAM_CHANGE, 0, 0));
        menu2->dialogMenu.pushMenu(newDialogBox(SWITCH, playerParty[action->actionIndex]->id, 0));
        this->changeCurMon(action->actionIndex);
        break;
    case ActionType::ESCAPE:
        // should add a check in here for opponent vs random encounter
        menu2->dialogMenu.pushMenu(newDialogBox(ESCAPE_ENCOUNTER, 0, 0));
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
    this->drawPlayer();
    this->drawOpponent();
}

void BattleEngine::drawOpponent() {
    // would be nice to flip this sprite
    // Sprites::drawSelfMasked(0, 0, creatureSprites, this->opponentCur->id);
    FX::drawBitmap(0, 0, creatureSprites, opponentCur->id, dbmWhite);
    this->drawOpponentHP();
}

void BattleEngine::drawPlayer() {
    // Sprites::drawSelfMasked(96, 0, creatureSprites, this->playerCur->id);
    FX::drawBitmap(96, 0, creatureSprites, this->playerCur->id, dbmWhite);
    this->drawPlayerHP();
}

void BattleEngine::drawPlayerHP() {
    FX::setFont(arduboyFont, dcmNormal);
    FX::setCursor(70, 35);   // select default font
    FX::drawString(MenuFXData::hpText);
    FX::drawNumber((unsigned)this->playerHealths[this->playerIndex]);
    FX::drawString(("/"));
    FX::drawNumber((unsigned)this->playerCur->statlist.hp);
    FX::setFont(arduboyFont, dcmBlack);
}

void BattleEngine::drawOpponentHP() {
    FX::setFont(arduboyFont, dcmNormal);
    FX::setCursor(2, 35);   // select default font
    FX::drawString(MenuFXData::hpText);
    FX::drawNumber((unsigned)this->opponentHealths[this->opponentIndex]);
    FX::drawString(("/"));
    FX::drawNumber((unsigned)this->opponentCur->statlist.hp);
    FX::setFont(arduboyFont, dcmBlack);
}