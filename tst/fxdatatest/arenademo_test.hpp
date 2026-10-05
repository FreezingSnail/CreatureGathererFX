#pragma once

#include "fxtest.hpp"
#include "arena_demo_ids.hpp"
#include "generated/arena_demo_data.hpp"
#include "src/engine/arena/ArenaCatalog.hpp"
#include "src/engine/arena/ArenaDemo.hpp"
#include "src/engine/battle/BattleFlow.hpp"
#include "src/engine/battle/MoveUses.hpp"
#include "src/engine/ModeState.hpp"
#include "src/engine/draw.h"
#include "src/engine/menu/MenuV2.hpp"
#include "src/lib/FxReadCounter.hpp"
#include "src/lib/ReadData.hpp"
#include "src/lib/MoveIds.hpp"
#include "src/player/Player.hpp"
#include "src/globals.hpp"

extern "C" uint8_t __bss_end;

namespace arena_spike_test {
inline uint8_t randomValue(uint8_t) { return 0; }

inline void renderBattle()
{
    battle::BattleView view = battleSession().view();
    battlePresenter().overlay(view);
    arduboy.clear();
    drawScene(view);
    battlePresenter().draw();
}

inline bool finishTrainer(FxTest &test)
{
    uint16_t frames = 0;
    battle::PresenterStage previous = battle::PresenterStage::Idle;
    bool returned = false;
    while (frames < 25000 && !returned) {
        FxReadCounter::resetFrame();
        if (battlePresenter().stage() == battle::PresenterStage::Idle &&
            battleSession().awaitingPlayer()) {
            BattleFlow::update(0, arena::finishBattle);
            BattleFlow::update(MENU_EDGE_A, arena::finishBattle);
            if (battle::remainingMoveUses(battleSession().state(),
                                          battle::Side::Player, 1))
                BattleFlow::update(MENU_NAV_RIGHT, arena::finishBattle);
            BattleFlow::update(MENU_EDGE_A, arena::finishBattle);
        } else {
            returned = BattleFlow::update((frames % 11u) == 0 ? MENU_EDGE_A : 0,
                                          arena::finishBattle);
        }
        if (returned) break;
        const bool updatePassed = FxReadCounter::markUpdate();
        const battle::PresenterStage current = battlePresenter().stage();
        if (current != previous) renderBattle();
        previous = current;
        const bool renderPassed = FxReadCounter::renderExact(0);
        if (!updatePassed || !renderPassed || !FxReadCounter::framePassed()) {
            test.expectEq(false, true, F("arena"));
            return false;
        }
        ++frames;
    }
    test.expectEq(returned, true,
                  F("arena"));
    test.expectEq(frames < 25000, true, F("arena"));
    Serial.print(F("arena trainer outcome="));
    Serial.println(static_cast<uint8_t>(arena::arenaContext.outcome));
    Serial.print(F("arena trainer frames="));
    Serial.println(frames);
    return returned;
}

inline void paintStack(uint16_t top)
{
    const uint16_t base = reinterpret_cast<uint16_t>(&__bss_end);
    for (volatile uint8_t *cursor = reinterpret_cast<volatile uint8_t *>(base);
         reinterpret_cast<uint16_t>(cursor) < top; ++cursor)
        *cursor = 0xC5;
}

inline uint16_t paintedHeadroom(uint16_t top)
{
    const uint16_t base = reinterpret_cast<uint16_t>(&__bss_end);
    for (volatile uint8_t *cursor = reinterpret_cast<volatile uint8_t *>(base);
         reinterpret_cast<uint16_t>(cursor) < top; ++cursor) {
        if (*cursor != 0xC5) return reinterpret_cast<uint16_t>(cursor) - base;
    }
    return top - base;
}

inline void recordMinimum(uint16_t top, uint16_t &minimum)
{
    const uint16_t measured = paintedHeadroom(top);
    if (measured < minimum) minimum = measured;
}

inline void verifyPlayerTeam(FxTest &test, uint8_t team)
{
    FxReadCounter::resetFrame();
    test.expectEq(arena::applyPlayerTeam(team, player), true,
                  F("arena"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("arena"));

    for (uint8_t slot = 0; slot < 3; ++slot) {
        arena::Member member{};
        test.expectEq(arena::readPlayerMember(team, slot, member), true,
                      F("arena"));

        const CreatureData_t seed = getCreatureFromStore(member.species);
        test.expectEq(FxReadCounter::transitionExact(1), true,
                      F("arena"));
        Creature expected;
        expected.id = seed.id;
        expected.level = member.level;
        expected.loadTypes(seed);
        expected.setStats(seed);

        test.expectEq(player.party[slot].id, member.species,
                      F("arena"));
        test.expectEq(player.party[slot].level, member.level,
                      F("arena"));
        test.expectEq(player.party[slot].statlist.hp, expected.statlist.hp,
                      F("arena"));
        test.expectEq(player.party[slot].statlist.attack, expected.statlist.attack,
                      F("arena"));
        test.expectEq(player.creatureHPs[slot], expected.statlist.hp,
                      F("arena"));

        for (uint8_t moveSlot = 0; moveSlot < 4; ++moveSlot) {
            const uint8_t moveId = member.moveIds[moveSlot];
            test.expectEq(player.party[slot].moves[moveSlot], moveId,
                          F("arena"));
            if (moveId == LEGACY_EMPTY_MOVE_ID) {
                test.expectEq(player.party[slot].moveList[moveSlot].move,
                              static_cast<uint16_t>(0),
                              F("arena"));
                test.expectEq(static_cast<uint8_t>(
                                  player.party[slot].moveList[moveSlot].effect1),
                              static_cast<uint8_t>(Effect::NONE),
                              F("arena"));
            } else {
                const Move expectedMove = readMoveFX(moveId);
                test.expectEq(FxReadCounter::transitionExact(1), true,
                              F("arena"));
                test.expectEq(player.party[slot].moveList[moveSlot].move,
                              expectedMove.move,
                              F("arena"));
                test.expectEq(static_cast<uint8_t>(
                                  player.party[slot].moveList[moveSlot].effect1),
                              static_cast<uint8_t>(expectedMove.effect1),
                              F("arena"));
            }
        }
    }
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("arena"));
}

inline void mutateFreshnessState()
{
    for (uint8_t slot = 0; slot < 3; ++slot) {
        player.creatureHPs[slot] = static_cast<uint8_t>(slot + 1);
        player.party[slot].status.applyEffect(Effect::CONCUSED);
        player.party[slot].statMods.setModifier(StatType::ATTACK_M, 2);
    }

    battle::BattleState &state =
        const_cast<battle::BattleState &>(battleSession().state());
    for (uint8_t side = 0; side < 2; ++side) {
        state.active[side].status.applyEffect(Effect::CONCUSED);
        state.active[side].statMods.setModifier(StatType::ATTACK_M, 2);
        for (uint8_t slot = 0; slot < 3; ++slot) {
            state.partyModifiers[side][slot] = 0xffff;
            state.moveUsesSpent[side][slot] = 0xff;
        }
    }
}

inline void expectFreshState(FxTest &test)
{
    const battle::BattleState &state = battleSession().state();
    test.expectEq(battleSession().isActive(), true,
                  F("arena"));
    test.expectEq(battleSession().awaitingPlayer(), true,
                  F("arena"));
    test.expectEq(static_cast<uint8_t>(battleSession().result().outcome),
                  static_cast<uint8_t>(battle::Outcome::None),
                  F("arena"));
    for (uint8_t slot = 0; slot < 3; ++slot) {
        test.expectEq(player.creatureHPs[slot], player.party[slot].statlist.hp,
                      F("arena"));
        test.expectEq(static_cast<uint8_t>(player.party[slot].status.effects[0]),
                      static_cast<uint8_t>(Effect::NONE),
                      F("arena"));
        test.expectEq(player.party[slot].statMods.modifiers,
                      static_cast<uint16_t>(0),
                      F("arena"));
    }
    for (uint8_t side = 0; side < 2; ++side) {
        test.expectEq(static_cast<uint8_t>(state.active[side].status.effects[0]),
                      static_cast<uint8_t>(Effect::NONE),
                      F("arena"));
        test.expectEq(state.active[side].statMods.modifiers,
                      static_cast<uint16_t>(0),
                      F("arena"));
        for (uint8_t slot = 0; slot < 3; ++slot) {
            test.expectEq(state.partyModifiers[side][slot],
                          static_cast<uint16_t>(0),
                          F("arena"));
            test.expectEq(state.moveUsesSpent[side][slot], static_cast<uint8_t>(0),
                          F("arena"));
        }
    }
}
} // namespace arena_spike_test

