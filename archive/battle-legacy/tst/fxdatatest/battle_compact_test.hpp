#pragma once

#include "fxtest.hpp"
#include "src/engine/battle/BattleFeedback.hpp"
#include "src/engine/battle/BattleSession.hpp"
#include "src/engine/battle/MoveUses.hpp"
#include "src/engine/battle/Resolve.hpp"
#include "src/lib/FxReadCounter.hpp"

namespace battle_compact_test_detail {
inline uint8_t draws;

inline uint8_t roll(uint8_t bound) {
    ++draws;
    (void)bound;
    return 0;
}

inline battle::BattleState state() {
    battle::BattleState result = {};
    result.trainer = true;
    for (uint8_t side = 0; side < 2; ++side) {
        battle::Combatant &creature = result.active[side];
        creature.id = static_cast<uint8_t>(side + 1);
        creature.types = DualType(Type::SPIRIT, Type::NONE);
        creature.level = 10;
        creature.hp = creature.maxHp = 16;
        creature.stats.hp = 16;
        creature.stats.attack = 100;
        creature.stats.defense = 10;
        creature.stats.spcAtk = 100;
        creature.stats.spcDef = 10;
        creature.stats.speed = 10;
        for (uint8_t slot = 0; slot < 4; ++slot)
            creature.moveIds[slot] = 255;
        creature.moveIds[0] = 7;
        creature.moves[0] = Move(MoveBitSet{
            static_cast<uint8_t>(Type::SPIRIT), 10, 1, 0, 0});
        result.partyCount[side] = 1;
        result.activeSlot[side] = 0;
    }
    return result;
}

// Durable variant for repeated-execution PP checks: the shared outcome fixture
// can KO the single trainer creature on the first hit, which would make later
// executions vacuous. High defense and low attack keep both actors alive.
inline battle::BattleState durable() {
    battle::BattleState result = state();
    for (uint8_t side = 0; side < 2; ++side) {
        battle::Combatant &creature = result.active[side];
        creature.hp = creature.maxHp = creature.stats.hp = 200;
        creature.stats.attack = 1;
        creature.stats.spcAtk = 1;
        creature.stats.defense = 100;
        creature.stats.spcDef = 100;
    }
    return result;
}
} // namespace battle_compact_test_detail

