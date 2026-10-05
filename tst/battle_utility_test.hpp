#pragma once
#include "test.hpp"
#include "../src/engine/battle/MoveUses.hpp"
#include "../src/engine/battle/Effects.hpp"
#include "../src/engine/battle/Resolve.hpp"
#include "../src/engine/battle/BattleSetup.hpp"
#include "../src/engine/battle/BattleSession.hpp"
#include "../src/engine/battle/Ai.hpp"
#include "../src/lib/MoveIds.hpp"
#include "../src/player/Player.hpp"

extern Player player;
namespace battle_utility_test_detail {
inline uint8_t zeroRoll(uint8_t) { return 0; }
inline Move utilityMove(uint8_t power, Effect effect = Effect::NONE) {
    Move result(MoveBitSet{static_cast<uint8_t>(Type::SPIRIT), power, 1, 0, 0});
    result.effect1 = effect;
    return result;
}
inline battle::BattleState fixture() {
    battle::BattleState state = {};
    for (uint8_t side = 0; side < 2; ++side) {
        auto &actor = state.active[side];
        actor.hp = actor.maxHp = 200;
        actor.types = DualType(Type::SPIRIT, Type::NONE);
        actor.stats.attack = actor.stats.spcAtk = 1;
        actor.stats.defense = actor.stats.spcDef = 100;
        for (uint8_t slot = 0; slot < 4; ++slot) {
            actor.moveIds[slot] = slot;
            actor.moves[slot] = utilityMove(slot == 0 ? 10 : 5);
        }
        actor.moves[2] = utilityMove(0, Effect::ATKUP);
        actor.moves[3] = utilityMove(5, Effect::PINNED);
        state.partyCount[side] = 3;
    }
    return state;
}
inline void prepareParty() {
    player = Player();
    for (uint8_t slot = 0; slot < PARTY_SIZE; ++slot) {
        player.loadCreature(slot, slot + 1);
        player.creatureHPs[slot] = 40;
    }
}
}

inline void BattleUtilityUsesTest(TestSuite &suite) {
    using namespace battle;
    using namespace battle_utility_test_detail;
    Test test(__func__);
    auto state = fixture();
    Rng rng = {zeroRoll};
    ActionResult out;
    test.assert(remainingMoveUses(state, Side::Player, 0), 2, "strong attack starts with two uses");
    test.assert(remainingMoveUses(state, Side::Player, 1), 255, "basic attack unlimited");
    test.assert(remainingMoveUses(state, Side::Player, 2), 3, "pure buff has three uses");
    test.assert(remainingMoveUses(state, Side::Player, 3), 2, "damaging control has two uses");
    state.active[0].status.applyEffect(Effect::PINNED);
    resolveAction(state, Side::Player, {ActionKind::Attack, 0}, rng, out);
    test.assert((out.flags & STATUS_SKIPPED) != 0, true, "pinned skips action");
    test.assert(remainingMoveUses(state, Side::Player, 0), 2, "pinned costs no use");
    state.active[0].status.clearEffects();
    state.active[0].status.applyEffect(Effect::CONCUSED);
    resolveAction(state, Side::Player, {ActionKind::Attack, 0}, rng, out);
    test.assert((out.flags & SELF_HIT) != 0, true, "confusion self hit executes");
    test.assert(remainingMoveUses(state, Side::Player, 0), 2, "self hit costs no use");
    state.active[0].status.clearEffects();
    state.active[0].moves[0] = Move(MoveBitSet{static_cast<uint8_t>(Type::FIRE), 10, 1, 0, 0});
    state.active[1].types = DualType(Type::WATER, Type::NONE);
    const uint8_t before = state.active[1].hp;
    resolveAction(state, Side::Player, {ActionKind::Attack, 0}, rng, out);
    test.assert(state.active[1].hp, before, "immune target takes no damage");
    test.assert(remainingMoveUses(state, Side::Player, 0), 1, "immunity spends use");
    resolveAction(state, Side::Player, {ActionKind::Attack, 0}, rng, out);
    test.assert(remainingMoveUses(state, Side::Player, 0), 0, "second execution exhausts move");
    resolveAction(state, Side::Player, {ActionKind::Attack, 0}, rng, out);
    test.assert((out.flags & REFUSED) != 0, true, "exhausted move refused");
    for (uint8_t count = 0; count < 5; ++count)
        resolveAction(state, Side::Player, {ActionKind::Attack, 1}, rng, out);
    test.assert(remainingMoveUses(state, Side::Player, 1), 255, "basic stays unlimited after execution");
    resolveAction(state, Side::Player, {ActionKind::Attack, 2}, rng, out);
    test.assert(remainingMoveUses(state, Side::Player, 2), 2, "move slots spend independently");
    test.assert(state.active[0].statMods.getModifier(StatType::ATTACK_M), 1, "pure buff executes its self effect");
    test.assert(remainingMoveUses(state, Side::Opponent, 0), 2, "opponent uses independent");
    state.activeSlot[0] = 2;
    test.assert(remainingMoveUses(state, Side::Player, 0), 2, "original party slot owns uses");
    spendMoveUse(state, Side::Player, 0);
    state.activeSlot[0] = 0;
    test.assert(remainingMoveUses(state, Side::Player, 0), 0, "original spent bank restored");
    resetMoveUses(state);
    for (uint8_t side = 0; side < 2; ++side)
        for (uint8_t slot = 0; slot < PARTY_SIZE; ++slot)
            test.assert(state.moveUsesSpent[side][slot], 0, "reset clears all banks");
    suite.addTest(test);
}

