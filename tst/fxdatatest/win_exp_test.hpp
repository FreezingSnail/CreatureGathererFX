#pragma once
// jp8.11.10 device proof: an ordinary wild Win grows every living occupied
// party instance's persistent store experience by exactly 8 * opponentLevel,
// once, and bounded level growth follows the existing level model. Wired by the
// dedicated sketch tst/fxdatatest/test_win_exp.ino so the near-limit
// test_battle_duel suite keeps its headroom. Reuses the duel session fixtures.

#include <string.h>

#include "duel_session_integration_test.hpp"

namespace win_exp_test_detail {
using namespace duel_session_test_detail;

// Install a real three-member beta party as stable store instances (slots 0..2)
// and mirror them into the live Player. `exps` pins each member's stored
// cumulative experience; `missing` pins the HP missing from its derived maximum
// (>= maximum = KO). Level/stats follow the stored exp through the existing model.
__attribute__((noinline)) void seedPartyInstances(const uint16_t (&exps)[PARTY_SIZE],
                        const uint8_t (&missing)[PARTY_SIZE]) {
    storeFresh(); // empty collection, cold journal
    battle::applyPlayerPreset(player, BattlePresets::beta);
    for (uint8_t i = 0; i < PARTY_SIZE; ++i) {
        player.party[i].level = levelFromExp(exps[i]);
        player.party[i].setStats(getCreatureFromStore(player.party[i].id));
        const uint8_t maxHp = player.party[i].hp;
        const uint8_t curHp = missing[i] >= maxHp ? 0 : static_cast<uint8_t>(maxHp - missing[i]);
        player.creatureHPs[i] = curHp;
        StoreRecord record = {};
        record.id = player.party[i].id;
        record.exp = exps[i];
        record.attackMask = static_cast<uint8_t>(player.party[i].attackMask & 0x1F);
        record.hpEncoded = curHp >= maxHp ? 0 : static_cast<uint8_t>(curHp + 1);
        storeWrite(i, record);
        storeAssign(i, i, curHp);
    }
}

// A wild session at a known foe level, ready for a one-hit Win (1-HP Fire foe;
// the Fire kit picks Spirit so the foe is KO'd on the reveal). The living hero
// starts at its derived maximum so the later checkpoint stays inside the model.
__attribute__((noinline)) void beginWildWinSession(uint8_t foeLevel, bool gatherable) {
    menu.clear();
    dialogMenu.clear();
    enterBattle();
    gameState.state = GameState_t::BATTLE;
    rngDraws = 0;
    battleSession().setRng({scriptedRng});
    battleSession().beginWild(0, foeLevel, gatherable, gatherable ? 1 : 0);
    configureMatchup(FireAuthored, SpiritAuthored, FireAuthored, FireAuthored,
                     player.party[0].hp, 1);
}

__attribute__((noinline)) void wildWinToWorld() {
    BattleFlow::update(0);              // choice entry -> pick row
    BattleFlow::update(MENU_EDGE_A);    // commit the single attack cell
    for (uint16_t f = 0; f < 4096 && gameState.state != GameState_t::WORLD; ++f)
        BattleFlow::update(0);
}

__attribute__((noinline)) uint16_t storedExp(uint8_t slot) {
    StoreRecord record;
    storeRead(slot, record);
    return record.exp;
}

const uint8_t kFull[PARTY_SIZE] = {0, 0, 0};

// jp8.11.10: an ordinary wild Win grows every living occupied member's stored
// exp by exactly 8 * opponentLevel once; levels follow the exact thresholds, a
// full level-up reaches its new derived maximum, a damaged one keeps its missing
// HP, a KO member gets nothing, and exp saturates at the level-31 cap.
__attribute__((noinline)) void duelDeviceWildWinExp(FxTest &test) {
    const uint16_t start = levelExpThreshold(3);

    // Exact amount once, no threshold crossed.
    const uint16_t exps[PARTY_SIZE] = {start, start, start};
    seedPartyInstances(exps, kFull);
    beginWildWinSession(4, false); // gain = 32
    wildWinToWorld();
    test.expectEq(storedExp(0), static_cast<uint16_t>(start + 32),
                  F("exp grows by exactly 8 * opponentLevel"));
    test.expectEq(player.party[1].level, static_cast<uint8_t>(3),
                  F("below the next threshold the level is unchanged"));
    for (uint8_t i = 0; i < 8; ++i) wildWinToWorld();
    test.expectEq(storedExp(1), static_cast<uint16_t>(start + 32),
                  F("later frames never re-award"));

    // Below / at / over the next threshold, with full and damaged HP growth.
    const uint16_t bounds[PARTY_SIZE] = {31, 32, 100};
    const uint8_t missing[PARTY_SIZE] = {0, 0, 5};
    seedPartyInstances(bounds, missing);
    beginWildWinSession(4, false);
    wildWinToWorld();
    test.expectEq(storedExp(0), static_cast<uint16_t>(63),
                  F("one below the threshold stores the exact total"));
    test.expectEq(player.party[1].level, static_cast<uint8_t>(4),
                  F("at the threshold the level increases"));
    test.expectEq(player.creatureHPs[1], player.party[1].hp,
                  F("a full level-up reaches its new derived maximum"));
    test.expectEq(storedExp(2), static_cast<uint16_t>(132),
                  F("over the threshold keeps the exact remainder"));
    test.expectEq(player.creatureHPs[2],
                  static_cast<uint8_t>(player.party[2].hp - 5),
                  F("a damaged level-up preserves its missing HP"));

    // Bounded growth at the level-31 cap; a KO member is untouched.
    const uint16_t cap = levelExpThreshold(LEVEL_COUNT - 1);
    const uint16_t nearCap[PARTY_SIZE] = {static_cast<uint16_t>(cap - 1), cap, start};
    const uint8_t ko[PARTY_SIZE] = {0, 0, 255};
    seedPartyInstances(nearCap, ko);
    beginWildWinSession(31, false); // gain = 248
    wildWinToWorld();
    test.expectEq(storedExp(0), cap, F("exp saturates at the level-31 threshold"));
    test.expectEq(player.party[0].level, static_cast<uint8_t>(LEVEL_COUNT - 1),
                  F("the derived level caps at 31"));
    test.expectEq(storedExp(2), start, F("a KO member receives no exp"));
    test.expectEq(player.creatureHPs[2], static_cast<uint8_t>(0),
                  F("a KO member is never revived"));
}

// jp8.11.12: crossing the authored level-8 edge on the real growth terminal
// rewrites each stable instance to its explicit target exactly once, preserving
// exp and moves; the evolved target (no further edge) never evolves again, a KO
// member is skipped, and the evolved species survives a real checkpoint/reboot.
__attribute__((noinline)) void duelDeviceWildWinEvolution(FxTest &test) {
    const uint16_t before = static_cast<uint16_t>(levelExpThreshold(8) - 1); // level 7
    const uint16_t exps[PARTY_SIZE] = {before, before, before};
    const uint8_t missing[PARTY_SIZE] = {0, 5, 255}; // member 2 KO
    seedPartyInstances(exps, missing);
    beginWildWinSession(1, false); // gain 8 crosses the level-8 edge
    wildWinToWorld();

    test.expectEq(player.party[0].id, static_cast<uint8_t>(1), F("species 0 evolves to 1"));
    test.expectEq(player.party[1].id, static_cast<uint8_t>(4), F("species 3 evolves to 4"));
    StoreRecord evolved = {};
    storeRead(0, evolved);
    test.expectEq(evolved.id, static_cast<uint8_t>(1), F("the stable slot holds the target"));
    test.expectEq(storedExp(0), static_cast<uint16_t>(before + 8),
                  F("exp preserved across evolution"));
    test.expectEq(player.party[0].level, static_cast<uint8_t>(8),
                  F("the derived level reaches the edge"));
    test.expectEq(player.party[0].attackMask, static_cast<uint8_t>(2),
                  F("the existing moves are preserved"));
    test.expectEq(player.party[1].attackMask, static_cast<uint8_t>(12),
                  F("the evolved crab adopts its target kit (4 -> 12)"));
    test.expectEq(player.creatureHPs[1],
                  static_cast<uint8_t>(player.party[1].hp - 5),
                  F("missing HP preserved across evolution"));
    test.expectEq(player.party[2].id, static_cast<uint8_t>(6),
                  F("a KO member does not evolve"));
    test.expectEq(player.creatureHPs[2], static_cast<uint8_t>(0),
                  F("a KO member is never revived"));

    // Once: a later win grows exp but never re-evolves a target species.
    const uint16_t afterFirst = storedExp(0);
    beginWildWinSession(1, false);
    wildWinToWorld();
    test.expectEq(player.party[0].id, static_cast<uint8_t>(1),
                  F("the evolved target never re-evolves"));
    test.expectEq(storedExp(0), static_cast<uint16_t>(afterFirst + 8),
                  F("exp still grows after evolution"));

    // Persistence: an explicit v3 checkpoint then reboot keeps the evolved species.
    SaveFile save = {};
    memset(save.partySlots, 255, 3);
    saveBegin();
    SaveStep step = SaveStep::Idle;
    for (uint16_t s = 0; s < 256 && saveInProgress(); ++s) {
        step = saveStepAdvance(save);
        FX::waitWhileBusy();
    }
    test.expectEq(static_cast<uint32_t>(step), static_cast<uint32_t>(SaveStep::Done),
                  F("the evolution checkpoint completes"));
    SaveFile reboot = {};
    memset(reboot.partySlots, 255, 3);
    test.expectEq(storeBoot(reboot), true, F("the evolution checkpoint reboots"));
    StoreRecord restored = {};
    storeRead(0, restored);
    test.expectEq(restored.id, static_cast<uint8_t>(1),
                  F("the evolved species survives reboot"));
    test.expectEq(restored.exp, static_cast<uint16_t>(afterFirst + 8),
                  F("the exact exp survives reboot"));
}

// jp8.11.12: a boxed stable instance whose accepted stored experience already
// qualifies evolves in place through the real packed mapping, preserving exp and
// moves and re-encoding HP; below the edge it is unchanged and an evolved
// instance never re-evolves.
__attribute__((noinline)) void duelDeviceBoxedEvolution(FxTest &test) {
    storeFresh();
    StoreRecord boxed = {};
    boxed.id = 0;
    boxed.exp = levelExpThreshold(8);
    boxed.attackMask = 2;
    boxed.hpEncoded = 0; // full
    uint8_t slot = 255;
    test.expectEq(storeAdd(boxed, slot), true, F("a boxed instance is added"));
    test.expectEq(storeEvolve(slot), true, F("a qualifying boxed instance evolves"));
    StoreRecord evolved = {};
    storeRead(slot, evolved);
    test.expectEq(evolved.id, static_cast<uint8_t>(1), F("the boxed species becomes its target"));
    test.expectEq(evolved.exp, levelExpThreshold(8), F("the boxed exp is preserved"));
    test.expectEq(evolved.attackMask, static_cast<uint8_t>(2), F("the boxed moves are preserved"));
    test.expectEq(storeEvolve(slot), false, F("an evolved boxed instance never re-evolves"));

    // rhs7.4: the target species authored kit replaces the instance mask at the
    // level-8 crab edge (4 -> 12) and both level-16 chain links (snail 2 -> 18,
    // squid 2 -> 10); the stable slot keeps the adopted mask.
    storeFresh();
    StoreRecord crab = {};
    crab.id = 3;
    crab.exp = levelExpThreshold(8);
    crab.attackMask = 2;
    crab.hpEncoded = 0;
    uint8_t crabSlot = 255;
    test.expectEq(storeAdd(crab, crabSlot), true, F("a boxed crab is added"));
    test.expectEq(storeEvolve(crabSlot), true, F("the boxed crab evolves"));
    StoreRecord evCrab = {};
    storeRead(crabSlot, evCrab);
    test.expectEq(evCrab.id, static_cast<uint8_t>(4), F("the crab becomes 4"));
    test.expectEq(evCrab.attackMask, static_cast<uint8_t>(12),
                  F("the crab adopts the earth/fire kit (4 -> 12)"));

    storeFresh();
    StoreRecord snail = {};
    snail.id = 1;
    snail.exp = levelExpThreshold(16);
    snail.attackMask = 2;
    snail.hpEncoded = 0;
    uint8_t snailSlot = 255;
    test.expectEq(storeAdd(snail, snailSlot), true, F("a boxed snail is added"));
    test.expectEq(storeEvolve(snailSlot), true, F("the boxed snail chain link evolves"));
    StoreRecord evSnail = {};
    storeRead(snailSlot, evSnail);
    test.expectEq(evSnail.id, static_cast<uint8_t>(2), F("the snail becomes 2"));
    test.expectEq(evSnail.attackMask, static_cast<uint8_t>(18),
                  F("the snail adopts the water/plant kit (2 -> 18)"));

    storeFresh();
    StoreRecord squid = {};
    squid.id = 7;
    squid.exp = levelExpThreshold(16);
    squid.attackMask = 2;
    squid.hpEncoded = 0;
    uint8_t squidSlot = 255;
    test.expectEq(storeAdd(squid, squidSlot), true, F("a boxed squid is added"));
    test.expectEq(storeEvolve(squidSlot), true, F("the boxed squid chain link evolves"));
    StoreRecord evSquid = {};
    storeRead(squidSlot, evSquid);
    test.expectEq(evSquid.id, static_cast<uint8_t>(8), F("the squid becomes 8"));
    test.expectEq(evSquid.attackMask, static_cast<uint8_t>(10),
                  F("the squid adopts the water/fire kit (2 -> 10)"));

    storeFresh();
    StoreRecord low = {};
    low.id = 0;
    low.exp = static_cast<uint16_t>(levelExpThreshold(8) - 1);
    low.attackMask = 2;
    low.hpEncoded = 0;
    uint8_t lowSlot = 255;
    test.expectEq(storeAdd(low, lowSlot), true, F("a below-edge instance is added"));
    test.expectEq(storeEvolve(lowSlot), false, F("a below-edge instance does not evolve"));
    StoreRecord held = {};
    storeRead(lowSlot, held);
    test.expectEq(held.id, static_cast<uint8_t>(0), F("the below-edge species is unchanged"));
}

} // namespace win_exp_test_detail

inline void test_win_exp(FxTest &test) {
    win_exp_test_detail::duelDeviceWildWinExp(test);
    win_exp_test_detail::duelDeviceWildWinEvolution(test);
    win_exp_test_detail::duelDeviceBoxedEvolution(test);
}
