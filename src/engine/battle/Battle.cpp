#include "Battle.hpp"
#include "../../globals.hpp"

#include "../../creature/Creature.hpp"
#include "../../lib/Move.hpp"
#include "../../lib/ReadData.hpp"
#include <ArduboyFX.h>

BattleEngine::BattleEngine() {
}

uint8_t static calculateDamage(Action *action, Creature *committer, Creature *receiver) {
    // need to do something here with atk def stats
    Move move = committer->moveList[action->actionIndex];
    // float mod = getMatchupModifier(getMoveType(move),
    // uint8_t(receiver->type1))*getMatchupModifier(getMoveType(move),
    // uint8_t(receiver->type2))/2;
    uint8_t power = move.getMovePower();
    bool bonus = committer->moveTypeBonus(move.getMoveType());
    // TODO: this daamage formula is bad
    // TODO: apply stat modifiers, Maybe need to refactor this entirely
    uint16_t damage = (power * committer->statlist.attack) / (receiver->statlist.defense / 2);
    damage = applyModifier(damage, (Type)move.getMoveType(), receiver->types);
    damage = damage == 0 ? 1 : damage;
    // TODO (Snail) need to move this modifiers location in the formula

    // going too need to balance this eventually
    return damage;
}

static uint8_t aiChooseStrongestMove(Creature *commiter, Creature *receiver) {
    uint8_t selected = 0;
    uint16_t maxDamage = 0;

    DualType type = receiver->types;
    // TODO have multiple ai levels?
    // always choose the highest damage (hardest ai)
    for (uint8_t i = 0; i < 4; i++) {
        Action a;
        a.actionIndex = i;
        uint16_t damage = calculateDamage(&a, commiter, receiver);
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
}
//////////////////////////////////////////////////////////////////////////////
//
//        Entry Functions
//
//////////////////////////////////////////////////////////////////////////////

// TODO: make these one to get rid of repeated code
void BattleEngine::startFight(uint8_t optID) {
    this->loadOpponent(optID);
    this->loadPlayer();
    // gameState.state = GameState_t::BATTLE;
    this->activeBattle = true;
    playerAction.actionIndex = -1;
    opponentAction.actionIndex = -1;
    menuStack.push(MenuEnum::BATTLE_OPTIONS);
}
void BattleEngine::startArena(uint8_t optID) {
    this->loadOpponent(optID);
    resetOpponent();
    loadPlayer();
    // gameState.state = GameState_t::BATTLE;
    activeBattle = true;
    playerAction.actionIndex = -1;
    opponentAction.actionIndex = -1;
    menuStack.push(MenuEnum::BATTLE_OPTIONS);
    Serial.println("start arena");
}

void BattleEngine::startEncounter(uint8_t creatureID, uint8_t level) {
    this->LoadCreature(creatureID, level);
    this->loadPlayer();
    // gameState.state = GameState_t::BATTLE;
    this->activeBattle = true;
    playerAction.actionIndex = -1;
    opponentAction.actionIndex = -1;
    menuStack.push(MenuEnum::BATTLE_OPTIONS);
}

//////////////////////////////////////////////////////////////////////////////
//
//        flow control Functions
//
//////////////////////////////////////////////////////////////////////////////

// Need to change something here for the flow of the game
void BattleEngine::encounter() {
    if (this->checkLoss()) {
        this->endEncounter();
        dialogMenu.pushMenu(newDialogBox(LOSS, 0, 0));
        this->activeBattle = false;
        return;
    }

    if (this->checkWin()) {
        this->endEncounter();
        dialogMenu.pushMenu(newDialogBox(WIN, 0, 0));
        this->activeBattle = false;
        return;
    }

    this->turnTick();
}

void BattleEngine::turnTick() {

    switch (turnState) {
    case BattleState::TURN_INPUT: {
        if (!OpponentActionReady()) {
            this->opponentInput();
        }
        if (!PlayerActionReady()) {
            return;
        }

        int8_t order = (int8_t)this->playerAction.priority - (int8_t)this->opponentAction.priority;
        if (order > 0) {
            turnState = BattleState::PLAYER_ATTACK;

        } else if (order < 0) {
            turnState = BattleState::OPPONENT_ATTACK;

        } else {
            order = this->playerCur->statlist.speed - this->opponentCur->statlist.speed;
            if (order > 0 || order == 0) {
                turnState = BattleState::PLAYER_ATTACK;
            } else if (order < 0) {
                turnState = BattleState::OPPONENT_ATTACK;
            }
        }

    } break;
    case BattleState::PLAYER_ATTACK:
        this->commitPlayerAction();
        break;
    case BattleState::OPPONENT_RECEIVE_DAMAGE:
        this->commitPlayerAction();
        break;
    case BattleState::OPPONENT_RECEIVE_EFFECT_APPLICATION:
        this->commitPlayerAction();
        playerAction.actionIndex = -1;
        break;

    case BattleState::OPPONENT_ATTACK:
        this->commitOpponentAction();
        break;
    case BattleState::PLAYER_RECEIVE_DAMAGE:
        this->commitOpponentAction();
        break;
    case BattleState::PLAYER_RECEIVE_EFFECT_APPLICATION:
        this->commitOpponentAction();
        opponentAction.actionIndex = -1;
        break;
    case BattleState::END_TURN:
        updateState = true;
        break;

    default:
        break;
    }
}

bool BattleEngine::checkLoss() {
    if (this->playerHealths[0] <= 0 && this->playerHealths[1] <= 0 && this->playerHealths[2] <= 0) {
        return true;
    }
    return false;
}

bool BattleEngine::checkWin() {
    if (this->opponentHealths[0] <= 0 && this->opponentHealths[1] <= 0 && this->opponentHealths[2] <= 0) {
        return true;
    }
    return false;
}

// These are just place holders until menu & ai written for proper swapping
bool BattleEngine::checkPlayerFaint() {
    if (this->playerHealths[this->playerIndex] <= 0) {
        // TODO(BUG): can bacout out of change menu if creature is down
        dialogMenu.pushMenu(newDialogBox(FAINT, playerCur->id, 0));
        battleEventPlayer.push({BattleEventType::FAINT, playerCur->id, 0});
        if (!checkLoss()) {
            menuStack.push(MenuEnum::BATTLE_CREATURE_SELECT);
        }
        return true;
    }
    return false;
}

// TODO: Need to move this change action out to the normal loop so creature doesnt change mid turn
bool BattleEngine::checkOpponentFaint() {
    if (this->opponentHealths[this->opponentIndex] <= 0) {
        dialogMenu.pushMenu(newDialogBox(FAINT, opponentCur->id, 0));
        battleEventPlayer.push({BattleEventType::OPPONENT_FAINT, opponentCur->id, 0});
        if (!checkWin()) {
            this->opponentAction.setActionType(ActionType::CHNGE, Priority::FAST);
            this->opponentAction.actionIndex = int8_t(this->opponentIndex + 1);
        }
        return true;
    }
    return false;
}

void BattleEngine::commitPlayerAction() {
    if (!PlayerActionReady()) {
        return;
    }
    this->commitAction(&this->playerAction, this->playerCur, this->opponentCur, true);
    if (turnState == BattleState::OPPONENT_RECEIVE_DAMAGE && checkOpponentFaint() || !this->activeBattle) {
        playerAction.setActionType(ActionType::SKIP, Priority::NORMAL);
        return;
    }
}

void BattleEngine::commitOpponentAction() {
    if (!OpponentActionReady()) {
        return;
    }
    this->commitAction(&this->opponentAction, this->opponentCur, this->playerCur, false);
    if (turnState == BattleState::PLAYER_RECEIVE_DAMAGE && checkPlayerFaint() || !this->activeBattle) {
        opponentAction.setActionType(ActionType::SKIP, Priority::NORMAL);
        return;
    }
}

void BattleEngine::setMoveList(uint8_t **pointer) {
    *pointer = this->playerCur->moves;
}

void BattleEngine::changeCurMon(uint8_t index) {

    uint8_t j = 0;
    for (uint8_t i = 0; i < 3; i++) {
        if (this->playerCur == this->playerParty[i]) {
            continue;
        }
        if (j == index) {
            this->playerCur = this->playerParty[i];
            playerIndex = i;
            return;
        }
        j++;
    }
}

void BattleEngine::changeOptMon(uint8_t index) {
    this->opponentCur = &opponent.party[index];
    opponentIndex = index;
}

bool BattleEngine::tryCapture() {
    uint8_t roll = 0;
    // random(1, 10);
    return roll < 5;
}

void BattleEngine::endEncounter() {
    this->activeBattle = false;
    // gameState.state = GameState_t::WORLD;
    //  menu.clear();
}

//////////////////////////////////////////////////////////////////////////////
//
//        Input Functions
//
//////////////////////////////////////////////////////////////////////////////

// TODO: prob can delete all this
void ::BattleEngine::queueAction(ActionType type, uint8_t index) {

    Priority p = Priority::NORMAL;
    switch (type) {
    case ActionType::CHNGE:
    case ActionType::GATHER:
    case ActionType::ESCAPE:
        p = Priority::FAST;
    }
    this->playerAction.actionType = type;
    this->playerAction.priority = p;
    this->playerAction.actionIndex = index;
}

void BattleEngine::opponentInput() {
    // already queued an action
    if (opponentAction.actionIndex != -1) {
        return;
    }
    // ai to select best move
    this->opponentAction.setActionType(ActionType::ATTACK, Priority::NORMAL);
    this->opponentAction.actionIndex = aiChooseStrongestMove(opponentCur, playerCur);
}

//////////////////////////////////////////////////////////////////////////////
//
//        Event Execution Functions
//
//////////////////////////////////////////////////////////////////////////////

void BattleEngine::commitAction(Action *action, Creature *commiter, Creature *receiver, bool isPlayer) {
    switch (action->actionType) {
    case ActionType::ATTACK: {
        uint8_t damage = calculateDamage(action, commiter, receiver);
        Move move = commiter->moveList[action->actionIndex];
        debug = move.getMoveType();
        Modifier m1 = getModifier(Type(move.getMoveType()), receiver->types.getType1());

        Modifier m2 = getModifier(Type(move.getMoveType()), receiver->types.getType2());
        Modifier mod = combineModifier(m1, m2);

        if (turnState == BattleState::PLAYER_ATTACK || turnState == BattleState::OPPONENT_ATTACK) {
            if (isPlayer) {
                battleEventPlayer.push({BattleEventType::ATTACK, commiter->id, commiter->moves[action->actionIndex]});
                dialogMenu.pushMenu(newDialogBox(NAME, commiter->id, commiter->moves[action->actionIndex]));

            } else {
                battleEventPlayer.push({BattleEventType::OPPONENT_ATTACK, commiter->id, commiter->moves[action->actionIndex]});
                dialogMenu.pushMenu(newDialogBox(ENEMY_NAME, commiter->id, commiter->moves[action->actionIndex]));
            }

        } else if (turnState == BattleState::PLAYER_RECEIVE_DAMAGE || turnState == BattleState::OPPONENT_RECEIVE_DAMAGE) {
            if (mod != Modifier::Same) {
                Serial.println("effective");
                dialogMenu.pushMenu(newDialogBox(EFFECTIVENESS, uint24_t(mod), 0));
            }
            applyDamage(damage, receiver);

            if (isPlayer) {
                dialogMenu.pushMenu(newDialogBox(ENEMY_DAMAGE, receiver->id, damage));
                battleEventPlayer.push({BattleEventType::OPPONENT_DAMAGE, 0, damage});

            } else {
                battleEventPlayer.push({BattleEventType::DAMAGE, 0, damage});
                dialogMenu.pushMenu(newDialogBox(DAMAGE, receiver->id, damage));
            }
        } else {
            runEffect(commiter, receiver, move.getMoveEffect());
        }

        break;
    }
    case ActionType::GATHER:
        // idk if this is staying at all
        // player.storeCreature(0, this->opponentCur->id, this->opponentCur->level);
        // dialogMenu.pushMenu(newDialogBox(GATHERING, 0, 0));
        break;
    case ActionType::CHNGE:

        if (turnState == BattleState::PLAYER_ATTACK || turnState == BattleState::OPPONENT_ATTACK) {
            if (isPlayer) {
                dialogMenu.pushMenu(newDialogBox(TEAM_CHANGE, 0, 0));
                dialogMenu.pushMenu(newDialogBox(SWITCH, playerParty[action->actionIndex]->id, 0));
            } else {
                dialogMenu.pushMenu(newDialogBox(SWITCH, opponent.party[action->actionIndex].id, 0));
            }

        } else if (turnState == BattleState::PLAYER_RECEIVE_DAMAGE || turnState == BattleState::OPPONENT_RECEIVE_DAMAGE) {
            if (isPlayer) {
                this->changeCurMon(action->actionIndex);

            } else {
                this->changeOptMon(action->actionIndex);
            }
        }

        break;
    case ActionType::ESCAPE:
        // should add a check in here for opponent vs random encounter
        if (turnState == BattleState::PLAYER_ATTACK || turnState == BattleState::OPPONENT_ATTACK) {

            dialogMenu.pushMenu(newDialogBox(ESCAPE_ENCOUNTER, 0, 0));
        } else {
            this->endEncounter();
        }

        break;

    default:
        break;
    }
    updateState = true;
}

void BattleEngine::applyDamage(uint16_t damage, Creature *receiver) {
    if (receiver == this->playerCur) {
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
    OpponentSeed seed = readOpponentSeed(optID);
    this->opponent.loadOpt(&seed);
    this->resetOpponent();
}

void BattleEngine::LoadCreature(uint8_t creatureID, uint8_t level) {
    loadEncounterOpt(&this->opponent, creatureID, level);
    this->resetOpponent();
}

void BattleEngine::loadPlayer() {
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
    opponentAction.actionIndex = -1;
}

void BattleEngine::applyEffects() {
    for (uint8_t i = 0; i < 2; i++) {
        applyEffect(playerCur, playerCur->status.effects[i]);
        applyEffect(opponentCur, opponentCur->status.effects[i]);
    }
}

void BattleEngine::applyEffect(Creature *target, Effect effect) {
    bool applied = target->status.applyEffect(effect);
    if (!applied)
        return;
    applyBattleEffect(target, effect);
}

// TODO: apply the effects
void BattleEngine::applyBattleEffect(Creature *target, Effect effect) {
    DialogType dt = DialogType::PLAYER_EFFECT;
    StatModifer *statMods = &playerModifiers;
    if (target == opponentCur) {
        dt = DialogType::ENEMY_EFFECT;
        statMods = &opponentModifiers;
    }

    switch (effect) {
    case Effect::EGOED:
        statMods->setModifier(StatType::ATTACK_M, 1);
        break;
    case Effect::DRENCHED:
        break;
    case Effect::BUFFETED:
        statMods->setModifier(StatType::SPEED_M, -1);
        break;
    case Effect::STUMBLED:
        statMods->setModifier(StatType::ATTACK_M, -1);
        break;
    case Effect::BURNED:
        statMods->setModifier(StatType::DEFENSE_M, -1);
        break;
    case Effect::SHOCKED:
        statMods->setModifier(StatType::SPECIAL_ATTACK_M, -1);
        break;
    case Effect::ENTANGLED:
        statMods->setModifier(StatType::SPEED_M, -1);
        break;
    case Effect::CURSED:
        statMods->setModifier(StatType::SPECIAL_DEFENSE_M, -1);
        break;
    default:
        return;
    }
    // dialogMenu.pushMenu(newDialogBox(dt, target->id, uint24_t(effect)));
}

void BattleEngine::runEffect(Creature *commiter, Creature *other, Effect effect) {
    if (effect == Effect::NONE) {
        return;
    }
    uint8_t rate = getEffectRate(effect);
    uint8_t roll = 0;   // random(1, 100);
    if (roll > rate) {
        return;
    }
    bool selfTarget = selfEffect(effect);
    Creature *target = selfTarget ? commiter : other;
    applyEffect(target, effect);
}

bool BattleEngine::TurnReady() {
    return playerAction.actionIndex != -1 && opponentAction.actionIndex != -1;
}

bool BattleEngine::PlayerActionReady() {
    return playerAction.actionIndex != -1;
}

bool BattleEngine::OpponentActionReady() {
    return opponentAction.actionIndex != -1;
}