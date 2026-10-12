#pragma once

#include "fxtest.hpp"
#include "src/engine/battle/BattleFeedback.hpp"
#include "src/engine/battle/BattleSession.hpp"
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
} // namespace battle_compact_test_detail

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
}
