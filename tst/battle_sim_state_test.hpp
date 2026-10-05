#pragma once
#include "../src/lib/MoveIds.hpp"

#include "test.hpp"
#include <avr/pgmspace.h>
#include "../tools/battle-sim/ScenarioBuilder.hpp"
#include "../src/engine/battle/BattleSession.hpp"
#include "../src/player/Player.hpp"
#include "../tst/fxdatatest/generated/creature_data.hpp"
#include "../tst/fxdatatest/generated/move_data.hpp"
#include "../tst/fxdatatest/generated/opponent_data.hpp"

#ifdef BATTLE_SIMULATOR
namespace battle_sim_state_test {

bool sameCombatant(const battle::Combatant &left,
                   const battle::Combatant &right)
{
    if (left.id != right.id || left.level != right.level ||
        left.hp != right.hp || left.maxHp != right.maxHp ||
        left.types.getType1() != right.types.getType1() ||
        left.types.getType2() != right.types.getType2() ||
        left.stats.attack != right.stats.attack ||
        left.stats.defense != right.stats.defense ||
        left.stats.hp != right.stats.hp ||
        left.stats.speed != right.stats.speed ||
        left.stats.spcAtk != right.stats.spcAtk ||
        left.stats.spcDef != right.stats.spcDef ||
        left.statMods.getModifier(StatType::ATTACK_M) !=
            right.statMods.getModifier(StatType::ATTACK_M) ||
        left.status.effects[0] != right.status.effects[0] ||
        left.status.effects[1] != right.status.effects[1]) return false;
    for (uint8_t slot = 0; slot < 4; ++slot) {
        if (left.moveIds[slot] != right.moveIds[slot] ||
            left.moves[slot].move != right.moves[slot].move ||
            left.moves[slot].effect1 != right.moves[slot].effect1 ||
            left.moves[slot].effect2 != right.moves[slot].effect2) return false;
    }
    return true;
}

battle_sim::MemberSpec member(uint8_t species, uint8_t level,
                              uint8_t move0 = battle_sim::EMPTY_MOVE,
                              uint8_t move1 = battle_sim::EMPTY_MOVE)
{
    battle_sim::MemberSpec value;
    value.species = species;
    value.level = level;
    value.hp = battle_sim::FULL_HP;
    value.moveIds[0] = move0;
    value.moveIds[1] = move1;
    return value;
}

void preparedOneVsOne(TestSuite &suite)
{
    using namespace battle_sim;
    Test test(__func__);
    PartySpec player;
    player.count = 1;
    player.members[0] = member(0, 12, DELUGE_MOVE_ID, EMPTY_MOVE);
    PartySpec opponent;
    opponent.count = 1;
    opponent.members[0] = member(1, 12, 0, EMPTY_MOVE);

    Scenario scenario{};
    test.assert(build(player, opponent, scenario), BuildError::None,
                "1v1 builds from generated creature and packed move fixtures");
    const battle::Combatant &active = scenario.state.active[0];
    const CreatureData_t species = creatureFixtures[0];
    test.assert(active.id, static_cast<uint8_t>(0), "species zero is valid");
    test.assert(active.level, static_cast<uint8_t>(12), "prepared level retained");
    test.assert(static_cast<uint8_t>(active.types.getType1()), species.type1,
                "generated primary type matches Creature::loadTypes");
    test.assert(static_cast<uint8_t>(active.types.getType2()), species.type2,
                "generated secondary type matches Creature::loadTypes");
    test.assert(active.stats.attack,
                static_cast<uint8_t>(2 * 12 + species.atkSeed * (12 / 3)),
                "generated attack matches Creature::setStats");
    test.assert(active.stats.hp,
                static_cast<uint8_t>(2 * 12 + species.hpSeed * (12 / 3) + 30),
                "generated HP matches Creature::setStats");
    test.assert(active.hp, active.maxHp, "FULL_HP starts at computed maximum");
    test.assert(active.moveIds[0], DELUGE_MOVE_ID,
                "Deluge has its explicit semantic ID");
    test.assert(active.moves[0].move, Move(moveFixtures[32]).move,
                "Deluge descriptor comes from packed move fixture");
    test.assert(active.moveIds[1], static_cast<uint8_t>(255),
                "move ID 255 is the empty-slot sentinel");
    test.assert(active.moves[1].move, static_cast<uint16_t>(0),
                "empty move has a zero descriptor");

    battle::BattleSession session;
    test.assert(session.beginPrepared(scenario.state, scenario.benchMaxHp), true,
                "prepared state starts the production battle session");
    test.assert(session.awaitingPlayer(), true,
                "prepared session opens at the ordinary choice phase");
    test.assert(session.result().kind, battle::ResultKind::None,
                "prepared start clears prior action result");
    test.assert(session.submitIntent({MenuIntentKind::SelectMove, 0}), true,
                "prepared session accepts an ordinary move choice");
    test.assert(session.advance(), true,
                "prepared session advances through the production resolver");
    test.assert(session.result().kind, battle::ResultKind::Attack,
                "prepared session emits a normal attack result");
    session.finishPresentation();
    test.assert(session.isActive(), true,
                "prepared session stays active after a normal result");
    suite.addTest(test);
}

void preparedThreeVsThreeAndValidation(TestSuite &suite)
{
    using namespace battle_sim;
    Test test(__func__);
    PartySpec player;
    player.count = 3;
    player.activeSlot = 1;
    player.members[0] = member(5, 9, 8);
    player.members[1] = member(4, 12, DELUGE_MOVE_ID);
    player.members[2] = member(3, 7, 12);
    player.members[0].hp = 4;

    PartySpec opponent;
    opponent.count = 3;
    opponent.activeSlot = 2;
    opponent.members[0] = member(8, 11, 31);
    opponent.members[1] = member(7, 10, 30);
    opponent.members[2] = member(6, 8, 29);

    Scenario scenario{};
    test.assert(build(player, opponent, scenario), BuildError::None,
                "3v3 preserves original party slots while compressing benches");
    test.assert(scenario.state.activeSlot[0], static_cast<uint8_t>(1),
                "player active slot retains its original index");
    test.assert(scenario.state.active[0].id, static_cast<uint8_t>(4),
                "selected player member becomes active");
    test.assert(scenario.state.bench[0][0].id, static_cast<uint8_t>(5),
                "player bench starts with original slot zero");
    test.assert(scenario.state.bench[0][0].hp, static_cast<uint8_t>(4),
                "player bench carries requested current HP");
    test.assert(scenario.state.bench[0][0].level, static_cast<uint8_t>(9),
                "player bench retains its generated level input");
    test.assert(scenario.state.bench[0][1].id, static_cast<uint8_t>(3),
                "player bench continues with original slot two");
    test.assert(scenario.state.activeSlot[1], static_cast<uint8_t>(2),
                "opponent active slot retains its original index");
    test.assert(scenario.state.bench[1][0].id, static_cast<uint8_t>(8),
                "opponent compressed bench starts with original slot zero");
    test.assert(scenario.state.bench[1][1].id, static_cast<uint8_t>(7),
                "opponent compressed bench preserves original order");
    test.assert(scenario.state.active[0].stats.hp,
                scenario.state.active[0].maxHp,
                "active stats and maximum HP stay aligned");
    test.assert(scenario.benchMaxHp[0][0] != 0, true,
                "compressed bench carries its generated HP limit");
    const CreatureData_t firstBenchSpecies = creatureFixtures[5];
    test.assert(scenario.benchMaxHp[0][0],
                static_cast<uint8_t>(2 * 9 + firstBenchSpecies.hpSeed * (9 / 3) + 30),
                "bench HP limit uses Creature::setStats at its own level");
    test.assert(scenario.state.bench[0][0].hp <= scenario.benchMaxHp[0][0],
                true, "bench HP does not exceed generated species maximum");

    battle::BattleSession session;
    test.assert(session.beginPrepared(scenario.state, scenario.benchMaxHp), true,
                "valid 3v3 state starts");

    battle::BattleState malformed = scenario.state;
    malformed.partyCount[0] = PARTY_SIZE + 1;
    test.assert(session.beginPrepared(malformed, scenario.benchMaxHp), false,
                "prepared session rejects an oversized party");
    malformed = scenario.state;
    malformed.activeSlot[0] = 3;
    test.assert(session.beginPrepared(malformed, scenario.benchMaxHp), false,
                "prepared session rejects an out-of-range active slot");
    malformed = scenario.state;
    malformed.active[0].id = 32;
    test.assert(session.beginPrepared(malformed, scenario.benchMaxHp), false,
                "prepared session rejects an out-of-range species ID");
    malformed = scenario.state;
    malformed.active[0].hp = static_cast<uint8_t>(malformed.active[0].maxHp + 1);
    test.assert(session.beginPrepared(malformed, scenario.benchMaxHp), false,
                "prepared session rejects active HP above maximum");
    malformed = scenario.state;
    malformed.bench[0][0].hp =
        static_cast<uint8_t>(scenario.benchMaxHp[0][0] + 1);
    test.assert(session.beginPrepared(malformed, scenario.benchMaxHp), false,
                "prepared session rejects bench HP above its supplied limit");
    malformed = scenario.state;
    malformed.active[0].moveIds[0] = 255;
    test.assert(session.beginPrepared(malformed, scenario.benchMaxHp), false,
                "prepared session rejects a descriptor on empty move ID 255");
    test.assert(session.state().active[0].id, static_cast<uint8_t>(4),
                "rejected prepared state leaves the prior session untouched");

    PartySpec invalid = player;
    invalid.count = 0;
    test.assert(build(invalid, opponent, scenario), BuildError::PartyCount,
                "builder rejects an empty party");
    invalid = player;
    invalid.activeSlot = 3;
    test.assert(build(invalid, opponent, scenario), BuildError::ActiveSlot,
                "builder rejects an invalid active slot");
    invalid = player;
    invalid.members[0].species = 32;
    test.assert(build(invalid, opponent, scenario), BuildError::Species,
                "builder rejects an invalid species ID");
    invalid = player;
    invalid.members[0].level = 0;
    test.assert(build(invalid, opponent, scenario), BuildError::Level,
                "builder rejects level zero");
    invalid = player;
    invalid.members[0].moveIds[0] = DELUGE_MOVE_ID + 1;
    test.assert(build(invalid, opponent, scenario), BuildError::MoveId,
                "builder rejects move IDs beyond the packed fixture table");
    invalid = player;
    invalid.members[1].hp = 0;
    test.assert(build(invalid, opponent, scenario), BuildError::NoLiveCreature,
                "builder rejects a fainted selected active member");
    invalid = player;
    invalid.members[0].hp = 254;
    test.assert(build(invalid, opponent, scenario), BuildError::HitPoints,
                "builder rejects explicit HP above generated maximum");
    invalid = player;
    invalid.members[0].hp = FULL_HP;
    test.assert(build(invalid, opponent, scenario), BuildError::None,
                "FULL_HP is resolved against the selected level stats");
    suite.addTest(test);
}

void generatedTrainerPreset(TestSuite &suite)
{
    Test test(__func__);
    battle_sim::Scenario scenario{};
    test.assert(battle_sim::buildPreset(BattlePresets::opening, scenario),
                battle_sim::BuildError::None,
                "generated preset builds with its generated trainer row");
    test.assert(scenario.state.trainer, true,
                "preset state keeps production trainer behavior enabled");
    test.assert(scenario.state.trainerId, static_cast<uint8_t>(0),
                "preset state preserves the generated trainer ID");
    test.assert(scenario.state.partyCount[0], static_cast<uint8_t>(3),
                "preset has a full player party");
    test.assert(scenario.state.active[0].id, static_cast<uint8_t>(0),
                "preset player member zero is active");
    test.assert(scenario.state.active[0].moveIds[0], static_cast<uint8_t>(8),
                "preset authored move remains selected");
    test.assert(scenario.state.active[0].moveIds[1], static_cast<uint8_t>(9),
                "second preset move remains selected");
    test.assert(scenario.state.active[0].moveIds[2], static_cast<uint8_t>(255),
                "preset empty marker 32 translates to runtime sentinel 255");
    test.assert(scenario.state.active[1].id, static_cast<uint8_t>(0),
                "trainer active species comes from generated trainer fixture");
    test.assert(scenario.state.bench[1][0].id, static_cast<uint8_t>(3),
                "trainer bench keeps authored slot one");
    test.assert(scenario.state.bench[1][1].id, static_cast<uint8_t>(6),
                "trainer bench keeps authored slot two");

    installPlayerParty(scenario, player);
    battle::BattleSession prepared;
    battle::BattleSession production;
    test.assert(prepared.beginPrepared(scenario.state, scenario.benchMaxHp), true,
                "simulator scenario enters shared session");
    production.beginTrainer(scenario.state.trainerId);
    bool activeMatches[2] = {true, true};
    bool benchMatches = true;
    for (uint8_t side = 0; side < 2; ++side) {
        activeMatches[side] =
            sameCombatant(prepared.state().active[side],
                          production.state().active[side]);
        if (prepared.state().partyCount[side] !=
            production.state().partyCount[side]) benchMatches = false;
        for (uint8_t slot = 0; slot < PARTY_SIZE - 1; ++slot) {
            const battle::BenchSlot &a = prepared.state().bench[side][slot];
            const battle::BenchSlot &b = production.state().bench[side][slot];
            if (a.id != b.id || a.level != b.level || a.hp != b.hp ||
                a.types.getType1() != b.types.getType1() ||
                a.types.getType2() != b.types.getType2() ||
                a.defense != b.defense || a.specialDefense != b.specialDefense)
                benchMatches = false;
        }
    }
    test.assert(activeMatches[0], true,
                "preset player active data matches production BattleSetup loads");
    test.assert(activeMatches[1], true,
                "preset trainer active data matches production BattleSetup loads");
    test.assert(benchMatches, true,
                "preset bench identities and HP match production BattleSetup");

    BattlePresets::Preset malformed = BattlePresets::copyPreset(
        BattlePresets::opening);
    malformed.trainerId = opponentSeedCount;
    test.assert(battle_sim::buildPreset(malformed, scenario),
                battle_sim::BuildError::TrainerId,
                "preset builder rejects a missing generated trainer row");
    suite.addTest(test);
}

} // namespace battle_sim_state_test

inline void BattleSimulatorStateSuite(TestRunner &runner)
{
    TestSuite suite("Battle simulator prepared state");
    battle_sim_state_test::preparedOneVsOne(suite);
    battle_sim_state_test::preparedThreeVsThreeAndValidation(suite);
    battle_sim_state_test::generatedTrainerPreset(suite);
    runner.addTestSuite(suite);
}
#endif
