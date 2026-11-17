#pragma once
// Shared device-suite fixtures for the ordinary duel session chain.
//
// duel_session_integration_test.hpp is the boss-only proof (jp8.11.13), so the
// device suites that still drive ordinary wild/trainer sessions carry these
// minimal framing helpers here instead of pulling test bodies into that sketch
// or re-growing its program-flash margin. Only fixtures live here: no
// assertions and no behavior change.

#if !defined(CGFX_DUEL_BATTLE) || !defined(CGFX_BATTLE_COMPACT_RESULT) || \
    !defined(CGFX_BATTLE_NO_FACTS)
#error "duel_session_fixture requires the duel compact profile: CGFX_DUEL_BATTLE, CGFX_BATTLE_COMPACT_RESULT, CGFX_BATTLE_NO_FACTS"
#endif

#include <stdint.h>

#include "src/engine/ModeState.hpp"
#include "src/engine/battle/BattlePresets.hpp"
#include "src/engine/battle/BattleSession.hpp"
#include "src/engine/game/Gamestate.hpp"
#include "src/engine/menu/MenuV2.hpp"
#include "src/lib/FxReadCounter.hpp"
#include "src/player/Player.hpp"

namespace duel_session_fixture {

const uint8_t SpiritAuthored = static_cast<uint8_t>(Type::SPIRIT);
const uint8_t FireAuthored = static_cast<uint8_t>(Type::FIRE);

uint8_t rngDraws = 0;

uint8_t scriptedRng(uint8_t bound)
{
    ++rngDraws;
    return bound == 0 ? 0 : static_cast<uint8_t>((rngDraws - 1) % bound);
}

// Install the authored opening kit; suites that need another kit re-apply
// applyPlayerPreset afterwards.
void preparePlayer()
{
    battle::applyPlayerPreset(player, BattlePresets::opening);
}

void beginWildDuel(bool gatherable, uint8_t tierRate)
{
    menu.clear();
    dialogMenu.clear();
    enterBattle();
    gameState.state = GameState_t::BATTLE;
    rngDraws = 0;
    battleSession().setRng({scriptedRng});
    FxReadCounter::resetFrame();
    battleSession().beginWild(4, 1, gatherable, tierRate);
}

// Known matchup on the resident session; only the packed move TYPE is retyped.
__attribute__((noinline)) void configureMatchup(
    uint8_t playerType, uint8_t playerMove, uint8_t opponentType,
    uint8_t opponentMove, uint8_t playerHp, uint8_t opponentHp)
{
    battle::BattleState &state =
        const_cast<battle::BattleState &>(battleSession().state());
    battle::Combatant &hero =
        state.active[static_cast<uint8_t>(battle::Side::Player)];
    hero.types = DualType(static_cast<Type>(playerType));
    hero.hp = hero.maxHp = playerHp;
    hero.attackMask = static_cast<uint8_t>(1u << duel::foldType(playerMove));

    battle::Combatant &foe =
        state.active[static_cast<uint8_t>(battle::Side::Opponent)];
    foe.types = DualType(static_cast<Type>(opponentType));
    foe.hp = foe.maxHp = opponentHp;
    if (foe.attackMask != 0)
        foe.attackMask = static_cast<uint8_t>(1u << duel::foldType(opponentMove));
}

} // namespace duel_session_fixture
