#include <avr/pgmspace.h>
#include <string.h>

#include "fxtest.hpp"
#include "arena_data.hpp"
#include "creature_data.hpp"
#include "src/creature/Creature.hpp"
#include "src/lib/ReadData.hpp"

// Teams::StockCreatures::null is the canonical first 5-byte arena row. Each
// fixture row follows it contiguously in fxdata/data/teams.txt.
constexpr uint24_t arenaFixtureBase = Teams::StockCreatures::null;
constexpr uint8_t arenaFixtureRowSize = 5;
constexpr uint8_t arenaFixtureLevel = 17;

uint8_t arenaFixtureStat(uint8_t seed) {
    return 2 * arenaFixtureLevel + seed * (arenaFixtureLevel / 3);
}

void test_arena(FxTest &test) {
    // arenaLoad currently reads data[4] before independently reading the same
    // four move bytes for setMove(). Those first reads are dead; preserve them
    // here as source behavior rather than masking or fixing them in this test.
    for (uint8_t row = 0; row < arenaFixtureCount; ++row) {
        uint8_t arenaRow[arenaFixtureRowSize];
        memcpy_P(arenaRow, arenaFixtures[row], sizeof(arenaRow));

        CreatureData_t creatureSeed;
        memcpy_P(&creatureSeed, creatureFixtures + arenaRow[0], sizeof(creatureSeed));

        Creature creature;
        arenaLoad(&creature, arenaFixtureBase + arenaFixtureRowSize * row,
                  arenaFixtureLevel);

        test.expectEqIdx(creature.id, arenaRow[0], F("arena.id"), row);
        test.expectEqIdx(creature.level, arenaFixtureLevel, F("arena.level"), row);
        for (uint8_t slot = 0; slot < 4; ++slot) {
            test.expectEqIdx(creature.getMove(slot), arenaRow[slot + 1],
                             F("arena.move"), row);
        }

        test.expectEqIdx(static_cast<uint8_t>(creature.types.getType1()), creatureSeed.type1,
                         F("arena.type1"), row);
        test.expectEqIdx(static_cast<uint8_t>(creature.types.getType2()), creatureSeed.type2,
                         F("arena.type2"), row);
        test.expectEqIdx(creature.statlist.attack, arenaFixtureStat(creatureSeed.atkSeed),
                         F("arena.attack"), row);
        test.expectEqIdx(creature.statlist.defense, arenaFixtureStat(creatureSeed.defSeed),
                         F("arena.defense"), row);
        test.expectEqIdx(creature.statlist.speed, arenaFixtureStat(creatureSeed.spdSeed),
                         F("arena.speed"), row);
        test.expectEqIdx(creature.statlist.hp, arenaFixtureStat(creatureSeed.hpSeed) + 30,
                         F("arena.hp"), row);
        test.expectEqIdx(creature.statlist.spcAtk, arenaFixtureStat(creatureSeed.spcAtkSeed),
                         F("arena.spcAtk"), row);
        test.expectEqIdx(creature.statlist.spcDef, arenaFixtureStat(creatureSeed.spcDefSeed),
                         F("arena.spcDef"), row);
    }
}