inline void BattleUtilitySwitchAndMenuTest(TestSuite &suite) {
    using namespace battle;
    using namespace battle_utility_test_detail;
    Test test(__func__);
    prepareParty();
    battle::BattleState state;
    beginWild(state, 4, 10, false, 1);
    state.active[0].moves[0] = utilityMove(10);
    state.active[0].moveIds[0] = 1;
    spendMoveUse(state, Side::Player, 0);
    Consequence fact;
    applyEffect(state, Side::Player, Effect::ATKUP, fact);
    ActionResult out;
    test.assert(applySwitch(state, Side::Player, 2, false, out), true, "switch to original slot two");
    test.assert(state.active[0].statMods.getModifier(StatType::ATTACK_M), 0, "reserve has independent stages");
    applyEffect(state, Side::Player, Effect::DEFDWN, fact);
    test.assert(applySwitch(state, Side::Player, 0, false, out), true, "return to original slot zero");
    state.active[0].moves[0] = utilityMove(10);
    state.active[0].moveIds[0] = 1;
    test.assert(remainingMoveUses(state, Side::Player, 0), 1, "switch retains spent use");
    test.assert(state.active[0].statMods.getModifier(StatType::ATTACK_M), 1, "switch retains attack buff");
    test.assert(state.active[0].statMods.getModifier(StatType::DEFENSE_M), 0, "reserve debuff does not leak");
    test.assert(applySwitch(state, Side::Player, 2, false, out), true, "second reserve switch");
    test.assert(state.active[0].statMods.getModifier(StatType::DEFENSE_M), -1, "reserve debuff persists");
    beginWild(state, 4, 10, false, 1);
    test.assert(state.active[0].statMods.getModifier(StatType::ATTACK_M), 0, "new battle clears active stage");
    for (uint8_t side = 0; side < 2; ++side)
        for (uint8_t slot = 0; slot < PARTY_SIZE; ++slot) {
            test.assert(state.moveUsesSpent[side][slot], 0, "battle start refills every use bank");
            test.assert(state.partyModifiers[side][slot], 0, "battle start resets every stage bank");
        }
    BattleSession session;
    session.beginWild(4, 10, false, 1);
    auto &live = session.stateForTest();
    live.active[0].moves[0] = utilityMove(10);
    live.active[0].moveIds[0] = 1;
    spendMoveUse(live, Side::Player, 0);
    spendMoveUse(live, Side::Player, 0);
    test.assert(session.moves().remainingUses[0], 0, "menu snapshot reports exhaustion");
    test.assert(session.submitIntent({MenuIntentKind::SelectMove, 0}), false, "menu rejects exhausted selection");
    test.assert(session.awaitingPlayer(), true, "refused selection keeps choice open");
    for (uint8_t slot = 1; slot < 4; ++slot) live.active[0].moveIds[slot] = 255;
    test.assert(session.submitIntent({MenuIntentKind::Pass, 0}), true, "fully exhausted loadout can pass");
    test.assert(session.awaitingPlayer(), false, "pass submits a turn");
    session.beginWild(4, 10, false, 1);
    auto &fresh = session.stateForTest();
    fresh.active[0].moveIds[0] = 1;
    fresh.active[0].moves[0] = utilityMove(5);
    for (uint8_t slot = 1; slot < 4; ++slot) fresh.active[0].moveIds[slot] = 255;
    test.assert(session.submitIntent({MenuIntentKind::Pass, 0}), false, "available basic attack prevents pass");
    test.assert(session.awaitingPlayer(), true, "illegal pass keeps choice open");
    suite.addTest(test);
}

