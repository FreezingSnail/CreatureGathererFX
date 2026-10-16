#pragma once

#include "fxtest.hpp"
#include "arena_demo_ids.hpp"
#include "src/engine/arena/ArenaDemo.hpp"
#include "src/engine/ModeState.hpp"
#include "src/engine/battle/BattleSession.hpp"
#include "src/engine/game/Gamestate.hpp"
#include "src/lib/random.hpp"
#include "src/globals.hpp"

namespace arena_demo_test_detail {

// Authored opponent index -> trainer id, matching the generated opponent list.
uint8_t trainerFor(uint8_t opponent)
{
    switch (opponent) {
    case ArenaDemoIds::opponent_speed: return ArenaDemoIds::trainer_speed;
    case ArenaDemoIds::opponent_fortress: return ArenaDemoIds::trainer_fortress;
    case ArenaDemoIds::opponent_tricks: return ArenaDemoIds::trainer_tricks;
    case ArenaDemoIds::opponent_champion: return ArenaDemoIds::trainer_champion;
    default: return ArenaDemoIds::trainer_starter;
    }
}

} // namespace arena_demo_test_detail

// Device proof for the random-match arena demo (CreatureGathererFX-lyd8):
// boot draws both sides from the shared RNG and starts the match immediately,
// the draws repeat for a fixed seed, and every terminal callback re-rolls the
// pair so consecutive rounds differ. These are the first two RNG draws, so the
// expected pair is exact regardless of any hidden-pick draws that battle setup
// later consumes from the same seeded stream.
inline void test_arenademo(FxTest &test)
{
    const uint16_t seed = 0xABCD;

    rngSeed(seed);
    const uint8_t expectPlayer =
        randomRoll(0, static_cast<uint8_t>(ArenaDemoIds::playerCount - 1));
    const uint8_t expectOpponent =
        randomRoll(0, static_cast<uint8_t>(ArenaDemoIds::opponentCount - 1));

    rngSeed(seed);
    arena::boot();
    test.expectEq(static_cast<uint8_t>(gameState.state),
                  static_cast<uint8_t>(GameState_t::BATTLE),
                  F("boot starts the rolled match immediately"));
    test.expectEq(arena::arenaContext.playerTeam, expectPlayer,
                  F("boot draws the player team from the shared RNG"));
    test.expectEq(arena::arenaContext.opponentTeam, expectOpponent,
                  F("boot draws the opponent trainer from the shared RNG"));
    test.expectEq(arena::arenaContext.playerTeam < ArenaDemoIds::playerCount,
                  true, F("rolled player team is inside the catalog"));
    test.expectEq(arena::arenaContext.opponentTeam < ArenaDemoIds::opponentCount,
                  true, F("rolled opponent trainer is inside the catalog"));
    test.expectEq(battleSession().state().partyCount[0], static_cast<uint8_t>(3),
                  F("rolled player team fields a full 3v3"));
    test.expectEq(battleSession().state().partyCount[1], static_cast<uint8_t>(3),
                  F("rolled opponent fields a full 3v3"));
    test.expectEq(static_cast<uint8_t>(battleSession().state().trainerId),
                  arena_demo_test_detail::trainerFor(expectOpponent),
                  F("rolled trainer drives the battle"));

    // The terminal callback re-rolls both sides and starts the next match.
    const uint8_t firstPlayer = arena::arenaContext.playerTeam;
    const uint8_t firstOpponent = arena::arenaContext.opponentTeam;
    arena::finishBattle(battle::Outcome::Win);
    test.expectEq(static_cast<uint8_t>(gameState.state),
                  static_cast<uint8_t>(GameState_t::BATTLE),
                  F("finishBattle starts the next rolled match"));
    test.expectEq(arena::arenaContext.playerTeam < ArenaDemoIds::playerCount,
                  true, F("re-rolled player team is inside the catalog"));
    test.expectEq(arena::arenaContext.opponentTeam < ArenaDemoIds::opponentCount,
                  true, F("re-rolled opponent trainer is inside the catalog"));
    test.expectEq(battleSession().isActive(), true,
                  F("finishBattle starts a live battle session"));
    test.expectEq(arena::arenaContext.playerTeam != firstPlayer ||
                      arena::arenaContext.opponentTeam != firstOpponent,
                  true, F("consecutive rolls advance the shared RNG"));

    // A fresh boot on the same seed reproduces the same pair (testable seam).
    rngSeed(seed);
    arena::boot();
    test.expectEq(arena::arenaContext.playerTeam, expectPlayer,
                  F("same seed reproduces the boot player team"));
    test.expectEq(arena::arenaContext.opponentTeam, expectOpponent,
                  F("same seed reproduces the boot opponent trainer"));
}
