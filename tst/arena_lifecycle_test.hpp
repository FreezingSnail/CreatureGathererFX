#pragma once

#include <cstring>

#include "test.hpp"
#include "src/FXDataFake.hpp"
#include "src/engine/arena/ArenaCatalog.hpp"
#include "src/engine/arena/ArenaDemo.hpp"
#include "src/engine/arena/ArenaNavigation.hpp"
#include "src/engine/ModeState.hpp"
#include "src/engine/menu/MenuV2.hpp"
#include "src/engine/battle/BattleFlow.hpp"
#include "src/fxdata.h"
#include "src/lib/FxReadCounter.hpp"
#include "src/lib/Stats.hpp"
#include "src/player/Player.hpp"
#include "src/globals.hpp"
#include "fxdatatest/generated/arena_demo_data.hpp"
#include "fxdata/generated/arena_demo_ids.hpp"

namespace arena_lifecycle_test_detail {

void setPlayerMembers(uint8_t team)
{
    const uint8_t *source = ArenaDemoFixture::player_blitz;
    if (team == ArenaDemoIds::player_bulwark)
        source = ArenaDemoFixture::player_bulwark;
    else if (team == ArenaDemoIds::player_utility)
        source = ArenaDemoFixture::player_utility;

    fxDataFake::dataBase = ArenaDemoData::playerMembers;
    std::memset(fxDataFake::dataBytes, 0, sizeof(fxDataFake::dataBytes));
    std::memcpy(fxDataFake::dataBytes +
                    static_cast<size_t>(team) * ArenaDemoIds::PlayerBytes,
                source, ArenaDemoIds::PlayerBytes);
}

const uint8_t *playerFixture(uint8_t team)
{
    switch (team) {
    case ArenaDemoIds::player_bulwark: return ArenaDemoFixture::player_bulwark;
    case ArenaDemoIds::player_utility: return ArenaDemoFixture::player_utility;
    default: return ArenaDemoFixture::player_blitz;
    }
}

} // namespace arena_lifecycle_test_detail

