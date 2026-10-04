#include "Battle.hpp"
#include "../../globals.hpp"

#include "../../creature/Creature.hpp"
#include "../../lib/Move.hpp"
#include "../../lib/random.hpp"
#include "../../lib/Effect.hpp"

// Legacy engine loaders remain until the session port. Keep their declarations
// local while the session boundary is introduced.
OpponentSeed readOpponentSeed(uint8_t index);
void ReadOpt(Opponent *opt, uint8_t index);
void loadEncounterOpt(Opponent *opt, uint8_t creatureID, uint8_t level);
uint8_t getEffectRateFX(uint8_t id);

static __attribute__((noinline)) void pushBattleDialog(DialogType type, uint24_t number, uint16_t damage) {
    dialogMenu.pushMenu(newDialogBox(type, number, damage));
}

BattleEngine::BattleEngine() {
}

void BattleEngine::updateFightState() {
    switch (turnState) {
    case BattleState::TURN_INPUT: {
        // handled automatically
    } break;
    case BattleState::PLAYER_ATTACK:
        turnState = BattleState::OPPONENT_RECEIVE_DAMAGE;
        break;
    case BattleState::OPPONENT_RECEIVE_DAMAGE:
        turnState = BattleState::OPPONENT_RECEIVE_EFFECT_APPLICATION;
        break;
    case BattleState::OPPONENT_ATTACK:
        turnState = BattleState::OPPONENT_RECEIVE_EFFECT_APPLICATION;
        break;
    case BattleState::PLAYER_RECEIVE_DAMAGE:
        turnState = BattleState::PLAYER_RECEIVE_EFFECT_APPLICATION;
        break;
    case BattleState::PLAYER_RECEIVE_EFFECT_APPLICATION:
        if (PlayerActionReady()) {
            turnState = BattleState::PLAYER_ATTACK;
        } else {
            turnState = BattleState::END_TURN;
        }
        break;
    case BattleState::OPPONENT_RECEIVE_EFFECT_APPLICATION:
        if (OpponentActionReady()) {
            turnState = BattleState::OPPONENT_ATTACK;
        } else {
            turnState = BattleState::END_TURN;
        }
        break;
    case BattleState::END_TURN:
        turnState = BattleState::TURN_INPUT;
        break;
    }
}

inline uint16_t applyIntMod(uint16_t value, int8_t mod) {
    mod = mod * 2;
    if (mod == 0) {
        return value;
    } else if (mod < 0) {
        return value / (mod * -1);
    }
    return value * mod;
}

Modifier BattleEngine::getTypeStatusModifier(Creature *committer, Creature *receiver) {
    Modifier committerEffectMod1 = typeEffectModifier(committer->status.effects[0], committer->types);
    Modifier committerEffectMod2 = typeEffectModifier(committer->status.effects[1], committer->types);
    Modifier committerEffectMod = combineModifier(committerEffectMod1, committerEffectMod2);

    Modifier receiverEffectMod1 = typeEffectModifier(receiver->status.effects[0], receiver->types);
    Modifier receiverEffectMod2 = typeEffectModifier(receiver->status.effects[1], receiver->types);
    Modifier receiverEffectMod = inverseModifier(combineModifier(receiverEffectMod1, receiverEffectMod2));

    return combineModifier(committerEffectMod, receiverEffectMod);
}