// Move-use spending mechanics for the compact resolver: one packed charge per
// finite attack, untouched unlimited counters, refusal ahead of the status
// gate, and no charge for PINNED, confusion or Gather self-hits, all banked by
// the active original party slot.
inline void test_battle_compact_moveuses(FxTest &test) {
    using namespace battle;
    using namespace battle_compact_test_detail;
    Rng rng = {roll};
    ActionResult result = {};

    battle::BattleState stateUnderTest = durable();
    draws = 0;
    resolveAction(stateUnderTest, Side::Player,
                  {ActionKind::Attack, 0}, rng, result);
    test.expectEq(static_cast<uint8_t>(result.kind),
                  static_cast<uint8_t>(ResultKind::Attack),
                  F("finite attack executes"));
    test.expectEq((stateUnderTest.moveUsesSpent[0][0] >> 0) & 3, 1,
                  F("finite attack spends exactly one use"));
    test.expectEq(draws, 0, F("unstatused finite attack needs no gate RNG"));
    resolveAction(stateUnderTest, Side::Player,
                  {ActionKind::Attack, 0}, rng, result);
    test.expectEq(static_cast<uint8_t>(result.kind),
                  static_cast<uint8_t>(ResultKind::Attack),
                  F("second finite attack still executes"));
    test.expectEq((stateUnderTest.moveUsesSpent[0][0] >> 0) & 3, 2,
                  F("second finite attack spends the last use"));

    stateUnderTest = durable();
    stateUnderTest.active[0].moveIds[1] = 1;
    stateUnderTest.active[0].moves[1] = Move(MoveBitSet{
        static_cast<uint8_t>(Type::SPIRIT), 5, 1, 0, 0});
    for (uint8_t count = 0; count < 5; ++count)
        resolveAction(stateUnderTest, Side::Player,
                      {ActionKind::Attack, 1}, rng, result);
    test.expectEq(static_cast<uint8_t>(result.kind),
                  static_cast<uint8_t>(ResultKind::Attack),
                  F("unlimited attack executes each time"));
    test.expectEq(stateUnderTest.moveUsesSpent[0][0], 0,
                  F("unlimited move keeps packed uses zero"));
    test.expectEq(remainingMoveUses(stateUnderTest, Side::Player, 1),
                  UNLIMITED_MOVE_USES, F("unlimited move stays usable"));

    stateUnderTest = durable();
    stateUnderTest.moveUsesSpent[0][0] = 2;
    stateUnderTest.active[0].status.effects[0] = Effect::CONCUSED;
    draws = 0;
    resolveAction(stateUnderTest, Side::Player,
                  {ActionKind::Attack, 0}, rng, result);
    test.expectEq((result.flags & REFUSED) != 0, true,
                  F("exhausted move is refused"));
    test.expectEq(draws, 0, F("no-PP refusal precedes the confusion gate"));
    test.expectEq(stateUnderTest.moveUsesSpent[0][0], 2,
                  F("refused exhausted move spends nothing"));

    stateUnderTest = durable();
    stateUnderTest.active[0].status.effects[0] = Effect::PINNED;
    draws = 0;
    resolveAction(stateUnderTest, Side::Player,
                  {ActionKind::Attack, 0}, rng, result);
    test.expectEq((result.flags & STATUS_SKIPPED) != 0, true,
                  F("PINNED still skips the attack"));
    test.expectEq(draws, 1, F("PINNED consumes exactly one gate roll"));
    test.expectEq(stateUnderTest.moveUsesSpent[0][0], 0,
                  F("PINNED skip spends no use"));

    stateUnderTest = durable();
    stateUnderTest.active[0].status.effects[0] = Effect::CONCUSED;
    draws = 0;
    resolveAction(stateUnderTest, Side::Player,
                  {ActionKind::Attack, 0}, rng, result);
    test.expectEq((result.flags & SELF_HIT) != 0, true,
                  F("confusion self-hit resolves"));
    test.expectEq(draws, 1, F("confusion consumes exactly one gate roll"));
    test.expectEq(stateUnderTest.moveUsesSpent[0][0], 0,
                  F("confusion self-hit spends no use"));

    stateUnderTest = durable();
    stateUnderTest.trainer = false;
    stateUnderTest.gatherable = true;
    stateUnderTest.active[0].status.effects[0] = Effect::CONCUSED;
    stateUnderTest.moveUsesSpent[0][0] = 2;
    draws = 0;
    resolveAction(stateUnderTest, Side::Player,
                  {ActionKind::Gather, 255}, rng, result);
    test.expectEq((result.flags & SELF_HIT) != 0, true,
                  F("Gather confusion self-hits at exhausted slot zero"));
    test.expectEq(draws, 1, F("Gather self-hit consumes one gate roll"));
    test.expectEq(stateUnderTest.moveUsesSpent[0][0], 2,
                  F("Gather self-hit spends no PP"));

    stateUnderTest = durable();
    stateUnderTest.partyCount[0] = 3;
    stateUnderTest.activeSlot[0] = 2;
    stateUnderTest.moveUsesSpent[0][2] = 0;
    resolveAction(stateUnderTest, Side::Player,
                  {ActionKind::Attack, 0}, rng, result);
    test.expectEq(static_cast<uint8_t>(result.kind),
                  static_cast<uint8_t>(ResultKind::Attack),
                  F("original-slot spend attack executes"));
    test.expectEq((stateUnderTest.moveUsesSpent[0][2] >> 0) & 3, 1,
                  F("spend targets the active original slot"));
    test.expectEq(stateUnderTest.moveUsesSpent[0][0], 0,
                  F("stale slot-zero field keeps its bank"));
}

