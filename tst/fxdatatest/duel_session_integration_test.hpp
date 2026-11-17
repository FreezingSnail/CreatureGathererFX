#pragma once
// Device proof for the fixed first-beta boss (CreatureGathererFX-jp8.11.13).
//
// The host suite (tst/duel_session_integration_test.hpp) owns the full duel
// timing/round/gather/item/exp/evolution matrix. This sketch keeps only the
// device proof the boss bead requires while restoring comfortable flash margin:
// one-member ardu, the authored boss persona/rate, Win records the flag after
// terminal feedback, Lose never records it, and steady frames perform no FX
// metadata reads.

#if !defined(CGFX_DUEL_BATTLE) || !defined(CGFX_BATTLE_COMPACT_RESULT) || \
    !defined(CGFX_BATTLE_NO_FACTS)
#error "duel_session_integration_test requires the duel compact profile: CGFX_DUEL_BATTLE, CGFX_BATTLE_COMPACT_RESULT, CGFX_BATTLE_NO_FACTS"
#endif

// The suite's metadata-read assertions are meaningless as no-ops. Require the
// counter so a future build that drops FX_READ_COUNTER fails loudly.
#ifndef FX_READ_COUNTER
#error "duel_session_integration_test requires FX_READ_COUNTER so metadata-read assertions cannot pass as no-ops"
#endif

#include "fxtest.hpp"
#include "src/fxdata.h"
#include "src/engine/battle/BattleFlow.hpp"
#include "src/engine/battle/BattleSession.hpp"
#include "src/engine/ModeState.hpp"
#include "src/engine/game/Gamestate.hpp"
#include "src/lib/FxReadCounter.hpp"
#include "src/player/Player.hpp"

// Generated flag id (defined once by World.cpp's flags mirror include).
extern uint16_t flag_beta_complete;

namespace duel_boss_test_detail {

// The appended one-member ardu opponent row, addressed through generated
// symbols rather than a literal packed row.
uint8_t bossId() {
    return static_cast<uint8_t>((opponent_seed_24 - opponent_seeds) / 9);
}

void beginBoss(uint8_t id) {
    player.basic();
    gameState.clearFlag(flag_beta_complete);
    enterBattle();
    gameState.state = GameState_t::BATTLE;
    battleSession().beginTrainer(id);
}

void rigWin() {
    battle::BattleState &state =
        const_cast<battle::BattleState &>(battleSession().state());
    state.active[0].types =
        DualType(static_cast<Type>(static_cast<uint8_t>(Type::FIRE)));
    state.active[0].attackMask = static_cast<uint8_t>(
        1u << duel::foldType(static_cast<uint8_t>(Type::SPIRIT)));
    state.active[0].hp = state.active[0].maxHp = 100;
    state.active[1].hp = 1;
}

void rigLose() {
    battle::BattleState &state =
        const_cast<battle::BattleState &>(battleSession().state());
    state.active[0].types =
        DualType(static_cast<Type>(static_cast<uint8_t>(Type::FIRE)));
    state.active[0].attackMask = static_cast<uint8_t>(
        1u << duel::foldType(static_cast<uint8_t>(Type::FIRE)));
    state.active[0].hp = state.active[0].maxHp = 1;
    state.active[1].hp = state.active[1].maxHp = 100;
    // One live member makes the first faint terminal, with no bench promotion.
    state.partyCount[0] = 1;
}

} // namespace duel_boss_test_detail

inline void test_duel_session(FxTest &test) {
    using namespace duel_boss_test_detail;
    const uint8_t id = bossId();

    beginBoss(id);
    battle::BattleState &boss =
        const_cast<battle::BattleState &>(battleSession().state());
    test.expectEq(boss.trainer, 1, F("boss is a trainer"));
    test.expectEq(boss.partyCount[1], 1, F("boss party is one member"));
    test.expectEq(boss.active[1].id, 31, F("boss is ardu"));
    test.expectEq(boss.active[1].level, 8, F("boss is level 8"));
    test.expectEq(static_cast<uint8_t>(battleSession().duelPersona()), 1,
                  F("boss authored persona"));
    test.expectEq(battleSession().duelReadRate(), 25,
                  F("boss authored read rate"));
    test.expectEq(gameState.getFlag(flag_beta_complete), 0,
                  F("entry does not set completion"));

    rigWin();
    FxReadCounter::resetFrame();
    BattleFlow::update(0); // choice entry owns the pick row
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(0),
                  F("choice entry reads nothing"));
    FxReadCounter::resetFrame();
    BattleFlow::update(MENU_EDGE_A); // commit the single attack cell
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(0),
                  F("commit reads nothing"));

    bool earlyFlag = false;
    bool cleanFrames = true;
    for (uint16_t frame = 0;
         frame < 4096 && gameState.state == GameState_t::BATTLE; ++frame) {
        FxReadCounter::resetFrame();
        BattleFlow::update(0);
        if (gameState.state == GameState_t::BATTLE) {
            if (FxReadCounter::count() != 0) cleanFrames = false;
            if (gameState.getFlag(flag_beta_complete)) earlyFlag = true;
        }
    }
    test.expectEq(cleanFrames, 1, F("steady fight frames read nothing"));
    test.expectEq(earlyFlag, 0, F("flag waits for terminal feedback"));
    test.expectEq(gameState.state, GameState_t::WORLD, F("win returns to world"));
    test.expectEq(gameState.getFlag(flag_beta_complete), 1,
                  F("win records completion"));

    // Lose on the same row: completion stays clear and the row is retryable.
    beginBoss(id);
    rigLose();
    for (uint16_t frame = 0;
         frame < 4096 && gameState.state == GameState_t::BATTLE; ++frame) {
        BattleFlow::update(battleSession().awaitingPlayer() ? MENU_EDGE_A : 0);
    }
    test.expectEq(gameState.state, GameState_t::WORLD, F("loss returns to world"));
    test.expectEq(gameState.getFlag(flag_beta_complete), 0,
                  F("loss never records completion"));
}