uint16_t BattleEngine::calculateDamage(Action *action, Creature *committer, Creature *receiver) {
    Move move = committer->moveList[action->actionIndex];
    uint8_t power = move.getMovePower();
    bool phys = move.isPhysical();

    Modifier typeEffectModifier = getTypeStatusModifier(committer, receiver);
    bool bonus = committer->moveTypeBonus(move.getMoveType());

    StatType committerBonus = phys ? StatType::ATTACK_M : StatType::SPECIAL_ATTACK_M;
    StatType receiverBonus = phys ? StatType::DEFENSE_M : StatType::SPECIAL_DEFENSE_M;

    int8_t commiterAtkMod = committer->statMods.getModifier(committerBonus);
    int8_t receiverDefMod = receiver->statMods.getModifier(receiverBonus);

    // TODO: this damage formula is bad
    // TODO: apply stat modifiers, Maybe need to refactor this entirely
    uint16_t damage = applyIntMod((power * committer->statlist.attack), commiterAtkMod) / applyIntMod((receiver->statlist.defense / 2), receiverDefMod);
    damage = applyModifier(damage, (Type)move.getMoveType(), receiver->types, typeEffectModifier);

    damage = damage == 0 ? 0 : damage;
    return damage;
}

uint8_t BattleEngine::aiChooseStrongestMove() {
    uint8_t selected = 0;
    uint16_t maxDamage = 0;

    DualType type = opponentCur->types;
    // TODO have multiple ai levels?
    // always choose the highest damage (hardest ai)
    for (uint8_t i = 0; i < 4; i++) {
        Action a;
        a.actionIndex = i;
        uint16_t damage = calculateDamage(&a, playerCur, opponentCur);
        if (damage > maxDamage) {
            maxDamage = damage;
            selected = i;
        }
    }
    return selected;
}

void BattleEngine::init() {
    debug = 0;
    for (uint8_t i = 0; i < PARTY_SIZE; ++i) {
        playerParty[i] = nullptr;
        opponentHealths[i] = 0;
    }
    playerCur = nullptr;
    opponentCur = nullptr;
    // A global mode entry placement-constructs a fresh BattleEngine first, so
    // its Opponent member has already run its constructor without a large
    // temporary copy on the constrained stack.
    playerIndex = 0;
    opponentIndex = 0;
    playerAction = Action();
    opponentAction = Action();
    playerAction.actionIndex = -1;
    opponentAction.actionIndex = -1;
    activeBattle = false;
    turnState = BattleState::TURN_INPUT;
    updateState = false;
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
            if (index > 1) {
                return;
            }
        }
    }
}

Creature *BattleEngine::getCreature(uint8_t index) {
    uint8_t i = 0;
    for (; i < 3; i++) {
        if (this->playerParty[i]->id == index) {
            break;
        }
    }
    return this->playerParty[i];
}
//////////////////////////////////////////////////////////////////////////////
//
//        Entry Functions
//
//////////////////////////////////////////////////////////////////////////////

// TODO: make these one to get rid of repeated code
void BattleEngine::startFight(uint8_t optID) {
    this->loadOpponent(optID);
#ifndef TEST
    FxReadCounter::transitionExact(17);
#endif
    // ReadOpt(&opponent, optID);
    // resetOpponent();
    this->loadPlayer();
    this->activeBattle = true;
    playerAction.actionIndex = -1;
    opponentAction.actionIndex = -1;
    menuStack.push(MenuEnum::BATTLE_OPTIONS);
    turnState = BattleState::TURN_INPUT;
}
void BattleEngine::startArena(uint8_t optID) {
    ReadOpt(&this->opponent, optID);
#ifndef TEST
    FxReadCounter::transitionExact(16);
#endif
    resetOpponent();
    loadPlayer();
    activeBattle = true;
    playerAction.actionIndex = -1;
    opponentAction.actionIndex = -1;
    menuStack.push(MenuEnum::BATTLE_OPTIONS);
}

