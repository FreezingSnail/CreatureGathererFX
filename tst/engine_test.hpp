#pragma once
#include "test.hpp"

#include "../src/lib/ReadData.hpp"
#include "../src/engine/battle/Battle.hpp"
#include "../src/globals.hpp"
#include "src/helpers.hpp"

void reset() {
    player = Player();
    battleEventPlayer = BattleEventPlayer();
    menuStack = MenuStack();
    dialogMenu = DialogMenu();
}

void EngineStartTest(TestSuite &t) {
    Test test = Test(__func__);
    BattleEngine eng = BattleEngine();
    eng.startFight(0);
    test.assert(eng.activeBattle, true, "Active Battle");
    test.assert(eng.playerAction.actionIndex, -1, "Player Action Index not set");

    t.addTest(test);
}

void EngineQueueActionTest(TestSuite &t) {
    reset();
    Test test = Test(__func__);
    BattleEngine eng = BattleEngine();
    eng.startFight(0);
    eng.queueAction(ActionType::ATTACK, 0);
    player.basic();
    eng.opponentCur->level = 31;
    eng.opponentHealths[0] = 1000;
    test.assert(eng.PlayerActionReady(), true, "Action Queued");
    test.assert(eng.playerAction.actionIndex, 0, "Action Index set");
    test.assert(eng.playerAction.actionType, ActionType::ATTACK, "Action Type set");
    test.assert(eng.playerAction.priority, Priority::NORMAL, "Action priority set");

    t.addTest(test);
}

void EngineTickTest(TestSuite &t) {
    reset();
    Test test = Test(__func__);
    BattleEngine eng = BattleEngine();
    player.basic();
    eng.startFight(0);
    test.assert(battleEventPlayer.stackPointer, -1, "battleEventPlayer.stackPointer Pointer empty");
    test.addToLog("Healths: " + std::to_string(eng.playerHealths[0]) + " " + std::to_string(eng.opponentHealths[0]));
    // TODO: shoudnt need to bump opponent for this test
    eng.opponentCur->level = 31;
    eng.opponentHealths[0] = 1000;
    test.addToLog("Healths: " + std::to_string(eng.playerHealths[0]) + " " + std::to_string(eng.opponentHealths[0]));

    auto playerHP = eng.playerHealths[0];
    auto opponentHP = eng.opponentHealths[0];

    test.assert(eng.activeBattle, true, "Active Battle");
    test.assert(eng.playerAction.actionIndex, -1, "Player Action Index not set");

    eng.queueAction(ActionType::ATTACK, 0);

    eng.turnState = BattleState::TURN_INPUT;
    eng.turnTick();

    eng.turnState = BattleState::PLAYER_ATTACK;
    eng.turnTick();
    test.addToLog("battleEventPlayer.stackPointer: " + std::to_string(battleEventPlayer.stackPointer));
    for (int i = 0; i <= battleEventPlayer.stackPointer; i++) {
        test.addToLog("battleEventStack " + std::to_string(i) + " Type: " + BattleEventTypeToString(battleEventStack[i].type));
        test.addToLog("battleEventStack " + std::to_string(i) + " Index: " + std::to_string(static_cast<int>(battleEventStack[i].index)));
        test.addToLog("battleEventStack " + std::to_string(i) + " Data: " + std::to_string(static_cast<int>(battleEventStack[i].data)));
    }
    // test.assert(battleEventPlayer.stackPointer, 0, "battleEventPlayer.stackPointer Pointer set");
    // test.assert(battleEventStack[0].type, BattleEventType::ATTACK, "battleEventStack 0 Type set");
    // test.assert(battleEventStack[0].index, eng.playerCur->id, "battleEventStack 0 index set");

    eng.turnState = BattleState::OPPONENT_RECEIVE_DAMAGE;
    eng.turnTick();

    // TODO: Figure out if using the battle stack rewrite
    // test.assert(battleEventStack[1].type, BattleEventType::OPPONENT_DAMAGE, "battleEventStack 1 Type set");
    // test.assert(battleEventStack[1].index, 0, "DibattleEventStackaaddToLog 1 index set");
    // test.assertGreaterThan(battleEventStack[1].data, 0, "battleEventStack 1 data set");

    eng.turnState = BattleState::OPPONENT_RECEIVE_EFFECT_APPLICATION;
    eng.turnTick();

    eng.turnState = BattleState::OPPONENT_ATTACK;
    eng.turnTick();
    // test.assert(battleEventStack[2].type, BattleEventType::OPPONENT_ATTACK, "battleEventStack 2 Type set");
    // test.assert(battleEventStack[2].index, eng.opponentCur->id, "battleEventStack 2 index set");

    eng.turnState = BattleState::PLAYER_RECEIVE_DAMAGE;
    eng.turnTick();
    // test.assert(battleEventStack[3].type, BattleEventType::DAMAGE, "battleEventStack 3 Type set");
    // test.assert(battleEventStack[3].index, 0, "battleEventStack 3 index set");
    // test.assertGreaterThan(battleEventStack[3].data, 0, "battleEventStack 3 data set");

    eng.turnState = BattleState::PLAYER_RECEIVE_EFFECT_APPLICATION;
    eng.turnTick();

    eng.turnState = BattleState::END_TURN;
    eng.turnTick();

    // test.assert(eng.playerHealths[0], playerHP - battleEventStack[3].data, "Player Health set");
    // test.assert(eng.opponentHealths[0], opponentHP - battleEventStack[1].data, "Opponent Health set");

    t.addTest(test);
}

