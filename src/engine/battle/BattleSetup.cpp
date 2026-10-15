// CreatureGathererFX-ffm: this TU is the sole battle setup FX-reader boundary.
#include "BattleSetup.hpp"
#include "MoveUses.hpp"
#include "../../lib/MoveIds.hpp"
#include "../../lib/ReadData.hpp"
#include "../../lib/FxReadCounter.hpp"
#include "../../player/Player.hpp"
#ifdef CGFX_DUEL_BATTLE
#include "DuelKernel.hpp"
#endif

extern Player player;

namespace battle {
namespace {

constexpr uint8_t EMPTY_MOVE = 255;
#if defined(CGFx_TRAINER_DEMO_EXPANSION) || defined(BATTLE_SIMULATOR)
constexpr uint8_t SPECIES_COUNT = 64;
#else
constexpr uint8_t SPECIES_COUNT = 32;
#endif

#ifdef CGFX_DUEL_BATTLE
// Folded 5-type attack mask of a creature's usable authored attack slots. Stored
// at setup transition (the only place a bench member's moves are resident) and
// aliased into the otherwise-unused moveUsesSpent storage in flag-ON builds, so
// the AI can pick a bench counter with no FX read during Choice.
uint8_t duelAttackMask(const Creature &creature)
{
    uint8_t mask = 0;
    for (uint8_t slot = 0; slot < 4; ++slot) {
        const uint8_t type = creature.moveList[slot].getMoveType();
        if (type < 8) mask |= static_cast<uint8_t>(1u << duel::foldType(type));
    }
    return mask;
}
#endif

void clearCombatant(Combatant &combatant)
{
    combatant.id = 0;
    combatant.types = DualType();
    combatant.level = 0;
    combatant.hp = 0;
    combatant.maxHp = 0;
    combatant.stats.attack = 0;
    combatant.stats.defense = 0;
    combatant.stats.hp = 0;
    combatant.stats.speed = 0;
    combatant.stats.spcAtk = 0;
    combatant.stats.spcDef = 0;
    for (uint8_t slot = 0; slot < 4; ++slot) {
        combatant.moves[slot] = Move();
        combatant.moveIds[slot] = EMPTY_MOVE;
    }
    combatant.status.clearEffects();
    combatant.statMods.clearModifiers();
    combatant.effectTurns = 0;
}

void clearState(BattleState &state)
{
    resetMoveUses(state);
#ifndef CGFX_DUEL_BATTLE
    for (uint8_t side = 0; side < 2; ++side)
        for (uint8_t slot = 0; slot < PARTY_SIZE; ++slot)
            state.partyModifiers[side][slot] = 0;
#endif
    for (uint8_t side = 0; side < 2; ++side) {
        clearCombatant(state.active[side]);
        for (uint8_t slot = 0; slot < PARTY_SIZE - 1; ++slot) {
            state.bench[side][slot] = {};
        }
        state.partyCount[side] = 0;
        state.activeSlot[side] = 0;
    }
    state.gather.progress = 0;
    state.gather.need = 0;
    state.gather.fleeTurns = 0;
    state.gather.tierRate = 0;
    state.gatherable = false;
    state.trainer = false;
    state.over = false;
    state.switchLockMask = 0;
    state.trainerId = 255;
}

void loadSpecies(Creature &creature, uint8_t id, uint8_t level)
{
    const CreatureData_t seed = getCreatureFromStore(id);
    creature.id = seed.id;
    creature.level = level;
    creature.loadTypes(seed);
    creature.setStats(seed);
    creature.loadMoves(seed);
    creature.status.clearEffects();
    creature.statMods.clearModifiers();
}

uint8_t trainerSpeciesForSlot(uint8_t slot, uint8_t sourceSpecies)
{
#ifdef CGFX_TRAINER_DEMO_EXPANSION
    static const uint8_t expansionSpecies[3] = {33, 56, 63};
    return slot < 3 ? expansionSpecies[slot] : sourceSpecies;
#else
    static_cast<void>(slot);
    return sourceSpecies;
#endif
}

void loadTrainerCreature(Creature &creature, const CreatureSeed &seed,
                        uint8_t partySlot)
{
#ifdef CGFX_TRAINER_DEMO_EXPANSION
    const uint8_t species = trainerSpeciesForSlot(partySlot, seed.id);
    if (species != seed.id) {
        const CreatureData_t data = getCreatureFromStore(species);
        creature.id = data.id;
        creature.level = seed.lvl;
        creature.loadTypes(data);
        creature.setStats(data);
        creature.loadMoves(data);
    } else {
        creature.loadFromOpponentSeed(seed);
    }
#else
    creature.loadFromOpponentSeed(seed);
#endif
    creature.status.clearEffects();
    creature.statMods.clearModifiers();
}

const CreatureSeed &trainerSeed(const OpponentSeed &seed, uint8_t index)
{
    switch (index) {
    case 0: return seed.firstCreature;
    case 1: return seed.secondCreature;
    default: return seed.thirdCreature;
    }
}

uint8_t wildNeed(uint8_t level)
{
    const uint16_t raw = static_cast<uint16_t>(8) +
                         static_cast<uint16_t>(level) / 2;
    if (raw < 8) return 8;
    if (raw > 24) return 24;
    return static_cast<uint8_t>(raw);
}

bool initializePlayer(BattleState &state)
{
    uint8_t count = 0;
    while (count < PARTY_SIZE && player.party[count].level != 0) {
        ++count;
    }
    state.partyCount[static_cast<uint8_t>(Side::Player)] = count;
    if (count == 0) {
        return false;
    }

    uint8_t activeSlot = count;
    for (uint8_t slot = 0; slot < count; ++slot) {
        if (player.creatureHPs[slot] != 0) {
            activeSlot = slot;
            break;
        }
    }
    if (activeSlot == count) {
        return false;
    }

    state.activeSlot[static_cast<uint8_t>(Side::Player)] = activeSlot;
    copyCreature(state.active[static_cast<uint8_t>(Side::Player)],
                 player.party[activeSlot], player.creatureHPs[activeSlot]);

    uint8_t benchSlot = 0;
    for (uint8_t slot = 0; slot < count; ++slot) {
        if (slot == activeSlot) continue;
        state.bench[static_cast<uint8_t>(Side::Player)][benchSlot].id =
            player.party[slot].id;
        state.bench[static_cast<uint8_t>(Side::Player)][benchSlot].level =
            player.party[slot].level;
        state.bench[static_cast<uint8_t>(Side::Player)][benchSlot].hp =
            player.creatureHPs[slot];
        const Creature &creature = player.party[slot];
        state.bench[static_cast<uint8_t>(Side::Player)][benchSlot].types = creature.types;
        state.bench[static_cast<uint8_t>(Side::Player)][benchSlot].defense =
            creature.statlist.defense;
        state.bench[static_cast<uint8_t>(Side::Player)][benchSlot].specialDefense =
            creature.statlist.spcDef;
#ifdef CGFX_DUEL_BATTLE
        // Bench attack-type mask, indexed by original slot in flag-ON builds.
        state.moveUsesSpent[static_cast<uint8_t>(Side::Player)][slot] =
            duelAttackMask(creature);
#endif
        ++benchSlot;
    }
    return true;
}

bool validCreatureSeed(const CreatureSeed &seed)
{
    return seed.lvl != 0 && seed.id < SPECIES_COUNT;
}

void setWildState(BattleState &state, bool gatherable, uint8_t tierRate,
                  uint8_t level)
{
    state.gatherable = gatherable;
    state.gather.progress = 0;
    state.gather.need = wildNeed(level);
    state.gather.fleeTurns = 6;
    state.gather.tierRate = gatherable ? tierRate : 0;
}

uint8_t speciesCreatureReads(const Creature &creature)
{
    uint8_t reads = 1;
    for (uint8_t slot = 0; slot < 4; ++slot)
        if (validMoveId(creature.moves[slot])) ++reads;
    return reads;
}

uint8_t trainerCreatureReads(const CreatureSeed &seed)
{
    uint8_t reads = 1; // Species record.
    for (uint8_t move = 0; move < 4; ++move) {
        if (validMoveId(parseOpponentCreatureSeedMove(seed.moves, move))) {
            ++reads;
        }
    }
    return reads;
}

uint8_t fillTrainer(BattleState &state, const OpponentSeed &seed, uint8_t count)
{
    uint8_t reads = 0;
    for (uint8_t slot = 0; slot < count; ++slot) {
        Creature creature;
        const CreatureSeed &authored = trainerSeed(seed, slot);
        loadTrainerCreature(creature, authored, slot);
        reads = static_cast<uint8_t>(reads + trainerCreatureReads(authored));
        if (slot == 0) {
            copyCreature(state.active[static_cast<uint8_t>(Side::Opponent)],
                         creature, creature.statlist.hp);
            continue;
        }
        const uint8_t benchSlot = static_cast<uint8_t>(slot - 1);
        state.bench[static_cast<uint8_t>(Side::Opponent)][benchSlot].id =
            creature.id;
        state.bench[static_cast<uint8_t>(Side::Opponent)][benchSlot].level =
            creature.level;
        state.bench[static_cast<uint8_t>(Side::Opponent)][benchSlot].hp =
            creature.statlist.hp;
        BenchSlot &cached = state.bench[static_cast<uint8_t>(Side::Opponent)][benchSlot];
        cached.types = creature.types;
        cached.defense = creature.statlist.defense;
        cached.specialDefense = creature.statlist.spcDef;
#ifdef CGFX_DUEL_BATTLE
        // Bench attack-type mask, indexed by original slot in flag-ON builds.
        state.moveUsesSpent[static_cast<uint8_t>(Side::Opponent)][slot] =
            duelAttackMask(creature);
#endif
    }
    return reads;
}

// Detailed host/legacy results retain the complete transition view.
#ifndef CGFX_BATTLE_COMPACT_RESULT
void captureTransitionBefore(const BattleState &state, ActionResult &out)
{
    for (uint8_t side = 0; side < 2; ++side) {
        out.speciesBefore[side] = state.active[side].id;
        out.maxHpBefore[side] = state.active[side].maxHp;
        out.hpBefore[side] = state.active[side].hp;
        out.hpAfter[side] = state.active[side].hp;
    }
    out.progressBefore = state.gather.progress;
    out.progressAfter = state.gather.progress;
}

void captureTransitionAfter(const BattleState &state, ActionResult &out)
{
    for (uint8_t side = 0; side < 2; ++side) {
        out.hpAfter[side] = state.active[side].hp;
    }
    out.progressAfter = state.gather.progress;
}
#endif

bool loadIncoming(const BattleState &state, Side side, uint8_t originalSlot,
                  const BenchSlot &slot, Combatant &incoming,
                  uint8_t &reads)
{
    reads = 0;
    if (slot.id >= SPECIES_COUNT || slot.hp == 0) {
        return false;
    }

#ifdef __AVR__
    // The shipping caller is the paused menu's original-slot snapshot. The
    // persistent player roster cannot change before this replacement boundary.
    if (side == Side::Player) {
        copyCreature(incoming, player.party[originalSlot], slot.hp);
        return true;
    }
#else
    // Player records already carry their authored move IDs; reuse that cache
    // when it still names the same original slot. Synthetic/native fixtures
    // fall through to the canonical FX species record.
    if (side == Side::Player && originalSlot < PARTY_SIZE &&
        player.party[originalSlot].id == slot.id &&
        player.party[originalSlot].level == slot.level) {
        copyCreature(incoming, player.party[originalSlot], slot.hp);
        return true;
    }
#endif

    Creature creature;
    if (side == Side::Opponent && state.trainer) {
        const OpponentSeed trainer = readOpponentSeed(state.trainerId);
        const CreatureSeed &authored = trainerSeed(trainer, originalSlot);
#ifndef __AVR__
        // Host fixtures can be synthetic; packaged trainer rows were already
        // validated when the party was counted and loaded.
        if (!validCreatureSeed(authored) ||
            trainerSpeciesForSlot(originalSlot, authored.id) != slot.id ||
            authored.lvl != slot.level) {
            return false;
        }
#endif
        loadTrainerCreature(creature, authored, originalSlot);
        reads = static_cast<uint8_t>(2 + trainerCreatureReads(authored));
        copyCreature(incoming, creature, slot.hp);
        return true;
    }

    loadSpecies(creature, slot.id, slot.level);
    reads = speciesCreatureReads(creature);
    copyCreature(incoming, creature, slot.hp);
    return true;
}

#ifndef __AVR__
bool validSwitchRequest(const BattleState &state, Side side,
                        uint8_t originalSlot)
{
    const uint8_t sideIndex = static_cast<uint8_t>(side);
    const uint8_t count = state.partyCount[sideIndex];
    return !state.over && count > 0 && count <= PARTY_SIZE &&
           originalSlot < count && originalSlot != state.activeSlot[sideIndex] &&
           state.activeSlot[sideIndex] < count;
}
#endif

} // namespace

void copyCreature(Combatant &destination, const Creature &source, uint8_t hp)
{
    clearCombatant(destination);
    destination.id = source.id;
    destination.types = source.types;
    destination.level = source.level;
    destination.hp = hp;
    destination.maxHp = source.statlist.hp;
    destination.stats = source.statlist;
    for (uint8_t slot = 0; slot < 4; ++slot) {
        destination.moves[slot] = source.moveList[slot];
        destination.moveIds[slot] = battleMoveId(source.moves[slot]);
    }
}

void beginWild(BattleState &state, uint8_t creatureId, uint8_t level,
               bool gatherable, uint8_t tierRate)
{
    clearState(state);
    setWildState(state, gatherable, tierRate, level);
    if (!initializePlayer(state) || creatureId >= SPECIES_COUNT || level == 0) {
        state.over = true;
        return;
    }

    Creature creature;
    loadSpecies(creature, creatureId, level);
    copyCreature(state.active[static_cast<uint8_t>(Side::Opponent)],
                 creature, creature.statlist.hp);
    FxReadCounter::transitionExact(speciesCreatureReads(creature));
    state.partyCount[static_cast<uint8_t>(Side::Opponent)] = 1;
    state.activeSlot[static_cast<uint8_t>(Side::Opponent)] = 0;
    state.trainer = false;
}

void beginTrainer(BattleState &state, uint8_t opponentId)
{
    clearState(state);
    state.trainer = true;
    state.trainerId = opponentId;
    if (!initializePlayer(state)) {
        state.over = true;
        return;
    }

    const OpponentSeed seed = readOpponentSeed(opponentId);
    uint8_t count = 0;
    while (count < PARTY_SIZE && validCreatureSeed(trainerSeed(seed, count))) {
        ++count;
    }
    if (count == 0) {
        state.over = true;
        return;
    }

    state.partyCount[static_cast<uint8_t>(Side::Opponent)] = count;
    state.activeSlot[static_cast<uint8_t>(Side::Opponent)] = 0;
    const uint8_t reads = fillTrainer(state, seed, count);
    FxReadCounter::transitionExact(static_cast<uint8_t>(2 + reads));
}

bool applySwitch(BattleState &state, Side side, uint8_t originalSlot,
                 bool forced, ActionResult &out)
{
    resetActionResult(out);
    const uint8_t sideIndex = static_cast<uint8_t>(side);
    out.kind = ResultKind::Switch;
    out.actor = side;
    out.flags = forced ? FORCED_SWITCH : 0;

#ifndef CGFX_BATTLE_COMPACT_RESULT
    // Refused detailed results retain their pre/post transition snapshots.
    if (sideIndex < 2) {
        captureTransitionBefore(state, out);
    }
#endif
#ifndef __AVR__
    if (sideIndex >= 2 || !validSwitchRequest(state, side, originalSlot)) {
        out.flags |= REFUSED;
        return false;
    }
#endif

    const uint8_t activeSlot = state.activeSlot[sideIndex];
    const uint8_t targetBenchSlot = originalSlot < activeSlot
        ? originalSlot : static_cast<uint8_t>(originalSlot - 1);
    const BenchSlot incomingSlot = state.bench[sideIndex][targetBenchSlot];
    if (incomingSlot.hp == 0 || incomingSlot.id >= SPECIES_COUNT) {
        out.flags |= REFUSED;
#ifndef CGFX_BATTLE_COMPACT_RESULT
        captureTransitionAfter(state, out);
#endif
        return false;
    }

    BenchSlot outgoing;
    outgoing.id = state.active[sideIndex].id;
    outgoing.level = state.active[sideIndex].level;
    outgoing.hp = state.active[sideIndex].hp;
    outgoing.types = state.active[sideIndex].types;
    outgoing.defense = state.active[sideIndex].stats.defense;
    outgoing.specialDefense = state.active[sideIndex].stats.spcDef;
    Combatant incoming;
    uint8_t reads = 0;
    if (!loadIncoming(state, side, originalSlot, incomingSlot, incoming, reads)) {
        out.flags |= REFUSED;
#ifndef CGFX_BATTLE_COMPACT_RESULT
        captureTransitionAfter(state, out);
#endif
        return false;
    }
    FxReadCounter::transitionExact(reads);

#ifndef CGFX_DUEL_BATTLE
    state.partyModifiers[sideIndex][state.activeSlot[sideIndex]] =
        state.active[sideIndex].statMods.modifiers;
    incoming.statMods.modifiers = state.partyModifiers[sideIndex][originalSlot];
#endif
    state.active[sideIndex] = incoming;
    state.activeSlot[sideIndex] = originalSlot;
    const uint8_t outgoingBenchSlot = activeSlot < originalSlot
        ? activeSlot : static_cast<uint8_t>(activeSlot - 1);
    if (targetBenchSlot < outgoingBenchSlot) {
        for (uint8_t slot = targetBenchSlot; slot < outgoingBenchSlot; ++slot) {
            state.bench[sideIndex][slot] = state.bench[sideIndex][slot + 1];
        }
    } else {
        for (uint8_t slot = targetBenchSlot; slot > outgoingBenchSlot; --slot) {
            state.bench[sideIndex][slot] = state.bench[sideIndex][slot - 1];
        }
    }
    state.bench[sideIndex][outgoingBenchSlot] = outgoing;
    const uint8_t usedBenchSlots = static_cast<uint8_t>(state.partyCount[sideIndex] - 1);
    if (usedBenchSlots < PARTY_SIZE - 1)
        state.bench[sideIndex][usedBenchSlots] = {};

    out.index = incoming.id;
#ifndef CGFX_BATTLE_COMPACT_RESULT
    out.hpAfter[sideIndex] = state.active[sideIndex].hp;
#endif
    return true;
}

} // namespace battle
