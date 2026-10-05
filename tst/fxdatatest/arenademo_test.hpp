#pragma once

#include "fxtest.hpp"
#include "src/engine/arena/ArenaDemo.hpp"
#include "src/engine/battle/BattleFlow.hpp"
#include "src/engine/battle/MoveUses.hpp"
#include "src/engine/ModeState.hpp"
#include "src/engine/battle/BattlePresets.hpp"
#include "src/engine/draw.h"
#include "src/engine/menu/MenuV2.hpp"
#include "src/lib/FxReadCounter.hpp"
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
            if (battle::remainingMoveUses(battleSession().state(), battle::Side::Player, 1))
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
            test.expectEq(false, true, F("arena battle keeps transition-only FX reads"));
            return false;
        }
        ++frames;
    }
    test.expectEq(returned, true, F("arena trainer battle completes through playback callback"));
    test.expectEq(frames < 25000, true, F("arena trainer playback is bounded"));
    Serial.print(F("arena trainer outcome="));
    Serial.println(static_cast<uint8_t>(arena::arenaContext.outcome));
    Serial.print(F("arena trainer frames="));
    Serial.println(frames);
    return returned;
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
} // namespace arena_spike_test

inline void test_arenademo(FxTest &test)
{
    const uint16_t base = reinterpret_cast<uint16_t>(&__bss_end);
    const uint16_t top = static_cast<uint16_t>(SP) - 64;
    for (volatile uint8_t *cursor = reinterpret_cast<volatile uint8_t *>(base);
         reinterpret_cast<uint16_t>(cursor) < top; ++cursor) *cursor = 0xC5;

    arena::boot();
    test.expectEq(gameState.state, GameState_t::ARENA,
                  F("arena starts outside world and battle"));
    test.expectEq(sizeof(arena::ArenaContext), static_cast<size_t>(3),
                  F("selection context is three bytes"));
    test.expectEq(sizeof(arena::ArenaUiState), static_cast<size_t>(24),
                  F("arena UI fits its 24-byte overlay"));
    test.expectEq(sizeof(arena::ArenaPreview), static_cast<size_t>(19),
                  F("arena preview fits its 19-byte overlay"));
    test.expectEq(sizeof(ModeState), static_cast<size_t>(191),
                  F("ModeState remains 191 bytes"));

    FxReadCounter::resetFrame();
    arena::update(MENU_EDGE_A);
    test.expectEq(gameState.state, GameState_t::ARENA,
                  F("first confirm opens opponent selection"));
    test.expectEq(static_cast<uint8_t>(modeState.arena.ui.screen),
                  static_cast<uint8_t>(arena::ArenaScreen::OpponentTeam),
                  F("player choice and opponent choice are separate steps"));
    test.expectEq(arena::arenaContext.playerTeam, static_cast<uint8_t>(0),
                  F("first authored player preset is selected"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("temporary spike navigation performs no table reads"));

    arena::update(MENU_EDGE_A);
    test.expectEq(gameState.state, GameState_t::BATTLE,
                  F("second confirm starts normal trainer battle"));
    const BattlePresets::Preset opponent =
        BattlePresets::copyPreset(BattlePresets::opening);
    test.expectEq(battleSession().state().trainerId, opponent.trainerId,
                  F("generated matchup trainer enters BattleSession"));
    test.expectEq(battleSession().view().partyCount[0], static_cast<uint8_t>(3),
                  F("player preset starts a three member team"));
    test.expectEq(battleSession().view().partyCount[1], static_cast<uint8_t>(3),
                  F("trainer preset starts a three member team"));

    battleSession().setRng({arena_spike_test::randomValue});
    const bool completed = arena_spike_test::finishTrainer(test);
    test.expectEq(completed, true, F("first 2x2 matchup completes without reset"));
    test.expectEq(gameState.state, GameState_t::ARENA,
                  F("completed match returns to arena"));
    test.expectEq(arena::arenaContext.outcome != battle::Outcome::None, true,
                  F("match outcome persists outside BattleMode"));
    test.expectEq(arena::arenaContext.playerTeam, static_cast<uint8_t>(0),
                  F("player selection persists after match"));
    test.expectEq(arena::arenaContext.opponentTeam, static_cast<uint8_t>(0),
                  F("opponent selection persists after match"));

    FxReadCounter::resetFrame();
    arena::update(MENU_EDGE_A);
    test.expectEq(gameState.state, GameState_t::BATTLE,
                  F("rematch starts without resetting the demo"));
    test.expectEq(player.creatureHPs[0], player.party[0].statlist.hp,
                  F("rematch restores player HP"));

    // Exercise the other complete team/opponent pairing independently of the
    // completed 0x0 battle above.
    arena::finishBattle(battle::Outcome::Win);
    arena::update(MENU_NAV_DOWN);
    arena::update(MENU_NAV_DOWN);
    arena::update(MENU_EDGE_A);
    test.expectEq(static_cast<uint8_t>(modeState.arena.ui.screen),
                  static_cast<uint8_t>(arena::ArenaScreen::PlayerTeam),
                  F("change team opens player selection"));
    arena::update(MENU_NAV_DOWN);
    arena::update(MENU_EDGE_A);
    arena::update(MENU_NAV_DOWN);
    arena::update(MENU_EDGE_A);
    test.expectEq(gameState.state, GameState_t::BATTLE,
                  F("second team and opponent combination starts"));
    test.expectEq(arena::arenaContext.playerTeam, static_cast<uint8_t>(1),
                  F("second player team is independently selected"));
    test.expectEq(arena::arenaContext.opponentTeam, static_cast<uint8_t>(1),
                  F("second trainer team is independently selected"));
    const BattlePresets::Preset secondOpponent =
        BattlePresets::copyPreset(BattlePresets::switch_drill);
    test.expectEq(battleSession().state().trainerId, secondOpponent.trainerId,
                  F("second selected trainer ID reaches BattleSession"));
    const uint16_t measured = arena_spike_test::paintedHeadroom(top);
    Serial.print(F("arena setup/return painted headroom=")); Serial.println(measured);
    Serial.print(F("arena setup/return effective headroom="));
    Serial.println(measured >= 69 ? measured - 69 : 0);
    test.expectEq(measured >= 219, true,
                  F("arena start and terminal callback preserve 150 B after USB ISR"));
}