void EngineTypeStatusMod(TestSuite &t) {
    Test test = Test(__func__);
    BattleEngine eng = BattleEngine();
    Creature c1 = basicCreatureSquibbleSnail();
    Creature c2 = basicCreatureSquibbleSnail();
    c1.types = DualType(Type::WIND, Type::NONE);
    c2.types = DualType(Type::SPIRIT, Type::NONE);

    test.assert(c1.types.getType1(), Type::WIND, "Player Type set");
    test.assert(c2.types.getType1(), Type::SPIRIT, "Opponent Type set");
    test.assert(c1.status.effects[0], Effect::NONE, "no status effects");
    test.assert(c1.status.effects[1], Effect::NONE, "no status effects");
    test.assert(c2.status.effects[0], Effect::NONE, "no status effects");
    test.assert(c2.status.effects[1], Effect::NONE, "no status effects");
    Modifier committerEffectMod1 = typeEffectModifier(c1.status.effects[0], c1.types);
    test.assert(committerEffectMod1, Modifier::Same, "type 1 effect mod");
    Modifier committerEffectMod2 = typeEffectModifier(c1.status.effects[1], c1.types);
    test.assert(committerEffectMod2, Modifier::Same, "type 2 effect mod");
    Modifier committerEffectMod = combineModifier(committerEffectMod1, committerEffectMod2);
    test.assert(committerEffectMod, Modifier::Same, "combined effect mod");
    Modifier receiverEffectMod1 = typeEffectModifier(c2.status.effects[0], c2.types);
    Modifier receiverEffectMod2 = typeEffectModifier(c2.status.effects[1], c2.types);
    Modifier receiverEffectMod = combineModifier(receiverEffectMod1, receiverEffectMod2);
    test.assert(receiverEffectMod, Modifier::Same, "combined effect mod receiver");

    Modifier mod = eng.getTypeStatusModifier(&c1, &c2);
    test.assert(mod, Modifier::Same, "no type effects");

    committerEffectMod1 = typeEffectModifier(Effect::AIRSWPT, DualType(Type::WIND, Type::NONE));
    test.assert(committerEffectMod1, Modifier::Double, "AIRSWPT DUALTYPE raw effect mod");

    c1.status.effects[0] = Effect::AIRSWPT;
    committerEffectMod1 = typeEffectModifier(c1.status.effects[0], c1.types);
    test.assert(committerEffectMod1, Modifier::Double, "AIRSWPT type 1 effect mod");
    committerEffectMod2 = typeEffectModifier(c1.status.effects[1], c1.types);
    test.assert(committerEffectMod2, Modifier::Same, "AIRSWPT type 2 effect mod");
    committerEffectMod = combineModifier(committerEffectMod1, committerEffectMod2);
    test.assert(committerEffectMod, Modifier::Double, "AIRSWPT combined effect mod");

    mod = eng.getTypeStatusModifier(&c1, &c2);
    test.assert(mod, Modifier::Double, "Double type effects committer");

    c1.status.effects[0] = Effect::BUFTD;
    mod = eng.getTypeStatusModifier(&c1, &c2);
    test.assert(mod, Modifier::Half, "Halved type effects committer");

    c1.status.effects[0] = Effect::NONE;
    c2.status.effects[0] = Effect::ENLTND;
    mod = eng.getTypeStatusModifier(&c1, &c2);
    test.assert(mod, Modifier::Half, "Halfed type effects receiver");

    c1.status.effects[0] = Effect::NONE;
    c2.status.effects[0] = Effect::DPRSD;
    mod = eng.getTypeStatusModifier(&c1, &c2);
    test.assert(mod, Modifier::Double, "Doubled type effects receiver");

    c1.status.effects[0] = Effect::AIRSWPT;
    c2.status.effects[0] = Effect::DPRSD;
    mod = eng.getTypeStatusModifier(&c1, &c2);
    test.assert(mod, Modifier::Quadruple, "Doubled type effects receiver");

    c1.status.effects[0] = Effect::BUFTD;
    c2.status.effects[0] = Effect::DPRSD;
    mod = eng.getTypeStatusModifier(&c1, &c2);
    test.assert(mod, Modifier::Same, "Doubled type effects receiver");

    t.addTest(test);
}