void BattleEngine::startEncounter(uint8_t creatureID, uint8_t level) {
    this->LoadCreature(creatureID, level);
#ifndef TEST
    FxReadCounter::transitionExact(5);
#endif
    this->loadPlayer();
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
        return;
    }

    if (this->checkWin()) {
        this->endEncounter();
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
        if (!this->commitPlayerAction()) return;
        break;
    case BattleState::OPPONENT_RECEIVE_DAMAGE:
        if (!this->commitPlayerAction()) return;
        break;
    case BattleState::OPPONENT_RECEIVE_EFFECT_APPLICATION:
        if (!this->commitPlayerAction()) return;
        playerAction.actionIndex = -1;
        break;

    case BattleState::OPPONENT_ATTACK:
        if (!this->commitOpponentAction()) return;
        break;
    case BattleState::PLAYER_RECEIVE_DAMAGE:
        if (!this->commitOpponentAction()) return;
        break;
    case BattleState::PLAYER_RECEIVE_EFFECT_APPLICATION:
        if (!this->commitOpponentAction()) return;
        opponentAction.actionIndex = -1;
        break;
    case BattleState::END_TURN:
        runtTickEffects();
        updateState = true;
        break;

    default:
        break;
    }
    if (updateState) {
        updateFightState();
        updateState = false;
    }
}

bool BattleEngine::checkLoss() {
    if (player.creatureHPs[0] <= 0 && player.creatureHPs[1] <= 0 &&
        player.creatureHPs[2] <= 0) {
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
    if (player.creatureHPs[this->playerIndex] <= 0) {
        // BUG: can backout out of change menu if creature is down
        dialogMenu.pushMenu(newDialogBox(FAINT, playerCur->id, 0));
        // battleEventPlayer.push({BattleEventType::FAINT, playerCur->id, 0});
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
        // battleEventPlayer.push({BattleEventType::OPPONENT_FAINT, opponentCur->id, 0});
        if (!checkWin()) {
            this->opponentAction.setActionType(ActionType::CHNGE, Priority::FAST);
            this->opponentAction.actionIndex = int8_t(this->opponentIndex + 1);
        }
        return true;
    }
    return false;
}

bool BattleEngine::commitPlayerAction() {
    // TODO: If changing creature should skip all this
    EffectResults effectRes = runTurnEffect(playerCur);

    Creature *target = opponentCur;
    bool skip = false;
    switch (effectRes) {
    case EffectResults::TARGET_SELF:
        target = playerCur;
        // TODO: add move specific for self damage
        break;
    case EffectResults::SKIP_TURN:
        skip = true;
    }
    if (!skip) {
        this->commitAction(&this->playerAction, this->opponentCur, target, true);
    }
    if (!this->activeBattle) {
        return false;
    }
    if (turnState == BattleState::OPPONENT_RECEIVE_DAMAGE && checkOpponentFaint()) {
        playerAction.setActionType(ActionType::SKIP, Priority::NORMAL);
    }
    return true;
}

bool BattleEngine::commitOpponentAction() {

    EffectResults effectRes = runTurnEffect(playerCur);

    Creature *target = playerCur;
    bool skip = false;
    switch (effectRes) {
    case EffectResults::TARGET_SELF:
        target = opponentCur;
        // TODO: add move specific for self damage
        break;
    case EffectResults::SKIP_TURN:
        skip = true;
    }

    if (!skip) {
        this->commitAction(&this->opponentAction, this->opponentCur, target, false);
    }
    if (!this->activeBattle) {
        return false;
    }
    if (turnState == BattleState::PLAYER_RECEIVE_DAMAGE && checkPlayerFaint()) {
        opponentAction.setActionType(ActionType::SKIP, Priority::NORMAL);
    }
    return true;
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
    if (!this->activeBattle) {
        return;
    }

    // The session owns the active mode now. This legacy object is retained
    // only for staged host compatibility until jp8.3.12 removes it.
    this->activeBattle = false;
    this->updateState = false;
    this->playerAction.actionIndex = -1;
    this->opponentAction.actionIndex = -1;

    gameState.state = GameState_t::WORLD;
    menu.clear();
    menuStack.clear();
    dialogMenu.clear();
    // No union transition here: only BattleSession owns the active mode.
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
    this->opponentAction.actionIndex = aiChooseStrongestMove();
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
            Effect moveEffect = move.getMoveEffect();
            runEffect(commiter, receiver, moveEffect);
            if (isPlayer) {
                // battleEventPlayer.push({BattleEventType::ATTACK, commiter->id, commiter->moves[action->actionIndex]});
                pushBattleDialog(NAME, commiter->id, commiter->moves[action->actionIndex]);

            } else {
                // battleEventPlayer.push({BattleEventType::OPPONENT_ATTACK, commiter->id, commiter->moves[action->actionIndex]});
                pushBattleDialog(ENEMY_NAME, commiter->id, commiter->moves[action->actionIndex]);
            }

        } else if (turnState == BattleState::PLAYER_RECEIVE_DAMAGE || turnState == BattleState::OPPONENT_RECEIVE_DAMAGE) {
            if (mod != Modifier::Same) {
                pushBattleDialog(EFFECTIVENESS, uint24_t(mod), 0);
            }
            applyDamage(damage, receiver);

            if (isPlayer) {
                pushBattleDialog(ENEMY_DAMAGE, receiver->id, damage);
                //  battleEventPlayer.push({BattleEventType::OPPONENT_DAMAGE, 0, damage});

            } else {
                //    battleEventPlayer.push({BattleEventType::DAMAGE, 0, damage});
                pushBattleDialog(DAMAGE, receiver->id, damage);
            }
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
                pushBattleDialog(TEAM_CHANGE, 0, 0);
                pushBattleDialog(SWITCH, playerParty[action->actionIndex]->id, 0);
            } else {
                pushBattleDialog(SWITCH, opponent.party[action->actionIndex].id, 0);
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

            pushBattleDialog(ESCAPE_ENCOUNTER, 0, 0);
        } else {
            this->endEncounter();
            return;
        }

        break;

    default:
        break;
    }
    updateState = true;
}

