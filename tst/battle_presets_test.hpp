#pragma once

#include "test.hpp"
#include "../src/engine/battle/BattlePresets.hpp"
#include "../src/lib/FxReadCounter.hpp"
#include "../src/lib/ReadData.hpp"
#include "../src/player/Player.hpp"

namespace battle_presets_test {

void checkPreset(Test &test, const BattlePresets::Preset &stored)
{
    const BattlePresets::Preset expected = BattlePresets::copyPreset(stored);
    Player party;
    FxReadCounter::resetFrame();
    battle::applyPlayerPreset(party, stored);
    // Each member's attack mask travels in its canonical species record; the
    // host fixture reader records no metadata reads, so the count is zero.
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(0),
                "host bootstrap records no move-descriptor reads");
    test.assert(FxReadCounter::markUpdate(), true,
                "preset transition accounts for all host metadata reads");

    for (uint8_t slot = 0; slot < 3; ++slot) {
        const BattlePresets::Member &member = expected.player[slot];
        const Creature &creature = party.party[slot];
        const CreatureData_t seed = getCreatureFromStore(member.species);
        test.assert(creature.id, member.species, "preset member species");
        test.assert(creature.level, member.level, "preset member level");
        const uint8_t hp = static_cast<uint8_t>(
            2 * member.level + seed.hpSeed * (member.level / 3) + 30);
        test.assert(creature.statlist.hp, hp, "preset level-derived max HP");
        test.assert(party.creatureHPs[slot], hp, "preset starts at full persistent HP");
        test.assert(creature.attackMask, member.attackMask,
                    "preset authored attack mask");
    }

    // Reapplication is a fresh bootstrap, independent of previous HP.
    party.creatureHPs[0] = 1;
    FxReadCounter::resetFrame();
    battle::applyPlayerPreset(party, stored);
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(0),
                "repeated preset has the same read count");
    test.assert(FxReadCounter::markUpdate(), true,
                "repeated preset remains a transition");
    test.assert(party.creatureHPs[0], party.party[0].statlist.hp,
                "repeated preset restores full HP");

    const OpponentSeed trainer = readOpponentSeed(expected.trainerId);
    const uint8_t trainerSpecies[3] = {
        trainer.firstCreature.id, trainer.secondCreature.id,
        trainer.thirdCreature.id,
    };
    for (uint8_t slot = 0; slot < 3; ++slot)
        test.assert(trainerSpecies[slot] < 32, true,
                    "selected trainer has three real species");
}

} // namespace battle_presets_test

void BattlePresetsSuite(TestRunner &runner)
{
    TestSuite suite("Battle Presets Suite");
    Test test(__func__);
    battle_presets_test::checkPreset(test, BattlePresets::opening);
    battle_presets_test::checkPreset(test, BattlePresets::switch_drill);
    // The authored first-beta starter party (jp8.11.24) is the ordinary
    // new-game roster, so pin it through the same loading seam.
    battle_presets_test::checkPreset(test, BattlePresets::beta);
    const BattlePresets::Preset opening = BattlePresets::copyPreset(BattlePresets::opening);
    const BattlePresets::Preset drill = BattlePresets::copyPreset(BattlePresets::switch_drill);
    test.assert(opening.trainerId != drill.trainerId, true,
                "named presets choose distinct canonical trainer rows");
    suite.addTest(test);
    runner.addTestSuite(suite);
}