void EngineDamageCalcTypeEffect(TestSuite &t) {
    reset();
    Test test = Test(__func__);
    BattleEngine eng = BattleEngine();
    player.basic();
    eng.startFight(0);

    eng.opponentCur->types = DualType(Type::SPIRIT, Type::NONE);
    test.assert(Type::WIND, eng.playerCur->types.getType1(), "Player Type set");
    test.assert(Type::SPIRIT, eng.opponentCur->types.getType1(), "Opponent Type set");

    Move move = Move({int(Type::WIND), 10, 1});
    eng.playerCur->moveList[0] = move;
    test.addToLog("move used power: " + std::to_string(move.getMovePower()));
    test.addToLog("move used type: " + std::to_string(move.getMoveType()));
    test.addToLog("move used isPhysical: " + std::to_string(move.isPhysical()));

    Action action = Action();
    action.actionType = ActionType::ATTACK;
    action.actionIndex = 0;
    action.priority = Priority::NORMAL;

    uint16_t damage = eng.calculateDamage(&action, eng.playerCur, eng.opponentCur);
    test.addToLog("calculated damage: " + std::to_string(damage));
    bool applied = eng.playerCur->status.applyEffect(Effect::AIRSWPT);
    test.assert(applied, true, "Status effect applied");
    test.assert(Effect::AIRSWPT, eng.playerCur->status.effects[0], "type Status effect set");
    uint16_t damage2 = eng.calculateDamage(&action, eng.playerCur, eng.opponentCur);
    test.addToLog("calculated damage atk up: " + std::to_string(damage2));
    test.assertGreaterThan(damage2, damage, "Damage increased greater than with type status effect committer");
    test.assert(damage2, damage * 2, "Damage doubled with type status effect committer");

    eng.playerCur->status.clearEffects();
    applied = eng.playerCur->status.applyEffect(Effect::BUFTD);
    test.assert(applied, true, "Status effect applied");
    test.assert(Effect::BUFTD, eng.playerCur->status.effects[0], "type Status effect set");
    damage2 = eng.calculateDamage(&action, eng.playerCur, eng.opponentCur);
    test.addToLog("calculated damage atk up: " + std::to_string(damage2));
    test.assert(damage2, damage / 2, "Damage halfed with type status effect committer");

    eng.playerCur->status.clearEffects();
    applied = eng.opponentCur->status.applyEffect(Effect::DPRSD);
    test.assert(applied, true, "Status effect applied");
    test.assert(Effect::DPRSD, eng.opponentCur->status.effects[0], "type Status effect set");
    damage2 = eng.calculateDamage(&action, eng.playerCur, eng.opponentCur);
    test.addToLog("calculated damage atk up: " + std::to_string(damage2));
    test.assert(damage2, damage * 2, "Damage Doubled with type status effect reciver");

    eng.opponentCur->status.clearEffects();
    applied = eng.opponentCur->status.applyEffect(Effect::ENLTND);
    test.assert(applied, true, "Status effect applied");
    test.assert(Effect::ENLTND, eng.opponentCur->status.effects[0], "type Status effect set");
    damage2 = eng.calculateDamage(&action, eng.playerCur, eng.opponentCur);
    test.addToLog("calculated damage atk up: " + std::to_string(damage2));
    test.assert(damage2, damage / 2, "Damage halfed with type status effect reciver");

    t.addTest(test);
}

