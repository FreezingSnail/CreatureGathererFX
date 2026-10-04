#pragma once

#include <avr/pgmspace.h>
#include <string.h>

#include "fxtest.hpp"
#include "generated/battle_preset_data.hpp"
#include "generated/creature_data.hpp"
#include "generated/move_data.hpp"
#include "generated/opponent_data.hpp"
#include "src/engine/battle/BattlePresets.hpp"
#include "src/lib/FxReadCounter.hpp"
#include "src/lib/ReadData.hpp"

namespace preset_teams_test {

const CreatureSeed &trainerMember(const OpponentSeed &trainer, uint8_t slot)
{
    return slot == 0 ? trainer.firstCreature :
           slot == 1 ? trainer.secondCreature : trainer.thirdCreature;
}

uint8_t logicalBootstrapReads(const BattlePresets::Preset &preset)
{
    uint8_t reads = 3; // One canonical species record per member.
    for (uint8_t member = 0; member < 3; ++member)
        for (uint8_t move = 0; move < 4; ++move)
            if (preset.player[member].moveIds[move] != BattlePresets::emptyMoveId)
                ++reads;
    return reads;
}

void verify(FxTest &test, const BattlePresets::Preset &stored)
{
    const BattlePresets::Preset preset = BattlePresets::copyPreset(stored);
    FxReadCounter::resetFrame();
    battle::applyPlayerPreset(player, stored);
    test.expectEq(FxReadCounter::count(), logicalBootstrapReads(preset),
                  F("preset bootstrap exact FX reads"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("preset reads stay in transition"));

    for (uint8_t slot = 0; slot < 3; ++slot) {
        const BattlePresets::Member &member = preset.player[slot];
        CreatureData_t source;
        memcpy_P(&source, creatureFixtures + member.species, sizeof(source));
        const Creature &actual = player.party[slot];
        test.expectEqIdx(actual.id, source.id, F("preset player species"), slot);
        test.expectEqIdx(actual.level, member.level, F("preset player level"), slot);
        const uint8_t maxHP = static_cast<uint8_t>(
            2 * member.level + source.hpSeed * (member.level / 3) + 30);
        test.expectEqIdx(actual.statlist.hp, maxHP, F("preset player max HP"), slot);
        test.expectEqIdx(player.creatureHPs[slot], maxHP,
                         F("preset player current HP"), slot);
        for (uint8_t position = 0; position < 4; ++position) {
            const uint8_t id = member.moveIds[position];
            test.expectEqIdx(actual.moves[position], id, F("preset player move ID"),
                             static_cast<uint8_t>(slot * 4 + position));
            if (id == BattlePresets::emptyMoveId) {
                test.expectEqIdx(actual.moveList[position].move, 0,
                                 F("preset empty descriptor"),
                                 static_cast<uint8_t>(slot * 4 + position));
                test.expectEqIdx(static_cast<uint8_t>(actual.moveList[position].effect1),
                                 static_cast<uint8_t>(Effect::NONE),
                                 F("preset empty effect"),
                                 static_cast<uint8_t>(slot * 4 + position));
            } else {
                const Move expected(pgm_read_dword(moveFixtures + id));
                test.expectEqIdx(actual.moveList[position].move, expected.move,
                                 F("preset authored descriptor"),
                                 static_cast<uint8_t>(slot * 4 + position));
            }
        }
    }

    player.creatureHPs[0] = 1;
    FxReadCounter::resetFrame();
    battle::applyPlayerPreset(player, stored);
    test.expectEq(FxReadCounter::count(), logicalBootstrapReads(preset),
                  F("preset reapply exact FX reads"));
    test.expectEq(player.creatureHPs[0], player.party[0].statlist.hp,
                  F("preset reapply restores full HP"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("preset reapply transition accounted"));

    OpponentSeed sourceTrainer;
    memcpy_P(&sourceTrainer, opponentSeeds + preset.trainerId,
             sizeof(sourceTrainer));
    FxReadCounter::resetFrame();
    const OpponentSeed actualTrainer = readOpponentSeed(preset.trainerId);
    test.expectEq(FxReadCounter::count(), 2, F("trainer row exact FX reads"));
    test.expectEq(FxReadCounter::transitionExact(2), true,
                  F("trainer row transition accounted"));
    for (uint8_t slot = 0; slot < 3; ++slot) {
        const CreatureSeed &expected = trainerMember(sourceTrainer, slot);
        const CreatureSeed &actual = trainerMember(actualTrainer, slot);
        test.expectEqIdx(actual.id, expected.id, F("trainer authored species"), slot);
        test.expectEqIdx(actual.lvl, expected.lvl, F("trainer authored level"), slot);
        test.expectEqIdx(actual.moves, expected.moves, F("trainer authored moves"), slot);
        test.expectEqIdx(actual.id, preset.player[slot].species,
                         F("trainer matches named matchup"), slot);
    }
    FxReadCounter::resetFrame();
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("ordinary frame makes no preset metadata reads"));
}

} // namespace preset_teams_test

void test_preset_teams(FxTest &test)
{
    const BattlePresets::Preset opening = BattlePresets::copyPreset(BattlePresets::opening);
    const BattlePresets::Preset drill = BattlePresets::copyPreset(BattlePresets::switch_drill);
    test.expectEq(opening.trainerId == drill.trainerId, false,
                  F("named matchups use distinct trainer rows"));
    preset_teams_test::verify(test, BattlePresets::opening);
    preset_teams_test::verify(test, BattlePresets::switch_drill);
}
