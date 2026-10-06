#pragma once

#include <cstring>

#include "test.hpp"
#include "src/FXDataFake.hpp"
#include "src/engine/arena/ArenaCatalog.hpp"
#include "src/engine/arena/ArenaDemo.hpp"
#include "src/engine/ModeState.hpp"
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

void setPreviewData()
{
    fxDataFake::dataBase = ArenaDemoData::playerLabelWidths;
    std::memset(fxDataFake::dataBytes, 0, sizeof(fxDataFake::dataBytes));
    for (uint8_t i = 0; i < ArenaDemoIds::playerCount; ++i)
        fxDataFake::dataBytes[i] = 10;
}

} // namespace arena_lifecycle_test_detail

inline void ArenaLifecycleSuite(TestRunner &runner)
{
    using namespace arena_lifecycle_test_detail;
    TestSuite suite("Arena mode lifecycle");
    Test test("arena catalog party reconstruction and terminal return");

    FxReadCounter::resetFrame();
    setPreviewData();
    arena::boot();
    test.assert(gameState.state, GameState_t::ARENA,
                "arena bootstrap enters arena mode");
    test.assert(static_cast<uint8_t>(modeState.arena.ui.screen),
                static_cast<uint8_t>(arena::ArenaScreen::PlayerTeam),
                "arena bootstrap opens player selection");
    test.assert(modeState.arena.ui.list.itemCount, ArenaDemoIds::playerCount,
                "bootstrap list includes every generated player team");
    test.assert(modeState.arena.ui.preview.labelWidth, static_cast<uint8_t>(10),
                "bootstrap caches the initial preview");
    test.assert(FxReadCounter::markUpdate(), true,
                "bootstrap preview reads are transition-approved");

    // Install authored team records over a deliberately dirty persistent
    // party. The canonical creature and move data still come from ReadData.
    player.party[0].status.applyEffect(Effect::CONCUSED);
    player.party[0].statMods.setModifier(StatType::ATTACK_M, 2);
    player.creatureHPs[0] = 1;
    setPlayerMembers(ArenaDemoIds::player_utility);
    test.assert(fxDataFake::dataBase, ArenaDemoData::playerMembers,
                "fake cart base is the player member table");
    test.assert(fxDataFake::dataBytes[ArenaDemoIds::player_utility *
                                      ArenaDemoIds::PlayerBytes],
                ArenaDemoFixture::player_utility[0],
                "fake cart fixture starts with the selected team data");
    arena::Member selectedMember{};
    test.assert(arena::readPlayerMember(ArenaDemoIds::player_utility, 0,
                                        selectedMember), true,
                "selected team member can be read from the fake cart");
    test.assert(selectedMember.species, ArenaDemoFixture::player_utility[0],
                "fake cart contains the last team's first species");
    test.assert(fxDataFake::lastDataAddress,
                ArenaDemoData::playerMembers +
                    ArenaDemoIds::player_utility * ArenaDemoIds::PlayerBytes,
                "party loader reads from the player member address");
    FxReadCounter::resetFrame();
    test.assert(arena::applyPlayerTeam(ArenaDemoIds::player_utility, player), true,
                "last generated player team loads from FX members");
    test.assert(FxReadCounter::markUpdate(), true,
                "party reconstruction approves member, species, and move reads");

    for (uint8_t slot = 0; slot < 3; ++slot) {
        const uint8_t offset = static_cast<uint8_t>(slot * ArenaDemoIds::MemberBytes);
        const uint8_t *member = ArenaDemoFixture::player_utility + offset;
        const CreatureData_t seed = getCreatureFromStore(member[0]);
        Creature expected;
        expected.id = seed.id;
        expected.level = member[1];
        expected.loadTypes(seed);
        expected.setStats(seed);

        test.assert(player.party[slot].id, member[0],
                    "party species matches the selected FX member");
        test.assert(player.party[slot].level, member[1],
                    "party level matches the selected FX member");
        test.assert(player.party[slot].statlist.hp, expected.statlist.hp,
                    "party HP stat is rebuilt from canonical species data");
        test.assert(player.party[slot].statlist.attack, expected.statlist.attack,
                    "party attack stat is rebuilt from canonical species data");
        test.assert(player.creatureHPs[slot], expected.statlist.hp,
                    "new party HP starts at the canonical maximum");
        for (uint8_t moveSlot = 0; moveSlot < 4; ++moveSlot) {
            const uint8_t moveId = member[2 + moveSlot];
            test.assert(player.party[slot].moves[moveSlot], moveId,
                        "party move ID matches the selected FX member");
            if (moveId == LEGACY_EMPTY_MOVE_ID) {
                test.assert(player.party[slot].moveList[moveSlot].move,
                            static_cast<uint16_t>(0),
                            "legacy empty move slot uses a blank descriptor");
            }
        }
        test.assert(player.party[slot].status.effects[0], Effect::NONE,
                    "party status clears during fresh reconstruction");
        test.assert(player.party[slot].statMods.modifiers, static_cast<uint16_t>(0),
                    "party modifiers clear during fresh reconstruction");
    }

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
    const uint32_t savedContextReads = fxDataFake::readCount;
    test.assert(arena::startMatch(), false,
                "invalid selected player is rejected before match mutation");
    test.assert(gameState.state, GameState_t::ARENA,
                "invalid selected player remains in the arena");
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
                "invalid selected opponent is rejected before party mutation");
    test.assert(player.party[0].id, savedSpecies,
                "invalid opponent leaves player party unchanged");
    test.assert(fxDataFake::readCount, savedOpponentReads,
                "invalid opponent selection performs no FX reads");

    arena::arenaContext.playerTeam = ArenaDemoIds::player_utility;
    arena::arenaContext.opponentTeam = ArenaDemoIds::opponent_champion;
    arena::finishBattle(battle::Outcome::Win);
    test.assert(gameState.state, GameState_t::ARENA,
                "terminal callback returns without initializing the overworld");
    test.assert(arena::arenaContext.playerTeam, ArenaDemoIds::player_utility,
                "result return preserves player selection");
    test.assert(arena::arenaContext.opponentTeam, ArenaDemoIds::opponent_champion,
                "result return preserves opponent selection");
    test.assert(static_cast<uint8_t>(modeState.arena.ui.screen),
                static_cast<uint8_t>(arena::ArenaScreen::PlayerTeam),
                "terminal callback returns to team selection");
    test.assert(modeState.arena.ui.list.itemCount, ArenaDemoIds::playerCount,
                "terminal return restores all player teams");

    suite.addTest(test);
    runner.addTestSuite(suite);
}