void EngineDamageCalcStatStatusEffect(TestSuite &t) {
    reset();
    Test test = Test(__func__);
    BattleEngine eng = BattleEngine();
    player.basic();
    eng.startFight(0);

    test.addToLog("playerCurStats: " + std::to_string(eng.playerCur->statlist.attack));
    test.addToLog("opponentCurStats: " + std::to_string(eng.opponentCur->statlist.defense));
    test.addToLog("playerCurType: " + std::to_string(int(eng.playerCur->types.getType1())));
    test.addToLog("opponentCurType: " + std::to_string(int(eng.opponentCur->types.getType1())));

    eng.opponentCur->types = DualType(Type::SPIRIT, Type::NONE);
    test.assert(Type::WIND, eng.playerCur->types.getType1(), "Player Type set");
    test.assert(Type::SPIRIT, eng.opponentCur->types.getType1(), "Opponent Type set");

    Move move = Move({int(Type::WIND), 10, 1});
    eng.playerCur->moveList[0] = move;
    test.addToLog("move used power: " + std::to_string(move.getMovePower()));
    test.addToLog("move used type: " + std::to_string(move.getMoveType()));
    test.addToLog("move used isPhysical: " + std::to_string(move.isPhysical()));

    test.assert(move.getMovePower(), 10, "Move Power set");
    test.assert(move.getMoveType(), Type::WIND, "Move Type set");
    test.assert(move.isPhysical(), true, "Move isPhysical set");

    Action action = Action();
    action.actionType = ActionType::ATTACK;
    action.actionIndex = 0;
    action.priority = Priority::NORMAL;

    uint16_t damage = eng.calculateDamage(&action, eng.playerCur, eng.opponentCur);
    test.addToLog("calculated damage: " + std::to_string(damage));
    eng.playerCur->statMods.setModifier(StatType::ATTACK_M, 1);
    test.assert(eng.playerCur->statMods.getModifier(StatType::ATTACK_M), 1, "stat ATTACK_M effect +1");
    uint16_t damage2 = eng.calculateDamage(&action, eng.playerCur, eng.opponentCur);
    test.addToLog("calculated damage atk up: " + std::to_string(damage2));
    test.assert(damage2, damage * 2, "Damage doubled with stat atk up effect");
    eng.playerCur->statMods.clearModifiers();
    eng.opponentCur->statMods.clearModifiers();

    test.addToLog("calculated damage: " + std::to_string(damage));
    eng.playerCur->statMods.setModifier(StatType::ATTACK_M, -1);
    test.assert(eng.playerCur->statMods.getModifier(StatType::ATTACK_M), -1, "stat ATTACK_M effect -1");
    damage2 = eng.calculateDamage(&action, eng.playerCur, eng.opponentCur);
    test.addToLog("calculated damage atk up: " + std::to_string(damage2));
    test.assert(damage2, damage / 2, "Damage halved with stat attk down effect");
    eng.playerCur->statMods.clearModifiers();
    eng.opponentCur->statMods.clearModifiers();

    test.addToLog("calculated damage: " + std::to_string(damage));
    eng.opponentCur->statMods.setModifier(StatType::DEFENSE_M, -1);
    test.assert(eng.opponentCur->statMods.getModifier(StatType::DEFENSE_M), -1, "stat DEFENSE_M effect -1");
    damage2 = eng.calculateDamage(&action, eng.playerCur, eng.opponentCur);
    test.addToLog("calculated damage atk up: " + std::to_string(damage2));
    test.assert(damage2, damage * 2, "Damage doubled with stat def down effect");
    eng.playerCur->statMods.clearModifiers();
    eng.opponentCur->statMods.clearModifiers();

    test.addToLog("calculated damage: " + std::to_string(damage));
    eng.opponentCur->statMods.setModifier(StatType::DEFENSE_M, 1);
    test.assert(eng.opponentCur->statMods.getModifier(StatType::DEFENSE_M), 1, "stat DEFENSE_M effect +1");
    damage2 = eng.calculateDamage(&action, eng.playerCur, eng.opponentCur);
    test.addToLog("calculated damage atk up: " + std::to_string(damage2));
    test.assert(damage2, damage / 2, "Damage halfed with stat def up effect");
    eng.playerCur->statMods.clearModifiers();
    eng.opponentCur->statMods.clearModifiers();

    t.addTest(test);
}

