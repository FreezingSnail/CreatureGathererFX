#pragma once
#include "fxtest.hpp"
#include "generated/move_data.hpp"
#include "src/engine/battle/BattleSetup.hpp"
#include "src/engine/battle/MoveUses.hpp"
#include "src/engine/battle/Effects.hpp"
#include "src/engine/battle/Ai.hpp"
#include "src/engine/battle/Damage.hpp"
#include "src/lib/FxReadCounter.hpp"
#include "src/lib/MoveIds.hpp"
#include "src/lib/ReadData.hpp"
#include "src/player/Player.hpp"

namespace battle_utility_fx_detail {
// Static state keeps the test's own state out of the measured transition stack.
static battle::BattleState state;
static battle::ActionResult out;
extern "C" uint8_t __bss_end;
inline uint16_t paintStack() {
    const uint16_t base = reinterpret_cast<uint16_t>(&__bss_end);
    const uint16_t top = static_cast<uint16_t>(SP) - 64;
    if (top <= base) return base;
    for (volatile uint8_t *p = reinterpret_cast<volatile uint8_t *>(base);
         reinterpret_cast<uint16_t>(p) < top; ++p) *p = 0xC5;
    return top;
}
inline uint16_t headroom(uint16_t top) {
    const uint16_t base = reinterpret_cast<uint16_t>(&__bss_end);
    for (volatile uint8_t *p = reinterpret_cast<volatile uint8_t *>(base);
         reinterpret_cast<uint16_t>(p) < top; ++p)
        if (*p != 0xC5) return reinterpret_cast<uint16_t>(p) - base;
    return top - base;
}
}

inline void test_battleutility(FxTest &test) {
    using namespace battle;
    using namespace battle_utility_fx_detail;
    // Expected records are addressed independently of the runtime ID decoder.
    for (uint8_t id = 36; id <= 44; ++id) {
        const uint8_t row = id == 44 ? 32 : id - 1;
        const uint32_t raw = pgm_read_dword(&moveFixtures[row]);
        const Move loaded = readMoveFX(id);
        test.expectEqIdx(loaded.move, raw >> 16, F("utility record descriptor"), id);
        test.expectEqIdx(static_cast<uint8_t>(loaded.effect1), (raw >> 8) & 255,
                         F("utility first effect"), id);
        test.expectEqIdx(static_cast<uint8_t>(loaded.effect2), raw & 255,
                         F("utility second effect"), id);
    }
    const Move empty = readMoveFX(32);
    test.expectEq(empty.move, 0, F("legacy empty descriptor"));
    test.expectEq(static_cast<uint8_t>(empty.effect1), static_cast<uint8_t>(Effect::NONE),
                  F("legacy empty effect"));
    test.expectEq(readMoveFX(255).move, 0, F("empty sentinel descriptor"));
    for (uint8_t slot = 0; slot < PARTY_SIZE; ++slot) {
        player.loadCreature(slot, slot + 1);
        player.creatureHPs[slot] = 40;
    }
    const uint16_t top = paintStack();
    beginWild(state, 4, 10, false, 1);
    state.active[0].moveIds[0] = 0;
    state.active[0].moves[0] = Move(MoveBitSet{static_cast<uint8_t>(Type::SPIRIT), 10, 1, 0, 0});
    spendMoveUse(state, Side::Player, 0);
    Consequence fact;
    applyEffect(state, Side::Player, Effect::ATKUP, fact);
    test.expectEq(applySwitch(state, Side::Player, 2, false, out), true, F("switch original slot two"));
    test.expectEq(state.active[0].statMods.getModifier(StatType::ATTACK_M), 0, F("reserve stage independent"));
    test.expectEq(state.moveUsesSpent[0][2], 0, F("reserve use independent"));
    test.expectEq(applySwitch(state, Side::Player, 0, false, out), true, F("switch original slot zero"));
    test.expectEq(state.moveUsesSpent[0][0], 1, F("spent use survives FX reload"));
    test.expectEq(state.active[0].statMods.getModifier(StatType::ATTACK_M), 1, F("stage survives FX reload"));
    beginWild(state, 4, 10, false, 1);
    test.expectEq(state.moveUsesSpent[0][0], 0, F("battle start refills"));
    test.expectEq(state.partyModifiers[0][0], 0, F("battle start clears stage bank"));
    // Exercise risk products above 65535 on AVR's actual __uint24 path.
    state = {};
    state.trainer = true;
    state.partyCount[0] = 1;
    state.partyCount[1] = 3;
    for (uint8_t side = 0; side < 2; ++side) {
        state.active[side].types = DualType(Type::SPIRIT);
        state.active[side].hp = state.active[side].maxHp = 255;
        state.active[side].stats.attack = 40;
        state.active[side].stats.defense = 2;
        for (uint8_t slot = 0; slot < 4; ++slot)
            state.active[side].moveIds[slot] = 255;
        state.active[side].moveIds[0] = 1;
        state.active[side].moves[0] = Move(MoveBitSet{
            static_cast<uint8_t>(Type::SPIRIT), uint8_t(side ? 1 : 31), 1, 0, 0});
    }
    state.bench[1][0] = {3, 1, 255, DualType(Type::SPIRIT), 14, 14};
    state.bench[1][1] = {4, 1, 254, DualType(Type::SPIRIT), 14, 14};
    FxReadCounter::resetFrame();
    BattleAction choice = chooseAction(state, Side::Opponent);
    test.expectEq(static_cast<uint8_t>(choice.kind), static_cast<uint8_t>(ActionKind::Switch),
                  F("wide risk selects switch"));
    test.expectEq(choice.index, 1, F("wide risk preserves best slot"));
    test.expectEq(FxReadCounter::count(), 0, F("switch decision performs no FX reads"));
    state.bench[1][1].hp = 255;
    FxReadCounter::resetFrame();
    test.expectEq(chooseAction(state, Side::Opponent).index, 1, F("wide risk stable tie"));
    test.expectEq(FxReadCounter::count(), 0, F("repeat decision performs no FX reads"));
    state.switchLockMask = 2;
    FxReadCounter::resetFrame();
    test.expectEq(static_cast<uint8_t>(chooseAction(state, Side::Opponent).kind),
                  static_cast<uint8_t>(ActionKind::Attack),
                  F("switch lock prevents an immediate repeat switch"));
    test.expectEq(FxReadCounter::count(), 0, F("locked decision performs no FX reads"));
    state.switchLockMask = 0;
    state.active[0].stats.attack = 1;
    FxReadCounter::resetFrame();
    test.expectEq(static_cast<uint8_t>(chooseAction(state, Side::Opponent).kind),
                  static_cast<uint8_t>(ActionKind::Attack),
                  F("neutral threat keeps the fallback attack"));
    test.expectEq(FxReadCounter::count(), 0, F("neutral decision performs no FX reads"));
    state.active[0].stats.attack = 255;
    state.active[0].statMods.setModifier(StatType::ATTACK_M, 3);
    test.expectEq(computeDamage(state.active[0], state.active[1], 0), 255,
                  F("maximum packed attack saturates without overflow"));
    const uint16_t free = headroom(top);
    Serial.print(F("utility transition stack headroom="));
    Serial.println(free);
    test.expectEq(free >= 150, true, F("utility focused stack reserve"));
}
