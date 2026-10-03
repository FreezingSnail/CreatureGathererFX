// CreatureGathererFX-ffm: this TU is the sole battle setup FX-reader boundary.
#include "BattleSetup.hpp"
#include "../../lib/ReadData.hpp"
#include "../../lib/FxReadCounter.hpp"
#include "../../player/Player.hpp"

extern Player player;

namespace battle {
namespace {

constexpr uint8_t EMPTY_MOVE = 255;
constexpr uint8_t SPECIES_COUNT = 32;

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
}

void clearState(BattleState &state)
{
    for (uint8_t side = 0; side < 2; ++side) {
        clearCombatant(state.active[side]);
        for (uint8_t slot = 0; slot < PARTY_SIZE - 1; ++slot) {
            state.bench[side][slot].id = 0;
            state.bench[side][slot].level = 0;
            state.bench[side][slot].hp = 0;
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
}

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
        destination.moveIds[slot] = source.moves[slot];
    }
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

void loadTrainerCreature(Creature &creature, const CreatureSeed &seed)
{
    creature.loadFromOpponentSeed(seed);
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
    while (count < PARTY_SIZE && player.party[count].id != 0) {
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

void fillTrainer(BattleState &state, const OpponentSeed &seed, uint8_t count)
{
    for (uint8_t slot = 0; slot < count; ++slot) {
        Creature creature;
        loadTrainerCreature(creature, trainerSeed(seed, slot));
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
    }
}

void snapshotParty(const BattleState &state, Side side,
                   BenchSlot (&slots)[PARTY_SIZE])
{
    const uint8_t sideIndex = static_cast<uint8_t>(side);
    const uint8_t count = state.partyCount[sideIndex];
    const uint8_t activeSlot = state.activeSlot[sideIndex];
    uint8_t benchSlot = 0;
    for (uint8_t originalSlot = 0; originalSlot < count; ++originalSlot) {
        if (originalSlot == activeSlot) {
            slots[originalSlot].id = state.active[sideIndex].id;
            slots[originalSlot].level = state.active[sideIndex].level;
            slots[originalSlot].hp = state.active[sideIndex].hp;
        } else {
            slots[originalSlot] = state.bench[sideIndex][benchSlot];
            ++benchSlot;
        }
    }
}

// Switch results carry a complete pre/post view so presentation and the next
// action observe the unchanged side as well as the incoming combatant.
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

uint8_t originalSlotForBench(const BattleState &state, Side side,
                             uint8_t benchIndex)
{
    const uint8_t sideIndex = static_cast<uint8_t>(side);
    const uint8_t count = state.partyCount[sideIndex];
    const uint8_t activeSlot = state.activeSlot[sideIndex];
    uint8_t benchSlot = 0;
    for (uint8_t originalSlot = 0; originalSlot < count; ++originalSlot) {
        if (originalSlot == activeSlot) continue;
        if (benchSlot == benchIndex) return originalSlot;
        ++benchSlot;
    }
    return PARTY_SIZE;
}

bool loadIncoming(Side side, uint8_t originalSlot,
                  const BenchSlot &slot, Combatant &incoming,
                  uint8_t &reads)
{
    reads = 0;
    if (slot.id >= SPECIES_COUNT || slot.hp == 0) {
        return false;
    }

    // Player records already carry their authored move IDs; reuse that cache
    // when it still names the same original slot. Synthetic/native fixtures
    // fall through to the canonical FX species record.
    if (side == Side::Player && originalSlot < PARTY_SIZE &&
        player.party[originalSlot].id == slot.id &&
        player.party[originalSlot].level == slot.level) {
        copyCreature(incoming, player.party[originalSlot], slot.hp);
        return true;
    }

    Creature creature;
    loadSpecies(creature, slot.id, slot.level);
    reads = 5;
    copyCreature(incoming, creature, slot.hp);
    return true;
}

bool validSwitchRequest(const BattleState &state, Side side,
                        uint8_t originalSlot)
{
    const uint8_t sideIndex = static_cast<uint8_t>(side);
    const uint8_t count = state.partyCount[sideIndex];
    return !state.over && count > 0 && count <= PARTY_SIZE &&
           originalSlot < count && originalSlot != state.activeSlot[sideIndex] &&
           state.activeSlot[sideIndex] < count;
}

} // namespace

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
    FxReadCounter::transitionExact(5);
    state.partyCount[static_cast<uint8_t>(Side::Opponent)] = 1;
    state.activeSlot[static_cast<uint8_t>(Side::Opponent)] = 0;
    state.trainer = false;
}

void beginTrainer(BattleState &state, uint8_t opponentId)
{
    clearState(state);
    state.trainer = true;
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
    fillTrainer(state, seed, count);
    FxReadCounter::transitionExact(static_cast<uint8_t>(2 + count * 5));
}

bool applySwitch(BattleState &state, Side side, uint8_t originalSlot,
                 bool forced, ActionResult &out)
{
    resetActionResult(out);
    const uint8_t sideIndex = static_cast<uint8_t>(side);
    out.kind = ResultKind::Switch;
    out.actor = side;
    out.flags = forced ? FORCED_SWITCH : 0;

    // Capture RAM state before validating. Refused requests still need a
    // presentable result, while validation must remain entirely pre-FX.
    if (sideIndex < 2) {
        captureTransitionBefore(state, out);
    }
    if (sideIndex >= 2 || !validSwitchRequest(state, side, originalSlot)) {
        out.flags |= REFUSED;
        return false;
    }

    BenchSlot slots[PARTY_SIZE];
    snapshotParty(state, side, slots);
    const BenchSlot incomingSlot = slots[originalSlot];
    if (incomingSlot.hp == 0 || incomingSlot.id >= SPECIES_COUNT) {
        out.flags |= REFUSED;
        captureTransitionAfter(state, out);
        return false;
    }

    const uint8_t oldId = state.active[sideIndex].id;
    const uint8_t oldMaxHp = state.active[sideIndex].maxHp;
    const uint8_t oldHp = state.active[sideIndex].hp;
    Combatant incoming;
    uint8_t reads = 0;
    if (!loadIncoming(side, originalSlot, incomingSlot, incoming, reads)) {
        out.flags |= REFUSED;
        captureTransitionAfter(state, out);
        return false;
    }
    FxReadCounter::transitionExact(reads);

    state.active[sideIndex] = incoming;
    state.activeSlot[sideIndex] = originalSlot;
    uint8_t benchSlot = 0;
    const uint8_t count = state.partyCount[sideIndex];
    for (uint8_t slot = 0; slot < count; ++slot) {
        if (slot == originalSlot) continue;
        state.bench[sideIndex][benchSlot] = slots[slot];
        ++benchSlot;
    }
    while (benchSlot < PARTY_SIZE - 1) {
        state.bench[sideIndex][benchSlot].id = 0;
        state.bench[sideIndex][benchSlot].level = 0;
        state.bench[sideIndex][benchSlot].hp = 0;
        ++benchSlot;
    }

    out.index = incoming.id;
    out.speciesBefore[sideIndex] = oldId;
    out.maxHpBefore[sideIndex] = oldMaxHp;
    out.hpBefore[sideIndex] = oldHp;
    captureTransitionAfter(state, out);
    return true;
}

void loadActive(BattleState &state, Side side, uint8_t benchIndex)
{
    const uint8_t sideIndex = static_cast<uint8_t>(side);
    if (sideIndex >= 2 || state.partyCount[sideIndex] == 0 ||
        state.partyCount[sideIndex] > PARTY_SIZE ||
        benchIndex >= static_cast<uint8_t>(state.partyCount[sideIndex] - 1)) {
        return;
    }
    const uint8_t originalSlot = originalSlotForBench(state, side, benchIndex);
    if (originalSlot >= PARTY_SIZE) return;
    ActionResult result;
    applySwitch(state, side, originalSlot, false, result);
}

} // namespace battle