void ApplyStatModEffects(TestSuite &t) {
    reset();
    Test test = Test(__func__);
    BattleEngine eng = BattleEngine();
    player.basic();
    eng.startFight(0);

    eng.applyEffect(eng.playerCur, Effect::ATKUP);
    test.assert(eng.playerCur->statMods.getModifier(StatType::ATTACK_M), 1, "stat ATTACK_M effect +1");

    eng.applyEffect(eng.playerCur, Effect::ATKUP);
    test.assert(eng.playerCur->statMods.getModifier(StatType::ATTACK_M), 2, "stat ATTACK_M effect +2");

    eng.applyEffect(eng.playerCur, Effect::ATKDWN);
    test.assert(eng.playerCur->statMods.getModifier(StatType::ATTACK_M), 1, "stat ATTACK_M effect lowered to +1");

    eng.applyEffect(eng.playerCur, Effect::SPCADWN);
    test.assert(eng.playerCur->statMods.getModifier(StatType::SPECIAL_ATTACK_M), -1, "stat SPECIAL_ATTACK_M effect set to -1");

    eng.applyEffect(eng.playerCur, Effect::SPCADWN);
    test.assert(eng.playerCur->statMods.getModifier(StatType::SPECIAL_ATTACK_M), -2, "stat SPECIAL_ATTACK_M effect lowered to -2");
    eng.applyEffect(eng.playerCur, Effect::SPCAUP);
    test.assert(eng.playerCur->statMods.getModifier(StatType::SPECIAL_ATTACK_M), -1, "stat SPECIAL_ATTACK_M effect raised to -1");
    eng.applyEffect(eng.playerCur, Effect::SPCAUP);
    test.assert(eng.playerCur->statMods.getModifier(StatType::SPECIAL_ATTACK_M), 0, "stat SPECIAL_ATTACK_M effect raised to 0");
    eng.applyEffect(eng.playerCur, Effect::SPCAUP);
    test.assert(eng.playerCur->statMods.getModifier(StatType::SPECIAL_ATTACK_M), 1, "stat SPECIAL_ATTACK_M effect raised to +1");
    eng.applyEffect(eng.playerCur, Effect::SPCAUP);
    test.assert(eng.playerCur->statMods.getModifier(StatType::SPECIAL_ATTACK_M), 2, "stat SPECIAL_ATTACK_M effect raised to +2");
    eng.applyEffect(eng.playerCur, Effect::SPCAUP);
    test.assert(eng.playerCur->statMods.getModifier(StatType::SPECIAL_ATTACK_M), 3, "stat SPECIAL_ATTACK_M effect raised to +3");
    eng.applyEffect(eng.playerCur, Effect::SPCAUP);
    test.assert(eng.playerCur->statMods.getModifier(StatType::SPECIAL_ATTACK_M), 3, "stat SPECIAL_ATTACK_M effect raised to +4");

    t.addTest(test);
}

void ApplyHPTickEffects(TestSuite &t) {
    reset();
    Test test = Test(__func__);
    BattleEngine eng = BattleEngine();
    player.basic();
    eng.startFight(0);
    eng.playerHealths[eng.playerIndex] = 100;

    eng.applyEffect(eng.playerCur, Effect::SAPPD);
    eng.runtTickEffects();
    int16_t targetHP = 100 - (eng.playerCur->statlist.hp / 16);
    test.assert(eng.playerHealths[eng.playerIndex], targetHP, "HP ticked down");

    eng.playerCur->status.clearEffects();

    eng.playerHealths[eng.playerIndex] = 10;

    eng.applyEffect(eng.playerCur, Effect::INFSED);
    targetHP = 10 + (eng.playerCur->statlist.hp / 16);
    test.addToLog(("targetHP for tick up: " + std::to_string(targetHP)));
    test.addToLog("playerHP for tick up: " + std::to_string(eng.playerHealths[eng.playerIndex]));
    eng.runtTickEffects();
    test.assert(eng.playerHealths[eng.playerIndex], targetHP, "HP ticked up");

    t.addTest(test);
}

void ChangeCreatureTest(TestSuite &t) {
    reset();
    Test test = Test(__func__);
    BattleEngine eng = BattleEngine();
    player.basic();
    eng.startFight(0);
    eng.queueAction(ActionType::CHNGE, 0);
    eng.playerParty[0]->statlist.speed = 255;
    eng.turnTick();
    eng.turnTick();
    eng.turnTick();
    eng.turnTick();
    eng.turnTick();
    test.assert(eng.playerIndex, 1, "Creature changed");

    t.addTest(test);
}

void EngineSuite(TestRunner &r) {
    TestSuite t = TestSuite("Engine Suite");
    EngineStartTest(t);
    EngineQueueActionTest(t);
    EngineTickTest(t);
    EngineTypeStatusMod(t);
    EngineDamageCalcStatStatusEffect(t);
    EngineDamageCalcTypeEffect(t);
    ApplyStatModEffects(t);
    ApplyHPTickEffects(t);
    ChangeCreatureTest(t);
    r.addTestSuite(t);
    reset();
}