inline void test_battle_compact(FxTest &test) {
    using namespace battle;
    using namespace battle_compact_test_detail;
    static_assert(sizeof(ActionResult) == 7, "shipping result remains seven bytes");
    static_assert(sizeof(BattleSession) == 142, "compact session result removes 21 bytes");

    Rng rng = {roll};
    ActionResult result = {};
    battle::BattleState stateUnderTest = state();
    stateUnderTest.active[1].hp = 1;
    draws = 0;
    resolveAction(stateUnderTest, Side::Player,
                  {ActionKind::Attack, 0}, rng, result);
    test.expectEq(static_cast<uint8_t>(result.kind),
                  static_cast<uint8_t>(ResultKind::Attack),
                  F("attack result retained"));
    test.expectEq(result.hpBefore[0], 16, F("actor pre-action HP retained"));
    test.expectEq(result.hpBefore[1], 1, F("target pre-action HP retained"));
    test.expectEq((result.flags & OPPONENT_FAINTED) != 0, true,
                  F("lethal attack sets target faint flag"));
    test.expectEq(static_cast<uint8_t>(result.outcome),
                  static_cast<uint8_t>(Outcome::Win),
                  F("lethal attack retains terminal outcome"));
    test.expectEq(draws, 0, F("plain lethal attack adds no effect roll"));

    // Use a nonterminal attack for the action-specific actor and move rows;
    // the lethal result above returns early to its terminal caption.
    stateUnderTest = state();
    stateUnderTest.active[1].stats.defense = 255;
    resolveAction(stateUnderTest, Side::Player,
                  {ActionKind::Attack, 0}, rng, result);
    test.expectEq(static_cast<uint8_t>(result.outcome),
                  static_cast<uint8_t>(Outcome::None),
                  F("nonlethal result keeps action feedback"));

    // BattleFlow begins feedback before replacement, so the current actor
    // remains active while its name and move metadata are cached.
    BattleFeedback feedback;
    FxReadCounter::resetFrame();
    feedback.begin(result, stateUnderTest);
    test.expectEq(FxReadCounter::count(), 2,
                  F("attack feedback caches current actor and move names"));
    arduboy.clear();
    feedback.draw();
    bool drew = false;
    for (uint16_t at = 0; at < 1024; ++at)
        if (arduboy.getBuffer()[at]) drew = true;
    test.expectEq(drew, true, F("compact attack feedback remains visible"));

    stateUnderTest = state();
    stateUnderTest.active[0].hp = 1;
    stateUnderTest.active[1].hp = 1;
    stateUnderTest.active[0].maxHp = stateUnderTest.active[1].maxHp = 16;
    stateUnderTest.active[0].status.effects[0] = Effect::SAPPD;
    stateUnderTest.active[1].status.effects[0] = Effect::SAPPD;
    resolveEndTurn(stateUnderTest, rng, false, result);
    test.expectEq(stateUnderTest.active[0].hp, 0, F("player tick damage still applies"));
    test.expectEq(stateUnderTest.active[1].hp, 0, F("opponent tick damage still applies"));
    test.expectEq((result.flags & (PLAYER_FAINTED | OPPONENT_FAINTED)),
                  (PLAYER_FAINTED | OPPONENT_FAINTED),
                  F("both sequential tick KOs are retained"));
    test.expectEq(static_cast<uint8_t>(result.outcome),
                  static_cast<uint8_t>(Outcome::Lose),
                  F("player defeat keeps terminal priority"));
    test.expectEq(result.hpBefore[0], 1, F("tick keeps player pre-action HP"));
    test.expectEq(result.hpBefore[1], 1, F("tick keeps opponent pre-action HP"));

    stateUnderTest = state();
    stateUnderTest.trainer = false;
    resolveAction(stateUnderTest, Side::Player,
                  {ActionKind::Escape, 255}, rng, result);
    test.expectEq(static_cast<uint8_t>(result.kind),
                  static_cast<uint8_t>(ResultKind::Escape),
                  F("escape kind retained"));
    test.expectEq(static_cast<uint8_t>(result.outcome),
                  static_cast<uint8_t>(Outcome::Escaped),
                  F("escape terminal result retained"));

    stateUnderTest = state();
    stateUnderTest.trainer = false;
    stateUnderTest.gatherable = true;
    stateUnderTest.gather.need = 5;
    stateUnderTest.gather.tierRate = 2;
    resolveAction(stateUnderTest, Side::Player,
                  {ActionKind::Gather, 255}, rng, result);
    test.expectEq(static_cast<uint8_t>(result.kind),
                  static_cast<uint8_t>(ResultKind::Gather),
                  F("gather kind retained"));
    test.expectEq(stateUnderTest.gather.progress, 2,
                  F("gather progress remains in battle state"));

    test_battle_compact_moveuses(test);
}