inline void ArenaLifecycleSuite(TestRunner &runner)
{
    using namespace arena_lifecycle_test_detail;
    TestSuite suite("Arena mode lifecycle");
    Test test("random-match boot, party reconstruction, and terminal re-roll");

    // The host RNG stub returns 1 for every draw, so boot deterministically
    // rolls player team 1 (bulwark) and opponent trainer 1. Load bulwark's
    // members first so the reconstructed party comes from a known record set.
    player.party[0].status.applyEffect(Effect::CONCUSED);
    player.party[0].statMods.setModifier(StatType::ATTACK_M, 2);
    player.creatureHPs[0] = 1;
    setPlayerMembers(ArenaDemoIds::player_bulwark);

    FxReadCounter::resetFrame();
    arena::boot();
    test.assert(gameState.state, GameState_t::BATTLE,
                "boot rolls the teams and starts the match immediately");
    test.assert(arena::arenaContext.playerTeam, ArenaDemoIds::player_bulwark,
                "boot rolls a valid player team from the shared RNG");
    test.assert(arena::arenaContext.opponentTeam, static_cast<uint8_t>(1),
                "boot rolls a valid opponent trainer from the shared RNG");
    test.assert(arena::arenaContext.playerTeam < ArenaDemoIds::playerCount, true,
                "rolled player team stays inside the catalog");
    test.assert(arena::arenaContext.opponentTeam < ArenaDemoIds::opponentCount,
                true, "rolled opponent stays inside the catalog");
    test.assert(battleSession().isActive(), true,
                "boot starts a live battle session");
    test.assert(battleSession().state().over, false,
                "boot rolled match is not already over");
    test.assert(fxDataFake::lastDataAddress,
                ArenaDemoData::playerMembers +
                    ArenaDemoIds::player_bulwark * ArenaDemoIds::PlayerBytes +
                    2 * ArenaDemoIds::MemberBytes,
                "boot party loader reads the rolled FX member table");

    // The rolled team reconstructs every slot from the canonical creature data,
    // clearing the deliberately dirty status/modifiers/HP.
    for (uint8_t slot = 0; slot < 3; ++slot) {
        const uint8_t offset = static_cast<uint8_t>(slot * ArenaDemoIds::MemberBytes);
        const uint8_t *member = ArenaDemoFixture::player_bulwark + offset;
        const CreatureData_t seed = getCreatureFromStore(member[0]);
        Creature expected;
        expected.id = seed.id;
        expected.level = member[1];
        expected.loadTypes(seed);
        expected.setStats(seed);

        test.assert(player.party[slot].id, member[0],
                    "party species matches the rolled FX member");
        test.assert(player.party[slot].level, member[1],
                    "party level matches the rolled FX member");
        test.assert(player.party[slot].hp, expected.hp,
                    "party HP stat is rebuilt from canonical species data");
        test.assert(player.creatureHPs[slot], expected.hp,
                    "new party HP starts at the canonical maximum");
        test.assert(player.party[slot].attackMask, member[2],
                    "party attack mask matches the rolled FX member");
        test.assert(player.party[slot].status.effects[0], Effect::NONE,
                    "party status clears during the roll's reconstruction");
        test.assert(player.party[slot].statMods.modifiers, static_cast<uint16_t>(0),
                    "party modifiers clear during the roll's reconstruction");
    }

    // The applyPlayerTeam seam still loads any in-range team directly.
    setPlayerMembers(ArenaDemoIds::player_utility);
    FxReadCounter::resetFrame();
    test.assert(arena::applyPlayerTeam(ArenaDemoIds::player_utility, player), true,
                "explicit team load reads the FX member table");
    test.assert(FxReadCounter::markUpdate(), true,
                "explicit team load approves its member reads");
    test.assert(player.party[0].id, ArenaDemoFixture::player_utility[0],
                "explicit team load uses the selected team's first species");
    test.assert(player.party[2].attackMask, ArenaDemoFixture::player_utility[8],
                "explicit team load keeps the selected team's attack mask");

    // Rejected catalog bounds must not read FX or mutate the party/context/UI.
    const uint8_t savedSpecies = player.party[0].id;
    const uint8_t savedLevel = player.party[0].level;
    const uint8_t savedHp = player.creatureHPs[0];
    const uint32_t savedReads = fxDataFake::readCount;
    test.assert(arena::applyPlayerTeam(ArenaDemoIds::playerCount, player), false,
                "out-of-range player team is rejected");
    test.assert(player.party[0].id, savedSpecies,
                "invalid team leaves player species unchanged");
    test.assert(player.party[0].level, savedLevel,
                "invalid team leaves player level unchanged");
    test.assert(player.creatureHPs[0], savedHp,
                "invalid team leaves player HP unchanged");
    test.assert(fxDataFake::readCount, savedReads,
                "invalid team performs no FX reads");

    arena::arenaContext.playerTeam = ArenaDemoIds::playerCount;
    arena::arenaContext.opponentTeam = ArenaDemoIds::opponent_starter;
    menu.menuPointer = 2;
    const arena::ArenaUiState savedUi = modeState.arena.ui;
    const int8_t savedMenuPointer = menu.menuPointer;
    const uint8_t savedState = static_cast<uint8_t>(gameState.state);
    const uint32_t savedContextReads = fxDataFake::readCount;
    test.assert(arena::startMatch(), false,
                "invalid rolled player is rejected before match mutation");
    test.assert(static_cast<uint8_t>(gameState.state), savedState,
                "invalid player selection performs no state mutation");
    test.assert(arena::arenaContext.playerTeam, ArenaDemoIds::playerCount,
                "invalid player selection remains unchanged");
    test.assert(std::memcmp(&modeState.arena.ui, &savedUi, sizeof(savedUi)), 0,
                "invalid selection leaves arena UI unchanged");
    test.assert(menu.menuPointer, savedMenuPointer,
                "invalid selection leaves battle menu unchanged");
    test.assert(fxDataFake::readCount, savedContextReads,
                "invalid player selection performs no FX reads");

    arena::arenaContext.playerTeam = ArenaDemoIds::player_utility;
    arena::arenaContext.opponentTeam = ArenaDemoIds::opponentCount;
    const uint32_t savedOpponentReads = fxDataFake::readCount;
    test.assert(arena::startMatch(), false,
                "invalid rolled opponent is rejected before party mutation");
    test.assert(player.party[0].id, savedSpecies,
                "invalid opponent leaves player party unchanged");
    test.assert(fxDataFake::readCount, savedOpponentReads,
                "invalid opponent selection performs no FX reads");

    // The terminal callback re-rolls both sides and restarts the next match.
    arena::arenaContext.playerTeam = ArenaDemoIds::player_utility;
    arena::arenaContext.opponentTeam = ArenaDemoIds::opponent_champion;
    setPlayerMembers(ArenaDemoIds::player_bulwark);
    FxReadCounter::resetFrame();
    arena::finishBattle(battle::Outcome::Win);
    test.assert(gameState.state, GameState_t::BATTLE,
                "terminal callback re-rolls and starts the next match");
    test.assert(arena::arenaContext.playerTeam, ArenaDemoIds::player_bulwark,
                "terminal callback rolls a fresh player team");
    test.assert(arena::arenaContext.opponentTeam, static_cast<uint8_t>(1),
                "terminal callback rolls a fresh opponent");
    test.assert(battleSession().isActive(), true,
                "terminal callback starts a live battle session");

    suite.addTest(test);
    runner.addTestSuite(suite);
}
