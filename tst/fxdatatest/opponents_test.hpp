#pragma once

#include <avr/pgmspace.h>

#include "fxtest.hpp"
#include "generated/opponent_data.hpp"
#include "src/common.hpp"
#include "src/lib/ReadData.hpp"
#include "src/engine/battle/Battle.hpp"
#include "src/opponent/Opponent.hpp"

void DGF test_opponents(FxTest &t) {
    for (uint8_t index = 0; index < opponentSeedCount; ++index) {
        OpponentSeed fixture;
        memcpy_P(&fixture, opponentSeeds + index, sizeof(fixture));

        Opponent control;
        control.loadOpt(&fixture);
        Opponent fx;
        ReadOpt(&fx, index);

        t.expectEqIdx(fx.party[0].id, control.party[0].id, F("ReadOpt.p0.id"), index);
        t.expectEqIdx(fx.party[1].id, control.party[1].id, F("ReadOpt.p1.id"), index);
        t.expectEqIdx(fx.party[2].id, control.party[2].id, F("ReadOpt.p2.id"), index);
        t.expectEqIdx(fx.party[0].level, control.party[0].level, F("ReadOpt.p0.level"), index);
        t.expectEqIdx(fx.party[1].level, control.party[1].level, F("ReadOpt.p1.level"), index);
        t.expectEqIdx(fx.party[2].level, control.party[2].level, F("ReadOpt.p2.level"), index);
        t.expectEqIdx(fx.levels[0], control.levels[0], F("ReadOpt.level0"), index);
        t.expectEqIdx(fx.levels[1], control.levels[1], F("ReadOpt.level1"), index);
        t.expectEqIdx(fx.levels[2], control.levels[2], F("ReadOpt.level2"), index);
        for (uint8_t move = 0; move < 4; ++move) {
            t.expectEqIdx(fx.party[0].moves[move], control.party[0].moves[move], F("ReadOpt.p0.move"), index);
            t.expectEqIdx(fx.party[1].moves[move], control.party[1].moves[move], F("ReadOpt.p1.move"), index);
            t.expectEqIdx(fx.party[2].moves[move], control.party[2].moves[move], F("ReadOpt.p2.move"), index);
        }

        OpponentSeed seed = readOpponentSeed(index);
        t.expectEqIdx(seed.firstCreature.id, fixture.firstCreature.id, F("readOpponentSeed.p0.id"), index);
        t.expectEqIdx(seed.firstCreature.lvl, fixture.firstCreature.lvl, F("readOpponentSeed.p0.lvl"), index);
        t.expectEqIdx(seed.firstCreature.moves, fixture.firstCreature.moves, F("readOpponentSeed.p0.moves"), index);
        t.expectEqIdx(seed.secondCreature.id, fixture.secondCreature.id, F("readOpponentSeed.p1.id"), index);
        t.expectEqIdx(seed.secondCreature.lvl, fixture.secondCreature.lvl, F("readOpponentSeed.p1.lvl"), index);
        t.expectEqIdx(seed.secondCreature.moves, fixture.secondCreature.moves, F("readOpponentSeed.p1.moves"), index);
        t.expectEqIdx(seed.thirdCreature.id, fixture.thirdCreature.id, F("readOpponentSeed.p2.id"), index);
        t.expectEqIdx(seed.thirdCreature.lvl, fixture.thirdCreature.lvl, F("readOpponentSeed.p2.lvl"), index);
        t.expectEqIdx(seed.thirdCreature.moves, fixture.thirdCreature.moves, F("readOpponentSeed.p2.moves"), index);
    }

    constexpr uint8_t encounterCreature = 4;
    constexpr uint8_t encounterLevel = 17;
    BattleEngine encounter;
    encounter.startEncounter(encounterCreature, encounterLevel);
    CreatureData_t encounterSeed = getCreatureFromStore(encounterCreature);

    t.expectEq(encounter.opponent.party[0].id, encounterCreature, F("encounter.id"));
    t.expectEq(encounter.opponent.party[0].level, encounterLevel, F("encounter.party.level"));
    t.expectEq(encounter.opponent.levels[0], encounterLevel, F("encounter.level"));
    t.expectEq(encounter.opponent.levels[1], 0, F("encounter.empty.level1"));
    t.expectEq(encounter.opponent.levels[2], 0, F("encounter.empty.level2"));
    t.expectEq(encounter.opponent.party[0].statlist.attack,
               2 * encounterLevel + encounterSeed.atkSeed * (encounterLevel / 3),
               F("encounter.attack"));
}
