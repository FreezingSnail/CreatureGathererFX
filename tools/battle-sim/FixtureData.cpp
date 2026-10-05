#include <avr/pgmspace.h>

#include "../../src/lib/ReadData.hpp"
#include "../../src/lib/MoveIds.hpp"
#include "../../src/item/ConsumableDef.hpp"
#include "../../tst/fxdatatest/generated/creature_data.hpp"
#include "../../tst/fxdatatest/generated/move_data.hpp"
#include "../../tst/fxdatatest/generated/opponent_data.hpp"

Move readMoveFX(uint8_t index)
{
    index = moveRecordIndex(index);
    return index < moveFixtureCount ? Move(moveFixtures[index]) : Move();
}

CreatureData_t getCreatureFromStore(uint8_t id)
{
    return id < creatureFixtureCount ? creatureFixtures[id] : CreatureData_t{};
}

OpponentSeed readOpponentSeed(uint8_t id)
{
    return id < opponentSeedCount ? opponentSeeds[id] : OpponentSeed{};
}

namespace item {

ConsumableDef readConsumableDef(uint8_t)
{
    return {static_cast<uint8_t>(ConsumableKind::None), 0};
}

} // namespace item