inline void BattleUtilityEffectsTest(TestSuite &suite) {
    using namespace battle;
    using namespace battle_utility_test_detail;
    Test test(__func__);
    auto state = fixture();
    Consequence fact;
    for (uint8_t count = 0; count < 5; ++count) applyEffect(state, Side::Player, Effect::ATKUP, fact);
    test.assert(state.active[0].statMods.getModifier(StatType::ATTACK_M), 2, "buff capped plus two");
    for (uint8_t count = 0; count < 8; ++count) applyEffect(state, Side::Player, Effect::ATKDWN, fact);
    test.assert(state.active[0].statMods.getModifier(StatType::ATTACK_M), -2, "debuff capped minus two");
    state.active[0].maxHp = 80;
    state.active[0].hp = 20;
    test.assert(applyEffect(state, Side::Player, Effect::INFSED, fact), true, "regen applies");
    test.assert(applyEffect(state, Side::Player, Effect::INFSED, fact), false, "duplicate regen refused");
    test.assert(state.active[0].status.effects[1], Effect::NONE, "duplicate cannot occupy second slot");
    test.assert(applyEffect(state, Side::Player, Effect::PINNED, fact), true, "control occupies other slot");
    ActionResult out;
    for (uint8_t turn = 1; turn <= 3; ++turn) {
        resetActionResult(out);
        tickEffects(state, out);
        test.assert(state.active[0].hp, static_cast<uint8_t>(20 + turn * 10), "regen heals eighth maximum each tick");
        if (turn < 3) test.assert(state.active[0].status.effects[0], Effect::INFSED, "regen stays through first two ticks");
        if (turn == 1) test.assert(applyEffect(state, Side::Player, Effect::INFSED, fact), false, "duplicate does not refresh duration");
    }
    test.assert(state.active[0].status.effects[0], Effect::NONE, "regen expires third tick");
    test.assert(state.active[0].status.effects[1], Effect::NONE, "control expires third tick");
    tickEffects(state, out);
    test.assert(state.active[0].hp, 50, "expired regen gives no fourth heal");
    applyEffect(state, Side::Player, Effect::CONCUSED, fact);
    for (uint8_t turn = 0; turn < 3; ++turn) tickEffects(state, out);
    test.assert(state.active[0].status.effects[0], Effect::NONE, "confusion expires third tick");
    state.active[0].hp = 79;
    applyEffect(state, Side::Player, Effect::INFSED, fact);
    tickEffects(state, out);
    test.assert(state.active[0].hp, 80, "regen caps at maximum HP");
    test.assert(combineModifier(Modifier::Quarter, Modifier::Same), Modifier::Quarter, "neutral preserves quarter");
    test.assert(combineModifier(Modifier::Quadruple, Modifier::Same), Modifier::Quadruple, "neutral preserves quadruple");
    test.assert(combineModifier(Modifier::Quarter, Modifier::Quadruple), Modifier::Same, "quarter cancels quadruple");
    test.assert(battleMoveId(32), 255, "legacy empty maps to runtime empty");
    test.assert(moveRecordIndex(44), 32, "Deluge maps original row");
    for (uint8_t id = 36; id <= 43; ++id) test.assert(moveRecordIndex(id), static_cast<uint8_t>(id - 1), "utility maps authored gap");
    suite.addTest(test);
}

