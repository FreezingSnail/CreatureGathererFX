#pragma once

#include "fxtest.hpp"
#include "generated/opponent_data.hpp"
#include "src/lib/ReadData.hpp"

void DGF test_opponents(FxTest &t) {
    for (uint8_t index = 0; index < opponentSeedCount; ++index) {
        OpponentSeed fixture;
        memcpy_P(&fixture, opponentSeeds + index, sizeof(fixture));
        const OpponentSeed actual = readOpponentSeed(index);

        t.expectEqIdx(actual.firstCreature.id, fixture.firstCreature.id,
                      F("readOpponentSeed.p0.id"), index);
        t.expectEqIdx(actual.firstCreature.lvl, fixture.firstCreature.lvl,
                      F("readOpponentSeed.p0.level"), index);
        t.expectEqIdx(actual.firstCreature.attackMask, fixture.firstCreature.attackMask,
                      F("readOpponentSeed.p0.mask"), index);
        t.expectEqIdx(actual.secondCreature.id, fixture.secondCreature.id,
                      F("readOpponentSeed.p1.id"), index);
        t.expectEqIdx(actual.secondCreature.lvl, fixture.secondCreature.lvl,
                      F("readOpponentSeed.p1.level"), index);
        t.expectEqIdx(actual.secondCreature.attackMask, fixture.secondCreature.attackMask,
                      F("readOpponentSeed.p1.mask"), index);
        t.expectEqIdx(actual.thirdCreature.id, fixture.thirdCreature.id,
                      F("readOpponentSeed.p2.id"), index);
        t.expectEqIdx(actual.thirdCreature.lvl, fixture.thirdCreature.lvl,
                      F("readOpponentSeed.p2.level"), index);
        t.expectEqIdx(actual.thirdCreature.attackMask, fixture.thirdCreature.attackMask,
                      F("readOpponentSeed.p2.mask"), index);
    }
}
