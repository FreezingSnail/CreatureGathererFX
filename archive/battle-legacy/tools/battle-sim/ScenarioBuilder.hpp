#pragma once

#if defined(BATTLE_SIMULATOR) && !defined(__AVR__)

#include "../../src/creature/Creature.hpp"
#include "../../src/engine/battle/BattleState.hpp"
#include "../../src/engine/battle/BattlePresets.hpp"

class Player;

namespace battle_sim {

constexpr uint8_t FULL_HP = 255;
constexpr uint8_t EMPTY_MOVE = 255;

struct MemberSpec {
    uint8_t species;
    uint8_t level;
    // FULL_HP fills from the generated level stats; zero is a fainted bench.
    uint8_t hp;
    uint8_t moveIds[4];

    MemberSpec();
};

struct PartySpec {
    MemberSpec members[PARTY_SIZE];
    uint8_t count;
    uint8_t activeSlot;

    PartySpec();
};

struct SwitchProfile {
    uint8_t type1;
    uint8_t type2;
    uint8_t defense;
    uint8_t specialDefense;
};

struct Scenario {
    battle::BattleState state;
    // Limits for the compressed bench slots, kept outside BattleState and the
    // production BattleSession object.
    uint8_t benchMaxHp[2][PARTY_SIZE - 1];
    // Full host-only player roster for the existing player switch cache.
    Creature playerRoster[PARTY_SIZE];
    uint8_t playerHp[PARTY_SIZE];
    // Simulator cache mirrors the defensive fields frozen for the production
    // BenchSlot profile; indexed by side and original party slot.
    SwitchProfile switchProfiles[2][PARTY_SIZE];
};

enum class BuildError : uint8_t {
    None,
    PartyCount,
    ActiveSlot,
    Species,
    Level,
    HitPoints,
    MoveId,
    NoLiveCreature,
    TrainerId,
};

BuildError build(const PartySpec &player, const PartySpec &opponent,
                 Scenario &out);
BuildError buildPreset(const BattlePresets::Preset &preset, Scenario &out);
MemberSpec speciesMember(uint8_t species, uint8_t level);
void installPlayerParty(const Scenario &scenario, Player &player);

} // namespace battle_sim

#endif