inline void test_arenademo(FxTest &test)
{
    const uint16_t residentEndBefore = reinterpret_cast<uint16_t>(&__bss_end);
    const uint16_t top = static_cast<uint16_t>(SP) - 64;
    uint16_t minimumStack = 0xffff;

    FxReadCounter::resetFrame();
    arena::boot();
    test.expectEq(gameState.state, GameState_t::ARENA,
                  F("arena"));
    test.expectEq(sizeof(arena::ArenaContext), static_cast<size_t>(3),
                  F("arena"));
    test.expectEq(sizeof(arena::ArenaUiState), static_cast<size_t>(24),
                  F("arena"));
    test.expectEq(sizeof(arena::ArenaPreview), static_cast<size_t>(19),
                  F("arena"));
    test.expectEq(sizeof(ModeState), static_cast<size_t>(191),
                  F("arena"));
    test.expectEq(modeState.arena.ui.list.itemCount, ArenaDemoIds::playerCount,
                  F("arena"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("arena"));
    test.expectEq(FxReadCounter::renderExact(0), true,
                  F("arena"));

    const uint8_t unchangedSpecies = player.party[0].id;
    const uint8_t unchangedLevel = player.party[0].level;
    const uint8_t unchangedHp = player.creatureHPs[0];
    arena::arenaContext.playerTeam = ArenaDemoIds::playerCount;
    FxReadCounter::resetFrame();
    test.expectEq(arena::startMatch(), false,
                  F("arena"));
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(0),
                  F("arena"));
    test.expectEq(gameState.state, GameState_t::ARENA,
                  F("arena"));
    test.expectEq(player.party[0].id, unchangedSpecies,
                  F("arena"));
    test.expectEq(player.party[0].level, unchangedLevel,
                  F("arena"));
    test.expectEq(player.creatureHPs[0], unchangedHp,
                  F("arena"));

    arena::arenaContext.playerTeam = ArenaDemoIds::player_blitz;
    arena::arenaContext.opponentTeam = ArenaDemoIds::opponentCount;
    FxReadCounter::resetFrame();
    test.expectEq(arena::startMatch(), false,
                  F("arena"));
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(0),
                  F("arena"));
    test.expectEq(player.party[0].id, unchangedSpecies,
                  F("arena"));

    arena::arenaContext.opponentTeam = ArenaDemoIds::opponent_starter;
    arena_spike_test::verifyPlayerTeam(test, ArenaDemoIds::player_blitz);
    arena::arenaContext.playerTeam = ArenaDemoIds::player_blitz;
    arena::arenaContext.opponentTeam = ArenaDemoIds::opponent_starter;
    arena_spike_test::paintStack(top);
    FxReadCounter::resetFrame();
    test.expectEq(arena::startMatch(), true,
                  F("arena"));
    arena_spike_test::recordMinimum(top, minimumStack);
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("arena"));
    test.expectEq(gameState.state, GameState_t::BATTLE,
                  F("arena"));
    test.expectEq(battleSession().state().trainerId, ArenaDemoIds::trainer_starter,
                  F("arena"));
    test.expectEq(battleSession().state().partyCount[0], static_cast<uint8_t>(3),
                  F("arena"));
    test.expectEq(battleSession().state().partyCount[1], static_cast<uint8_t>(3),
                  F("arena"));
    test.expectEq(battleSession().state().active[0].id, player.party[0].id,
                  F("arena"));
    test.expectEq(battleSession().state().bench[1][1].id, static_cast<uint8_t>(0),
                  F("arena"));

    battleSession().setRng({arena_spike_test::randomValue});
    arena_spike_test::paintStack(top);
    const bool completed = arena_spike_test::finishTrainer(test);
    arena_spike_test::recordMinimum(top, minimumStack);
    test.expectEq(completed, true, F("arena"));
    test.expectEq(gameState.state, GameState_t::ARENA,
                  F("arena"));
    test.expectEq(arena::arenaContext.outcome == battle::Outcome::Win ||
                      arena::arenaContext.outcome == battle::Outcome::Lose,
                  true, F("arena"));
    test.expectEq(arena::arenaContext.playerTeam, ArenaDemoIds::player_blitz,
                  F("arena"));
    test.expectEq(arena::arenaContext.opponentTeam, ArenaDemoIds::opponent_starter,
                  F("arena"));
    test.expectEq(static_cast<uint8_t>(modeState.arena.ui.screen),
                  static_cast<uint8_t>(arena::ArenaScreen::Result),
                  F("arena"));

    arena::arenaContext.playerTeam = ArenaDemoIds::player_utility;
    arena::arenaContext.opponentTeam = ArenaDemoIds::opponent_champion;
    arena_spike_test::verifyPlayerTeam(test, ArenaDemoIds::player_utility);
    arena_spike_test::paintStack(top);
    FxReadCounter::resetFrame();
    test.expectEq(arena::startMatch(), true,
                  F("arena"));
    arena_spike_test::recordMinimum(top, minimumStack);
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("arena"));
    test.expectEq(battleSession().state().trainerId, ArenaDemoIds::trainer_champion,
                  F("arena"));
    test.expectEq(battleSession().state().active[1].id, static_cast<uint8_t>(31),
                  F("arena"));
    test.expectEq(player.party[1].moves[1], static_cast<uint8_t>(43),
                  F("arena"));

    for (uint8_t cycle = 0; cycle < 10; ++cycle) {
        arena_spike_test::mutateFreshnessState();
        arena::finishBattle(battle::Outcome::Win);
        test.expectEq(gameState.state, GameState_t::ARENA,
                      F("arena"));
        arena_spike_test::paintStack(top);
        FxReadCounter::resetFrame();
        test.expectEq(arena::startMatch(), true,
                      F("arena"));
        arena_spike_test::recordMinimum(top, minimumStack);
        test.expectEq(FxReadCounter::markUpdate(), true,
                      F("arena"));
        arena_spike_test::expectFreshState(test);
    }

    test.expectEq(minimumStack >= 219, true,
                  F("arena"));
    test.expectEq(sizeof(ModeState), static_cast<size_t>(191),
                  F("arena"));
    test.expectEq(reinterpret_cast<uint16_t>(&__bss_end), residentEndBefore,
                  F("arena"));
    Serial.print(F("arena start/callback effective stack headroom="));
    Serial.println(minimumStack >= 69 ? minimumStack - 69 : 0);
    Serial.println(F("arena replay cycles=10"));
}