void BattleEngine::applyDamage(uint16_t damage, Creature *receiver) {
    if (receiver == this->playerCur) {
        const uint8_t hp = player.creatureHPs[this->playerIndex];
        const uint8_t next = damage >= hp
            ? 0
            : static_cast<uint8_t>(hp - damage);
        player.creatureHPs[this->playerIndex] = next;
    } else {
        uint16_t hp = this->opponentHealths[this->opponentIndex];
        this->opponentHealths[this->opponentIndex] =
            damage >= hp ? 0 : static_cast<uint16_t>(hp - damage);
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

void BattleEngine::applyEffect(Creature *target, Effect effect) {
    if (!isStatEffect(effect)) {
        bool applied = target->status.applyEffect(effect);

        if (!applied)
            return;
        DialogType dt = DialogType::PLAYER_EFFECT;
        if (target == opponentCur) {
            dt = DialogType::ENEMY_EFFECT;
        }
        dialogMenu.pushMenu(newDialogBox(dt, target->id, uint24_t(effect)));
        return;
    }
    applyBattleEffect(target, effect);
}

// TODO: apply the effects
void BattleEngine::applyBattleEffect(Creature *target, Effect &effect) {
    DialogType dt = DialogType::PLAYER_EFFECT;
    if (target == opponentCur) {
        dt = DialogType::ENEMY_EFFECT;
    }

    int8_t amount = 0;
    StatType targetStat = StatType::NONE;

    switch (effect) {
        // case Effect::NONE:
        //     return;
        // case Effect::DPRSD:
        // case Effect::SOAKED:
        // case Effect::BUFTD:
        // case Effect::SOILED:
        // case Effect::SCRCHD:
        // case Effect::ZAPPED:
        // case Effect::TANGLD:
        // case Effect::REDCD:
        // case Effect::ENLTND:
        // case Effect::DRNCHD:
        // case Effect::AIRSWPT:
        // case Effect::GRNDED:
        // case Effect::KINDLD:
        // case Effect::CHRGD:
        // case Effect::ENRCHD:
        // case Effect::EVOLVD:
        // case Effect::SAPPD:
        // case Effect::INFSED:
        //     target->status.applyEffect(effect);
        //     break;

    case Effect::ATKDWN:
        targetStat = StatType::ATTACK_M;
        amount = -1;
        break;
    case Effect::DEFDWN:
        targetStat = StatType::DEFENSE_M;
        amount = -1;
        break;
    case Effect::SPCADWN:
        targetStat = StatType::SPECIAL_ATTACK_M;
        amount = -1;
        break;
    case Effect::SPCDDWN:
        targetStat = StatType::SPECIAL_DEFENSE_M;
        amount = -1;
        break;
    case Effect::SPDDWN:
        targetStat = StatType::SPEED_M;
        amount = -1;
        break;

    case Effect::ATKUP:
        targetStat = StatType::ATTACK_M;
        amount = 1;
        break;
    case Effect::DEFUP:
        targetStat = StatType::DEFENSE_M;
        amount = 1;
        break;
    case Effect::SPCAUP:
        targetStat = StatType::SPECIAL_ATTACK_M;
        amount = 1;
        break;
    case Effect::SPCDUP:
        targetStat = StatType::SPECIAL_DEFENSE_M;
        amount = 1;
        break;
    case Effect::SPDUP:
        targetStat = StatType::SPEED_M;
        amount = 1;
        break;
    }

    target->statMods.incrementModifier(targetStat, amount);

    dialogMenu.pushMenu(newDialogBox(dt, target->id, uint24_t(effect)));
}

void BattleEngine::runEffect(Creature *commiter, Creature *other, Effect &effect) {
    if (effect == Effect::NONE) {
        return;
    }

    // TODO: not sure if this is right
    // if (!isTickEffect(effect)) {
    //     applyEffect(other, effect);
    //     return;
    //

    uint8_t rate = getEffectRateFX(static_cast<uint8_t>(effect));
    uint8_t roll = randomRoll(1, 100);   // random(1, 100);
    if (roll > rate) {
        return;
    }
    // TODO: need to base this off the move or mabye handle elsewhere
    bool selfTarget = isSelfEffect(effect);
    Creature *target = selfTarget ? commiter : other;
    applyEffect(target, effect);
}

void BattleEngine::runtTickEffects() {
    for (uint8_t i = 0; i < 2; i++) {
        tickEffects(playerCur, playerCur->status.effects[i]);
        tickEffects(opponentCur, opponentCur->status.effects[i]);
    }
}

void BattleEngine::tickEffects(Creature *target, Effect &effect) {
    if (effect == Effect::NONE) {
        return;
    }
    if (effect == Effect::SAPPD || effect == Effect::INFSED) {
        int16_t hp16th = target->statlist.hp >> 4;
        if (effect == Effect::SAPPD) {
            hp16th *= -1;
        }
        bool playerSide = target == playerCur;
        if (playerSide) {
            int16_t next = static_cast<int16_t>(player.creatureHPs[playerIndex]) + hp16th;
            if (next > playerCur->statlist.hp) {
                next = playerCur->statlist.hp;
            } else if (next < 0) {
                next = 0;
            }
            player.creatureHPs[playerIndex] = static_cast<uint8_t>(next);
        } else {
            int16_t next = static_cast<int16_t>(opponentHealths[opponentIndex]) + hp16th;
            if (next > opponentCur->statlist.hp) {
                next = opponentCur->statlist.hp;
            } else if (next < 0) {
                next = 0;
            }
            opponentHealths[opponentIndex] = static_cast<uint16_t>(next);
        }
    }
}

EffectResults BattleEngine::runTurnEffect(Creature *committer) {
    for (auto e : committer->status.effects) {
        switch (e) {
        case Effect::CONCUSED: {
            if (randomRoll(0, 4, 1)) {
                return EffectResults::TARGET_SELF;
            }
            break;
        }
        case Effect::PINNED: {
            if (randomRoll(0, 3, 1)) {
                return EffectResults::SKIP_TURN;
            }
            break;
        }

        default:
            break;
        }
    }

    return EffectResults::NO_EFFECT;
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
