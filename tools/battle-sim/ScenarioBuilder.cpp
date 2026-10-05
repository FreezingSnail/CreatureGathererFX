#include "ScenarioBuilder.hpp"

#if defined(BATTLE_SIMULATOR) && !defined(__AVR__)

#include <avr/pgmspace.h>

#include "../../src/creature/Creature.hpp"
#include "../../src/engine/battle/BattleSetup.hpp"
#include "../../src/lib/MoveIds.hpp"
#include "../../src/lib/ReadData.hpp"
#include "../../src/player/Player.hpp"
#include "../../tst/fxdatatest/generated/creature_data.hpp"
#include "../../tst/fxdatatest/generated/move_data.hpp"
#include "../../tst/fxdatatest/generated/opponent_data.hpp"

namespace battle_sim {
namespace {

constexpr uint8_t SIDE_COUNT = 2;
constexpr uint8_t MAX_LEVEL = 31;

uint8_t sideIndex(battle::Side side)
{
    return static_cast<uint8_t>(side);
}

BuildError validateParty(const PartySpec &party)
{
    if (party.count == 0 || party.count > PARTY_SIZE) {
        return BuildError::PartyCount;
    }
    if (party.activeSlot >= party.count) return BuildError::ActiveSlot;

    for (uint8_t slot = 0; slot < party.count; ++slot) {
        const MemberSpec &member = party.members[slot];
        if (member.species >= creatureFixtureCount) return BuildError::Species;
        if (member.level == 0 || member.level > MAX_LEVEL) {
            return BuildError::Level;
        }
        for (uint8_t move = 0; move < 4; ++move) {
            const uint8_t id = member.moveIds[move];
            if (id != EMPTY_MOVE && id != LEGACY_EMPTY_MOVE_ID &&
                !validMoveId(id)) {
                return BuildError::MoveId;
            }
        }
        if (slot == party.activeSlot && member.hp == 0) {
            return BuildError::NoLiveCreature;
        }
    }
    return BuildError::None;
}

BuildError appendParty(const PartySpec &party, battle::Side side,
                       Scenario &scenario)
{
    const uint8_t index = sideIndex(side);
    battle::BattleState &state = scenario.state;
    state.partyCount[index] = party.count;
    state.activeSlot[index] = party.activeSlot;

    uint8_t bench = 0;
    for (uint8_t original = 0; original < party.count; ++original) {
        const MemberSpec &member = party.members[original];
        const CreatureData_t seed = getCreatureFromStore(member.species);
        Creature creature;
        creature.id = seed.id;
        creature.level = member.level;
        creature.loadTypes(seed);
        creature.setStats(seed);
        for (uint8_t move = 0; move < 4; ++move) {
            const uint8_t id = member.moveIds[move];
            creature.setMove(id, move);
        }
        creature.status.clearEffects();
        creature.statMods.clearModifiers();

        scenario.switchProfiles[index][original] = {
            static_cast<uint8_t>(creature.types.getType1()),
            static_cast<uint8_t>(creature.types.getType2()),
            creature.statlist.defense, creature.statlist.spcDef};

        const uint8_t maxHp = creature.statlist.hp;
        const uint8_t hp = member.hp == FULL_HP ? maxHp : member.hp;
        if (hp > maxHp) return BuildError::HitPoints;
        if (side == battle::Side::Player) {
            scenario.playerRoster[original] = creature;
            scenario.playerHp[original] = hp;
        }
        if (original == party.activeSlot) {
            battle::copyCreature(state.active[index], creature, hp);
        } else {
            battle::BenchSlot &slot = state.bench[index][bench];
            slot.id = creature.id;
            slot.level = creature.level;
            slot.hp = hp;
            slot.types = creature.types;
            slot.defense = creature.statlist.defense;
            slot.specialDefense = creature.statlist.spcDef;
            scenario.benchMaxHp[index][bench] = maxHp;
            ++bench;
        }
    }
    return BuildError::None;
}

BuildError buildInternal(const PartySpec &player, const PartySpec &opponent,
                         bool trainer, uint8_t trainerId, Scenario &out)
{
    BuildError error = validateParty(player);
    if (error != BuildError::None) return error;
    error = validateParty(opponent);
    if (error != BuildError::None) return error;

    Scenario next = {};
    error = appendParty(player, battle::Side::Player, next);
    if (error != BuildError::None) return error;
    error = appendParty(opponent, battle::Side::Opponent, next);
    if (error != BuildError::None) return error;
    next.state.trainer = trainer;
    next.state.trainerId = trainer ? trainerId : 255;
    out = next;
    return BuildError::None;
}

const CreatureSeed &trainerMember(const OpponentSeed &seed, uint8_t slot)
{
    switch (slot) {
    case 0: return seed.firstCreature;
    case 1: return seed.secondCreature;
    default: return seed.thirdCreature;
    }
}

PartySpec trainerParty(const OpponentSeed &seed)
{
    PartySpec party;
    party.count = PARTY_SIZE;
    party.activeSlot = 0;
    for (uint8_t slot = 0; slot < PARTY_SIZE; ++slot) {
        const CreatureSeed &source = trainerMember(seed, slot);
        MemberSpec &member = party.members[slot];
        member.species = source.id;
        member.level = source.lvl;
        member.hp = FULL_HP;
        for (uint8_t move = 0; move < 4; ++move) {
            member.moveIds[move] = parseOpponentCreatureSeedMove(source.moves,
                                                                  move);
        }
    }
    return party;
}

} // namespace

MemberSpec::MemberSpec() : species(255), level(0), hp(FULL_HP),
                           moveIds{EMPTY_MOVE, EMPTY_MOVE,
                                   EMPTY_MOVE, EMPTY_MOVE}
{
}

PartySpec::PartySpec() : members{}, count(0), activeSlot(0)
{
}

BuildError build(const PartySpec &player, const PartySpec &opponent,
                 Scenario &out)
{
    return buildInternal(player, opponent, false, 255, out);
}

BuildError buildPreset(const BattlePresets::Preset &stored, Scenario &out)
{
    const BattlePresets::Preset preset = BattlePresets::copyPreset(stored);
    if (preset.trainerId >= opponentSeedCount) return BuildError::TrainerId;

    PartySpec player;
    player.count = PARTY_SIZE;
    player.activeSlot = 0;
    for (uint8_t slot = 0; slot < PARTY_SIZE; ++slot) {
        const BattlePresets::Member &source = preset.player[slot];
        MemberSpec &member = player.members[slot];
        member.species = source.species;
        member.level = source.level;
        member.hp = FULL_HP;
        for (uint8_t move = 0; move < 4; ++move) {
            const uint8_t id = source.moveIds[move];
            // Preset data historically uses 32 for an empty slot. Scenario
            // move lists use 255, leaving packed move ID 32 valid elsewhere.
            member.moveIds[move] = id == BattlePresets::emptyMoveId
                ? EMPTY_MOVE : id;
        }
    }

    const OpponentSeed trainer = opponentSeeds[preset.trainerId];
    const PartySpec opponent = trainerParty(trainer);
    return buildInternal(player, opponent, true, preset.trainerId, out);
}

MemberSpec speciesMember(uint8_t species, uint8_t level)
{
    MemberSpec member;
    member.species = species;
    member.level = level;
    member.hp = FULL_HP;
    if (species < creatureFixtureCount) {
        const CreatureData_t seed = getCreatureFromStore(species);
        member.moveIds[0] = seed.move1;
        member.moveIds[1] = seed.move2;
        member.moveIds[2] = seed.move3;
        member.moveIds[3] = seed.move4;
    }
    return member;
}

void installPlayerParty(const Scenario &scenario, Player &player)
{
    Creature roster[PARTY_SIZE];
    uint8_t hp[PARTY_SIZE] = {};
    const uint8_t count = scenario.state.partyCount[
        static_cast<uint8_t>(battle::Side::Player)];
    for (uint8_t slot = 0; slot < PARTY_SIZE; ++slot) {
        if (slot < count) {
            roster[slot] = scenario.playerRoster[slot];
            hp[slot] = scenario.playerHp[slot];
            continue;
        }
        roster[slot].id = 0;
        roster[slot].types = DualType();
        roster[slot].level = 0;
        for (uint8_t move = 0; move < 4; ++move) {
            roster[slot].moves[move] = EMPTY_MOVE;
            roster[slot].moveList[move] = Move();
        }
        roster[slot].status.clearEffects();
        roster[slot].statMods.clearModifiers();
    }
    player.restore(roster, hp);
}

} // namespace battle_sim

#endif