inline void BattleUtilityTacticalAiTest(TestSuite &suite) {
    using namespace battle;
    using namespace battle_utility_test_detail;
    Test test(__func__);
    auto state = fixture();
    for (uint8_t side = 0; side < 2; ++side) {
        state.active[side].moves[0] = utilityMove(5);
        state.active[side].moves[1] = utilityMove(0, Effect::ATKUP);
        state.active[side].moveIds[2] = state.active[side].moveIds[3] = 255;
    }
    test.assert(chooseDamageAction(state, Side::Player).index, 0, "damage policy ignores setup");
    test.assert(chooseAction(state, Side::Player).index, 1, "tactical policy selects compatible physical buff");
    Rng rng = {zeroRoll};
    ActionResult out;
    resolveAction(state, Side::Player, chooseAction(state, Side::Player), rng, out);
    test.assert(chooseAction(state, Side::Player).index, 0, "tactical attacks after one buff");
    state.active[0].statMods.clearModifiers();
    state.active[1].hp = 1;
    test.assert(chooseAction(state, Side::Player).index, 0, "lethal attack takes priority over buff");
    state.active[1].hp = 200;
    state.active[0].moves[1] = utilityMove(0, Effect::SPCAUP);
    test.assert(chooseAction(state, Side::Player).index, 0, "special buff ignored for physical attacker");
    state.active[0].moves[0] = Move(MoveBitSet{static_cast<uint8_t>(Type::SPIRIT), 5, 0, 0, 0});
    test.assert(chooseAction(state, Side::Player).index, 1, "special attacker chooses special buff");
    for (uint8_t count = 0; count < 3; ++count) spendMoveUse(state, Side::Player, 1);
    test.assert(chooseAction(state, Side::Player).index, 0, "exhausted buff ignored");
    resetMoveUses(state);
    state.active[0].moves[1] = utilityMove(0, Effect::INFSED);
    state.active[0].hp = 90;
    test.assert(chooseAction(state, Side::Player).index, 1, "injured creature chooses regen");
    test.assert(chooseDamageAction(state, Side::Player).index, 0, "damage policy still attacks when injured");
    Consequence fact;
    applyEffect(state, Side::Player, Effect::INFSED, fact);
    test.assert(chooseAction(state, Side::Player).index, 0, "existing regen not reapplied");
    state.active[0].status.clearEffects();
    state.active[0].effectTurns = 0;
    spendMoveUse(state, Side::Player, 1);
    spendMoveUse(state, Side::Player, 1);
    test.assert(chooseAction(state, Side::Player).index, 0, "exhausted healing ignored");
    resetMoveUses(state);
    state.active[0].hp = 200;
    test.assert(chooseAction(state, Side::Player).index, 0, "healthy creature does not heal");
    state.active[0].moves[1] = utilityMove(0, Effect::DEFUP);
    test.assert(chooseAction(state, Side::Player).index, 1, "Ironbody useful against physical best attack");
    state.active[1].moves[0] = Move(MoveBitSet{static_cast<uint8_t>(Type::SPIRIT), 5, 0, 0, 0});
    test.assert(chooseAction(state, Side::Player).index, 0, "Ironbody ignored against special best attack");
    state.active[0].moves[1] = utilityMove(0, Effect::SPDDWN);
    state.active[0].stats.speed = 10;
    state.active[1].stats.speed = 12;
    test.assert(chooseAction(state, Side::Player).index, 1, "slower creature chooses Sweep");
    applyEffect(state, Side::Opponent, Effect::SPDDWN, fact);
    test.assert(chooseAction(state, Side::Player).index, 0, "Sweep not repeated on debuffed opponent");
    state.active[1].statMods.clearModifiers();
    state.active[0].stats.speed = 20;
    test.assert(chooseAction(state, Side::Player).index, 0, "already faster creature attacks");
    suite.addTest(test);
}

inline void BattleUtilitySuite(TestRunner &runner) {
    TestSuite suite("Battle utility and move uses");
    BattleUtilityUsesTest(suite);
    BattleUtilitySwitchAndMenuTest(suite);
    BattleUtilityEffectsTest(suite);
    BattleUtilityTacticalAiTest(suite);
    runner.addTestSuite(suite);
}
