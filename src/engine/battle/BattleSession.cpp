#include "BattleSession.hpp"

#include "BattleSetup.hpp"
#include "Resolve.hpp"
#include "MoveUses.hpp"
#include "../../player/Player.hpp"
#include "DuelKernel.hpp"
#include "DuelView.hpp"
#if defined(CGFX_DUEL_BATTLE) && defined(__AVR__)
#include "../../fxdata.h"
#include "../../lib/FxRead.hpp"
// Resident chart cache for the canonical AVR duel route; loaded once per
// encounter.
namespace duel {
uint8_t duelChart[TYPE_COUNT * TYPE_COUNT];
}
#endif

#ifndef CGFX_BATTLE_AUTO_REPLACE
#define CGFX_BATTLE_AUTO_REPLACE 0
#endif
#if CGFX_BATTLE_AUTO_REPLACE != 0 && CGFX_BATTLE_AUTO_REPLACE != 1
#error "CGFX_BATTLE_AUTO_REPLACE must be 0 or 1"
#endif

extern Player player;

namespace battle {
namespace {

constexpr uint8_t SIDE_COUNT = 2;
constexpr uint8_t NEXT_COMPLETE = 3;

uint8_t sideIndex(Side side)
{
    return static_cast<uint8_t>(side);
}

BattleAction skipAction()
{
    return {ActionKind::Skip, 255};
}

// Spike-inc.1 adapter (CreatureGathererFX-d73i.1). The authored 8->5 fold runs
// at runtime only because step 4 has not emitted a build-time fold table yet;
// the final authoring fold stays build-time. Attack eligibility mirrors the
// mock: a slot is usable when its authored move type is a real attack type
// (not STATUS/NONE). One option per usable authored slot, in slot order.
// Temporary spike fixture persona/rate mapping so all four planned beta
// personas are reachable and exercised: wild encounters never read (Random/0);
// trainers rotate Reader 25% (introductory), Frequency 50% (ordinary) and
// AntiRepeat 50% (boss) by trainer id. This maps onto real authored encounters
// until step 4 emits authored persona/rate values.
duel::Persona duelFixturePersona(const BattleState &state)
{
    if (!state.trainer) return duel::Persona::Random;
    switch (state.trainerId % 3) {
    case 0: return duel::Persona::Reader;
    case 1: return duel::Persona::Frequency;
    default: return duel::Persona::AntiRepeat;
    }
}

uint8_t duelFixtureReadRate(const BattleState &state)
{
    if (!state.trainer) return 0;
    return state.trainerId % 3 == 0 ? 25 : 50;
}

// At the shipping 52 FPS: 29 ticks = 557.7 ms for the concealed interval, 68
// ticks = 1307.7 ms for the result dwell (the mock's 550 ms / 1300 ms).
constexpr uint8_t DUEL_CONCEAL_TICKS = 29;
constexpr uint8_t DUEL_RESULT_TICKS = 68;

uint8_t duelFoldAuthored(uint8_t authored)
{
    return duel::foldType(authored);
}

// Folded active creature types (1 or 2 distinct) for the matchup modifier.
// Distinctness is checked AFTER folding: wind+water collapse to water and
// elder+spirit collapse to spirit, so those dual types dedupe to one.
uint8_t duelActiveTypes(const BattleState &state, uint8_t side, uint8_t *out)
{
    const DualType types = state.active[side].types;
    const uint8_t first = static_cast<uint8_t>(types.getType1());
    const uint8_t second = static_cast<uint8_t>(types.getType2());
    out[0] = duelFoldAuthored(first);
    const uint8_t foldedSecond = second == static_cast<uint8_t>(Type::NONE)
        ? duel::NO_TYPE : duelFoldAuthored(second);
    if (foldedSecond != duel::NO_TYPE && foldedSecond != out[0]) {
        out[1] = foldedSecond;
        return 2;
    }
    return 1;
}

// Options are the active member's set mask bits in ascending type order.
uint8_t duelUsableOptionCount(const BattleState &state, uint8_t side)
{
    return duel::maskOptionCount(state.active[side].attackMask);
}

uint8_t duelOptionType(const BattleState &state, uint8_t side, uint8_t option)
{
    return duel::maskOptionType(state.active[side].attackMask, option);
}

// First living bench member (original slot) whose cached attack mask beats the
// guessed type; 255 when none. Uses setup-cached masks only, no FX read.
uint8_t duelBenchCounter(const BattleState &state, uint8_t side, uint8_t guessed)
{
    const uint8_t count = state.partyCount[side] > PARTY_SIZE
        ? PARTY_SIZE : state.partyCount[side];
    const uint8_t active = state.activeSlot[side];
    for (uint8_t original = 0; original < count; ++original) {
        if (original == active) continue;
        const uint8_t benchIndex = original < active ? original
                                                     : static_cast<uint8_t>(original - 1);
        if (state.bench[side][benchIndex].hp == 0) continue;
        const uint8_t mask = state.moveUsesSpent[side][original];
        for (uint8_t type = 0; type < duel::TYPE_COUNT; ++type)
            if (((mask >> type) & 1u) &&
                duel::relation(type, guessed) == duel::REL_WIN)
                return original;
    }
    return 255;
}

} // namespace

BattleSession::BattleSession()
    : state_{}, result_{}, cursor_{}, rng_{nullptr}
{
    resetActionResult(result_);
    duelCommittedOpponent_ = {duel::ActionKind::Wait, duel::NO_SLOT};
    duelHistoryCount_ = 0;
    resetCursor(SessionPhase::Inactive);
}

void BattleSession::setRng(Rng rng)
{
    rng_ = rng;
}

void BattleSession::resetCursor(SessionPhase phase)
{
    cursor_.plan.action[0] = skipAction();
    cursor_.plan.action[1] = skipAction();
    cursor_.first = Side::Player;
    cursor_.next = 0;
    cursor_.invalidMask = 0;
    cursor_.flags = 0;
    cursor_.phase = phase;
    // Commit the opponent's concealed pick once on Choice entry and mark the
    // entry so the flow rebuilds its menu on this transition only.
    if (phase == SessionPhase::Choice) {
        duelCommitOpponent();
        cursor_.flags |= 1u;
        state_.feedback = 0; // a fresh choice owns the pick row, not feedback
    }
}

void BattleSession::beginWild(uint8_t id, uint8_t level, bool gatherable,
                              uint8_t tierRate)
{
    ::battle::beginWild(state_, id, level, gatherable, tierRate);
    resetActionResult(result_);
#if defined(CGFX_DUEL_BATTLE) && defined(__AVR__)
    FxRead::bytes(duelChartFixture, duel::duelChart, sizeof(duel::duelChart));
#endif
    duelHistoryCount_ = 0; // history resets per encounter
    state_.round = 1;
    state_.feedback = 0;
    state_.damage[0] = state_.damage[1] = 0;
    state_.reveal[0] = state_.reveal[1] = duel::NO_TYPE;
    resetCursor(state_.over ? SessionPhase::Ready : SessionPhase::Choice);
    if (state_.over) cursor_.next = NEXT_COMPLETE;
    // An empty or fully KO'd player party fails real setup; the duel has no
    // Ready transition, so terminate at entry as a loss. The normal Flow defeat
    // recovery then runs (no reward, no win, no opponent action).
    if (state_.over) {
        result_.outcome = Outcome::Lose;
        cursor_.phase = SessionPhase::Terminal;
    }
}

void BattleSession::beginTrainer(uint8_t id)
{
    ::battle::beginTrainer(state_, id);
    resetActionResult(result_);
#if defined(CGFX_DUEL_BATTLE) && defined(__AVR__)
    FxRead::bytes(duelChartFixture, duel::duelChart, sizeof(duel::duelChart));
#endif
    duelHistoryCount_ = 0; // history resets per encounter
    state_.round = 1;
    state_.feedback = 0;
    state_.damage[0] = state_.damage[1] = 0;
    state_.reveal[0] = state_.reveal[1] = duel::NO_TYPE;
    resetCursor(state_.over ? SessionPhase::Ready : SessionPhase::Choice);
    if (state_.over) cursor_.next = NEXT_COMPLETE;
    // Exhausted player party (or no valid trainer party) at entry: terminate as
    // a loss so the Flow's defeat recovery runs, matching the wild route.
    if (state_.over) {
        result_.outcome = Outcome::Lose;
        cursor_.phase = SessionPhase::Terminal;
    }
}

#ifndef CGFX_BATTLE_COMPACT_RESULT
void BattleSession::captureState(ActionResult &out) const
{
    for (uint8_t side = 0; side < SIDE_COUNT; ++side) {
        out.speciesBefore[side] = state_.active[side].id;
        out.maxHpBefore[side] = state_.active[side].maxHp;
        out.hpBefore[side] = state_.active[side].hp;
        out.hpAfter[side] = state_.active[side].hp;
    }
    out.progressBefore = state_.gather.progress;
    out.progressAfter = state_.gather.progress;
}
#endif

void BattleSession::beginResultIfNeeded()
{
    if (result_.kind != ResultKind::None) return;
    resetActionResult(result_);
#ifndef CGFX_BATTLE_COMPACT_RESULT
    captureState(result_);
#endif
    result_.kind = ResultKind::EndTurn;
    result_.actor = Side::Player;
    result_.outcome = Outcome::Lose;
    state_.over = true;
    cursor_.next = NEXT_COMPLETE;
}

bool BattleSession::needsReplacement(Side side) const
{
    const uint8_t index = sideIndex(side);
    if (index >= SIDE_COUNT || state_.active[index].hp != 0) return false;
    return !sideDefeated(state_, side);
}

uint8_t BattleSession::firstLiveBenchSlot(Side side) const
{
    const uint8_t index = sideIndex(side);
    if (index >= SIDE_COUNT) return 255;

    const uint8_t count = state_.partyCount[index] > PARTY_SIZE
        ? PARTY_SIZE : state_.partyCount[index];
    const uint8_t activeSlot = state_.activeSlot[index];
    uint8_t bench = 0;
    for (uint8_t original = 0; original < count; ++original) {
        if (original == activeSlot) continue;
        if (state_.bench[index][bench].hp != 0) return original;
        ++bench;
    }
    return 255;
}

bool BattleSession::emitReplacement(Side side, uint8_t originalSlot)
{
    const bool switched = applySwitch(state_, side, originalSlot, true, result_);
    if (!switched) cursor_.plan.action[sideIndex(side)] = skipAction();
    cursor_.phase = SessionPhase::Presenting;
    return true;
}

bool BattleSession::submitPlayerAction(BattleAction action)
{
    if (state_.over) return false;
    cursor_.plan.action[static_cast<uint8_t>(Side::Player)] = action;
    // The opponent's concealed pick was already committed on Choice entry; the
    // legacy planner/mover must not run.
    cursor_.plan.action[static_cast<uint8_t>(Side::Opponent)] = skipAction();
    cursor_.first = Side::Player;
    cursor_.invalidMask = 0;
    cursor_.flags = 0;
    if (action.kind == ActionKind::Escape) {
        // Wild escape is immediate: resolve on the acceptance frame, no
        // concealed interval.
        cursor_.next = 0;
        duelResolveRound();
    } else {
        // Accept the pick, then hold the 29-frame concealed interval before
        // revealing and resolving.
        cursor_.next = DUEL_CONCEAL_TICKS;
        cursor_.phase = SessionPhase::Concealed;
    }
    return true;
}

bool BattleSession::submitIntent(MenuIntent intent)
{
    if (cursor_.phase == SessionPhase::Choice) {
        BattleAction action = skipAction();
        switch (intent.kind) {
        case MenuIntentKind::SelectMove:
            // Duel has no PP: the pick-row index selects one of the active
            // member's set mask bits (ascending type order); an index past the
            // option count is not selectable.
            if (intent.index >= duelUsableOptionCount(
                                    state_, static_cast<uint8_t>(Side::Player)))
                return false;
            action = {ActionKind::Attack, intent.index};
            break;
        case MenuIntentKind::SelectParty:
            if (!canSwitch(state_, Side::Player, intent.index)) return false;
            action = {ActionKind::Switch, intent.index};
            break;
        case MenuIntentKind::Pass:
            break;
        case MenuIntentKind::Gather:
            action = {ActionKind::Gather, 0};
            break;
        case MenuIntentKind::Escape:
            action = {ActionKind::Escape, 0};
            break;
        case MenuIntentKind::None:
        case MenuIntentKind::Back:
        default:
            return false;
        }
        return submitPlayerAction(action);
    }

#if !CGFX_BATTLE_AUTO_REPLACE
    if (cursor_.phase == SessionPhase::Replacement &&
        needsReplacement(Side::Player) &&
        intent.kind == MenuIntentKind::SelectParty) {
#ifndef __AVR__
        // Keep direct native intent tests checked; the shipping replacement
        // menu only emits living original slots from this paused phase.
        if (!canSwitch(state_, Side::Player, intent.index)) return false;
#endif
        cursor_.plan.action[static_cast<uint8_t>(Side::Player)] =
            {ActionKind::Switch, intent.index};
        cursor_.phase = SessionPhase::Ready;
        return true;
    }
#endif
    return false;
}

void BattleSession::duelRecordHistory(uint8_t type)
{
    if (duelHistoryCount_ < duel::HISTORY_CAP) {
        duelHistory_[duelHistoryCount_++] = type;
        return;
    }
    for (uint8_t i = 1; i < duel::HISTORY_CAP; ++i)
        duelHistory_[i - 1] = duelHistory_[i];
    duelHistory_[duel::HISTORY_CAP - 1] = type;
}

// Commit the opponent's concealed pick once, on entry to Choice, before the
// player's current choice is accepted. Options are the active member's usable
// authored attack slots; no legacy AI runs in this path.
void BattleSession::duelCommitOpponent()
{
    const uint8_t opponentSide = static_cast<uint8_t>(Side::Opponent);
    duelCommittedOpponent_ = {duel::ActionKind::Wait, duel::NO_SLOT};

    const uint8_t options = duelUsableOptionCount(state_, opponentSide);
    if (options == 0) return; // unusable kit -> WAIT

    const duel::Persona persona = duelFixturePersona(state_);
    const bool reads = persona != duel::Persona::Random &&
                       rng_.roll(100) < duelFixtureReadRate(state_);
    if (reads) {
        const uint8_t guessed = duel::predictPlayerType(
            duelHistory_, duelHistoryCount_, persona, rng_);
        if (guessed != duel::NO_TYPE) {
            uint8_t counters[5];
            uint8_t count = 0;
            for (uint8_t o = 0; o < options; ++o)
                if (duel::relation(duelOptionType(state_, opponentSide, o),
                                   guessed) == duel::REL_WIN)
                    counters[count++] = o;
            if (count != 0) {
                duelCommittedOpponent_ = {duel::ActionKind::Attack,
                                          counters[rng_.roll(count)]};
                return;
            }
            // No counter in the active kit: switch to the first living bench
            // member that holds one, using the setup-cached attack masks.
            const uint8_t bench = duelBenchCounter(state_, opponentSide, guessed);
            if (bench != 255) {
                duelCommittedOpponent_ = {duel::ActionKind::Switch, bench};
                return;
            }
        }
        // Fired read with no counter anywhere: uniform across every attack slot.
        duelCommittedOpponent_ = {duel::ActionKind::Attack, rng_.roll(options)};
        return;
    }

    // No read: prefer the best creature-matchup tier, then uniform.
    uint8_t playerTypes[2];
    const uint8_t playerTypeCount = duelActiveTypes(state_, 0, playerTypes);
    uint8_t bestTier = 0;
    for (uint8_t o = 0; o < options; ++o) {
        const uint8_t tier = duel::creatureModifier(
            duelOptionType(state_, opponentSide, o), playerTypes, playerTypeCount);
        if (tier > bestTier) bestTier = tier;
    }
    uint8_t pool[5];
    uint8_t count = 0;
    for (uint8_t o = 0; o < options; ++o)
        if (duel::creatureModifier(duelOptionType(state_, opponentSide, o),
                                   playerTypes, playerTypeCount) == bestTier)
            pool[count++] = o;
    duelCommittedOpponent_ = {duel::ActionKind::Attack, pool[rng_.roll(count)]};
}

// One atomic round: attacks, switches and WAIT resolve together against the
// same pre-hit state. Returns false only for the still-legacy gather/escape
// bridge, which the caller then handles.
bool BattleSession::duelResolveRound()
{
    const uint8_t playerSide = static_cast<uint8_t>(Side::Player);
    const uint8_t opponentSide = static_cast<uint8_t>(Side::Opponent);
    if (state_.active[playerSide].hp == 0 || state_.active[opponentSide].hp == 0)
        return false;

    const BattleAction playerAction = cursor_.plan.action[playerSide];
    bool playerGathering = false;

    // Escape is a wild-only action (the menu already withholds it from
    // trainers). It is immediate and cancels the pending round.
    if (playerAction.kind == ActionKind::Escape) {
        if (state_.trainer) return false;
        resetActionResult(result_);
        result_.kind = ResultKind::Escape;
        result_.actor = Side::Player;
        result_.outcome = Outcome::Escaped;
        state_.over = true;
        state_.feedback = static_cast<uint8_t>(duel::Feedback::Escape);
        state_.reveal[playerSide] = state_.reveal[opponentSide] = duel::NO_TYPE;
        state_.damage[playerSide] = state_.damage[opponentSide] = 0;
        cursor_.flags |= 2u; // result entry: the flow prepares feedback once
        cursor_.next = DUEL_RESULT_TICKS;
        cursor_.phase = SessionPhase::Result;
        return true;
    }

    duel::Action playerDuel = {duel::ActionKind::Wait, duel::NO_SLOT};
    switch (playerAction.kind) {
    case ActionKind::Gather:
        // Gather availability is a wild-encounter rule; the menu gates it, so a
        // trainer/gatherable-less gather is a no-op rather than a legacy call.
        if (state_.trainer || !state_.gatherable) return false;
        playerGathering = true;
        break;
    case ActionKind::UseItem:
        // No battle item interface; the round commits no attack.
        break;
    case ActionKind::Attack: {
        // playerAction.index is the pick-row option index (a set mask bit in
        // ascending type order); reject an index past the option count.
        if (playerAction.index >= duelUsableOptionCount(state_, playerSide))
            return false;
        playerDuel = {duel::ActionKind::Attack, playerAction.index};
        break;
    }
    case ActionKind::Switch:
        playerDuel = {duel::ActionKind::Switch, playerAction.index};
        break;
    case ActionKind::Skip:
        playerDuel = {duel::ActionKind::Wait, duel::NO_SLOT};
        break;
    default:
        return false;
    }

    const duel::Action opponentDuel = duelCommittedOpponent_;
    const bool playerSwitch = playerDuel.kind == duel::ActionKind::Switch;
    const bool opponentSwitch = opponentDuel.kind == duel::ActionKind::Switch;

    // Remember outgoing original slots so a swap-KO can revert to them.
    uint8_t outgoing[2] = {255, 255};
    if (playerSwitch) outgoing[playerSide] = state_.activeSlot[playerSide];
    if (opponentSwitch) outgoing[opponentSide] = state_.activeSlot[opponentSide];

    // Switches use the setup-owned transition so the incoming member's moves
    // load at the transition boundary, then default wins read the new types.
    if (playerSwitch &&
        !applySwitch(state_, Side::Player, playerDuel.index, false, result_))
        return false;
    if (opponentSwitch &&
        !applySwitch(state_, Side::Opponent, opponentDuel.index, false, result_))
        return false;

    uint8_t attackType[2] = {duel::NO_TYPE, duel::NO_TYPE};
    if (playerDuel.kind == duel::ActionKind::Attack)
        attackType[playerSide] = duelOptionType(state_, playerSide, playerDuel.index);
    if (opponentDuel.kind == duel::ActionKind::Attack)
        attackType[opponentSide] =
            duelOptionType(state_, opponentSide, opponentDuel.index);

    uint8_t playerTypes[2], opponentTypes[2];
    const uint8_t playerTypeCount = duelActiveTypes(state_, playerSide, playerTypes);
    const uint8_t opponentTypeCount =
        duelActiveTypes(state_, opponentSide, opponentTypes);

    uint8_t damage[2] = {0, 0};
    const bool playerAttacks = attackType[playerSide] != duel::NO_TYPE;
    const bool opponentAttacks = attackType[opponentSide] != duel::NO_TYPE;
    if (playerSwitch && opponentSwitch) {
        // Both swaps complete and deal zero damage.
    } else if (playerAttacks && opponentAttacks) {
        const uint8_t rel =
            duel::relation(attackType[playerSide], attackType[opponentSide]);
        const uint8_t playerMod = duel::creatureModifier(
            attackType[playerSide], opponentTypes, opponentTypeCount);
        const uint8_t opponentMod = duel::creatureModifier(
            attackType[opponentSide], playerTypes, playerTypeCount);
        if (rel == duel::REL_WIN) {
            damage[opponentSide] = duel::scaleDamage(duel::DMG_WIN, playerMod);
            damage[playerSide] = duel::scaleDamage(duel::DMG_CHIP, opponentMod);
        } else if (rel == duel::REL_LOSE) {
            damage[playerSide] = duel::scaleDamage(duel::DMG_WIN, opponentMod);
            damage[opponentSide] = duel::scaleDamage(duel::DMG_CHIP, playerMod);
        } else {
            // Trade: each side's outgoing hit scales by its OWN matchup and lands
            // on the opponent, so damage[playerSide] (received) uses the foe's mod
            // and damage[opponentSide] uses the player's mod (mock/duel-bonus).
            damage[opponentSide] = duel::scaleDamage(duel::DMG_TRADE, playerMod);
            damage[playerSide] = duel::scaleDamage(duel::DMG_TRADE, opponentMod);
        }
    } else {
        // Exactly one side lands a default win (the other switched or waited).
        if (playerAttacks) {
            damage[opponentSide] = duel::scaleDamage(
                duel::DMG_WIN, duel::creatureModifier(attackType[playerSide],
                                                      opponentTypes, opponentTypeCount));
        } else if (opponentAttacks) {
            damage[playerSide] = duel::scaleDamage(
                duel::DMG_WIN, duel::creatureModifier(attackType[opponentSide],
                                                      playerTypes, playerTypeCount));
        }
    }

    // Publish the compact round facts the duel UI reveals: both folded move
    // types, both damage amounts, and the caption kind. The player's relation is
    // against the opponent's move; the bonus label rides on the winning side.
    state_.reveal[playerSide] = attackType[playerSide];
    state_.reveal[opponentSide] = attackType[opponentSide];
    state_.damage[playerSide] = damage[playerSide];
    state_.damage[opponentSide] = damage[opponentSide];
    if (playerGathering) {
        state_.feedback = static_cast<uint8_t>(duel::Feedback::Gather);
    } else if (playerSwitch && opponentSwitch) {
        state_.feedback = static_cast<uint8_t>(duel::Feedback::Swap);
    } else if (playerAttacks && opponentAttacks) {
        const uint8_t rel =
            duel::relation(attackType[playerSide], attackType[opponentSide]);
        const uint8_t yourMod = duel::creatureModifier(
            attackType[playerSide], opponentTypes, opponentTypeCount);
        const uint8_t foeMod = duel::creatureModifier(
            attackType[opponentSide], playerTypes, playerTypeCount);
        state_.feedback =
            static_cast<uint8_t>(duel::attackFeedback(rel, yourMod, foeMod));
    } else if (playerAttacks || opponentAttacks) {
        state_.feedback = static_cast<uint8_t>(duel::Feedback::DefaultWin);
    } else {
        state_.feedback = static_cast<uint8_t>(duel::Feedback::Wait);
    }

    // Gather adds tier progress BEFORE the opponent's default win. A KO this
    // round blocks acquisition but never discards the added progress, so a
    // surviving reserve keeps the gathered charge. Need/rate stay frozen.
    if (playerGathering) {
        const uint8_t need = state_.gather.need;
        if (state_.gather.progress < need) {
            const uint16_t updated =
                static_cast<uint16_t>(state_.gather.progress) +
                state_.gather.tierRate;
            state_.gather.progress = updated >= need
                ? need : static_cast<uint8_t>(updated);
        }
    }

    // Capture the round's identity and pre-promotion faint mask before any KO
    // auto-promotion transition, which rewrites its own scratch result.
    const ResultKind roundKind =
        playerGathering ? ResultKind::Gather : ResultKind::Attack;
    const uint8_t roundIndex = playerAction.index;
    for (uint8_t side = 0; side < 2; ++side) {
        Combatant &active = state_.active[side];
        active.hp = damage[side] >= active.hp
            ? 0 : static_cast<uint8_t>(active.hp - damage[side]);
    }
    const bool playerFainted = state_.active[playerSide].hp == 0;
    const bool opponentFainted = state_.active[opponentSide].hp == 0;
    if (attackType[playerSide] != duel::NO_TYPE)
        duelRecordHistory(attackType[playerSide]);

    // KO resolution (the duel contract, independent of CGFX_BATTLE_AUTO_REPLACE):
    // a swap-KO reverts to that side's outgoing member while it lives, and an
    // ordinary KO promotes the first living original slot. A scratch result
    // keeps the atomic round facts from being overwritten by the transition.
    ActionResult promotion;
    resetActionResult(promotion);
    for (uint8_t side = 0; side < 2; ++side) {
        if (state_.active[side].hp != 0) continue;
        uint8_t target = 255;
        if (outgoing[side] != 255 && outgoing[side] != state_.activeSlot[side]) {
            const uint8_t original = outgoing[side];
            const uint8_t bench = original < state_.activeSlot[side]
                ? original : static_cast<uint8_t>(original - 1);
            if (state_.bench[side][bench].hp != 0) target = original;
        }
        if (target == 255)
            target = firstLiveBenchSlot(static_cast<Side>(side));
        if (target != 255)
            applySwitch(state_, static_cast<Side>(side), target, true, promotion);
    }

    // Acquisition blocks on the round's pre-promotion faint mask: an active KO'd
    // this round cannot acquire even though a reserve now stands in. Progress
    // earned before the hit is retained.
    const bool acquisition = playerGathering && state_.gather.need != 0 &&
        state_.gather.progress >= state_.gather.need &&
        !playerFainted && !opponentFainted;

    // Terminal order: KO wins over pending gather, which wins over the wild
    // flee countdown. Every terminal KO outcome marks the battle over.
    resetActionResult(result_);
    result_.kind = roundKind;
    result_.actor = Side::Player;
    result_.index = roundIndex;
    const bool playerAlive = !sideDefeated(state_, Side::Player);
    const bool opponentAlive = !sideDefeated(state_, Side::Opponent);
    if (!playerAlive && !opponentAlive) {
        state_.over = true;
        result_.outcome = Outcome::Draw;
    } else if (!opponentAlive) {
        state_.over = true;
        result_.outcome = Outcome::Win;
    } else if (!playerAlive) {
        state_.over = true;
        result_.outcome = Outcome::Lose;
    } else if (acquisition) {
        state_.over = true;
        result_.outcome = Outcome::Gathered;
    } else if (!state_.trainer && state_.gatherable &&
               !playerFainted && !opponentFainted) {
        // The archived end-turn resolver returns before the flee countdown when
        // either active fainted this round, so the six-round clock pauses on a
        // KO round (progress is retained; acquisition was already blocked).
        if (state_.gather.fleeTurns != 0) --state_.gather.fleeTurns;
        if (state_.gather.fleeTurns == 0) {
            state_.over = true;
            result_.outcome = Outcome::Fled;
        }
    }
    // A terminal outcome owns the footer caption so the result is never hidden:
    // win/defeat/draw/flee and a successful acquisition replace the round
    // spelling; a progress-only or KO-blocked gather keeps its in-progress
    // "gather" caption (mock/duel-bonus.html endText + the plan's reuse of the
    // existing terminal/gather/escape text).
    switch (result_.outcome) {
    case Outcome::Win:
        state_.feedback = static_cast<uint8_t>(duel::Feedback::Win);
        break;
    case Outcome::Lose:
        state_.feedback = static_cast<uint8_t>(duel::Feedback::Lose);
        break;
    case Outcome::Draw:
        state_.feedback = static_cast<uint8_t>(duel::Feedback::Draw);
        break;
    case Outcome::Gathered:
        state_.feedback = static_cast<uint8_t>(duel::Feedback::Gathered);
        break;
    case Outcome::Fled:
        state_.feedback = static_cast<uint8_t>(duel::Feedback::Fled);
        break;
    case Outcome::Escaped:
        state_.feedback = static_cast<uint8_t>(duel::Feedback::Escape);
        break;
    case Outcome::None:
    default:
        break;
    }
    if (playerFainted) result_.flags |= PLAYER_FAINTED;
    if (opponentFainted) result_.flags |= OPPONENT_FAINTED;
    cursor_.flags |= 2u; // result entry: the flow prepares feedback once

    cursor_.next = DUEL_RESULT_TICKS;
    cursor_.phase = SessionPhase::Result;
    return true;
}

bool BattleSession::consumeChoiceEntry()
{
    const bool fresh = (cursor_.flags & 1u) != 0;
    cursor_.flags &= static_cast<uint8_t>(~1u);
    return fresh;
}

bool BattleSession::consumeResultEntry()
{
    const bool entered = (cursor_.flags & 2u) != 0;
    cursor_.flags &= static_cast<uint8_t>(~2u);
    return entered;
}

bool BattleSession::advance()
{
    // Duel timing phases own these frames; no button edges are consumed here.
    // The transition fires exactly on the frame the counter decrements to zero,
    // giving exactly DUEL_CONCEAL_TICKS concealed updates and DUEL_RESULT_TICKS
    // result updates.
    if (cursor_.phase == SessionPhase::Concealed) {
        if (cursor_.next > 0) {
            --cursor_.next;
            if (cursor_.next == 0) duelResolveRound(); // -> SessionPhase::Result
        }
        return false;
    }
    if (cursor_.phase == SessionPhase::Result) {
        if (cursor_.next > 0) {
            --cursor_.next;
            if (cursor_.next == 0) {
                if (result_.outcome != Outcome::None || state_.over) {
                    syncPlayerHp();
                    cursor_.phase = SessionPhase::Terminal;
                } else {
                    // The duel auto-promoted any KO before the dwell, so a
                    // non-terminal round returns straight to a fresh Choice
                    // (not the legacy Ready state). The round counter advances
                    // once per fresh choice.
                    ++state_.round;
                    resetCursor(SessionPhase::Choice);
                }
            }
        }
        return false;
    }
    // The duel has no sequential resolver/AI/status/PP loop to fall through to.
    return false;
}

void BattleSession::finishNormalResult()
{
    // Replacements after EndTurn complete before opening a fresh choice. The
    // cursor stays at COMPLETE while those switch results are presented.
    if (result_.kind == ResultKind::EndTurn || cursor_.next == NEXT_COMPLETE) {
        resetCursor(SessionPhase::Choice);
        return;
    }
    cursor_.phase = SessionPhase::Ready;
}

void BattleSession::finishPresentation()
{
    if (cursor_.phase != SessionPhase::Presenting) return;

    if (result_.outcome != Outcome::None || state_.over) {
        syncPlayerHp();
        cursor_.phase = SessionPhase::Terminal;
        cursor_.next = NEXT_COMPLETE;
        return;
    }

    if (needsReplacement(Side::Player) || needsReplacement(Side::Opponent)) {
        cursor_.plan.action[static_cast<uint8_t>(Side::Player)] = skipAction();
        cursor_.phase = SessionPhase::Replacement;
        return;
    }

    finishNormalResult();
}

const ActionResult &BattleSession::result() const
{
    return result_;
}

bool BattleSession::isActive() const
{
    return cursor_.phase != SessionPhase::Inactive;
}

bool BattleSession::awaitingReplacement() const
{
#if CGFX_BATTLE_AUTO_REPLACE
    return false;
#else
    return cursor_.phase == SessionPhase::Replacement &&
           needsReplacement(Side::Player);
#endif
}

bool BattleSession::isPresenting() const
{
    return cursor_.phase == SessionPhase::Presenting;
}

bool BattleSession::awaitingPlayer() const
{
    return cursor_.phase == SessionPhase::Choice || awaitingReplacement();
}

bool BattleSession::exitReady() const
{
    return cursor_.phase == SessionPhase::Terminal;
}

void BattleSession::syncPlayerHp()
{
    const uint8_t side = static_cast<uint8_t>(Side::Player);
    const uint8_t count = state_.partyCount[side] > PARTY_SIZE
        ? PARTY_SIZE : state_.partyCount[side];
    const uint8_t activeSlot = state_.activeSlot[side];
    uint8_t bench = 0;
    for (uint8_t original = 0; original < count; ++original) {
        if (original == activeSlot) {
            player.creatureHPs[original] = state_.active[side].hp;
        } else {
            player.creatureHPs[original] = state_.bench[side][bench].hp;
            ++bench;
        }
    }
}

BattleView BattleSession::view() const
{
    BattleView out = {};
    for (uint8_t side = 0; side < SIDE_COUNT; ++side) {
        const Combatant &active = state_.active[side];
        out.partyCount[side] = state_.partyCount[side] > PARTY_SIZE
            ? PARTY_SIZE : state_.partyCount[side];
        out.activeSlot[side] = state_.activeSlot[side];
        if (out.partyCount[side] != 0 &&
            out.activeSlot[side] < out.partyCount[side]) {
            out.active[side] = {active.id, active.hp, active.maxHp};
        } else {
            out.active[side] = {255, 0, 0};
        }

        uint8_t bench = 0;
        for (uint8_t original = 0; original < out.partyCount[side]; ++original) {
            BenchSlot slot = {};
            if (original == out.activeSlot[side]) {
                slot = {active.id, active.level, active.hp};
            } else {
                slot = state_.bench[side][bench++];
            }
            out.party[side][original] = {
                slot.id, slot.hp, static_cast<uint8_t>(slot.hp != 0)
            };
        }
    }

    for (uint8_t slot = 0; slot < 4; ++slot) {
        // The duel has no legacy move slots; the pick row reads the resident
        // attack mask directly. Every legacy cell reads empty.
        out.moveIds[slot] = 255;
        out.remainingUses[slot] = 0;
    }
    out.gatherProgress = state_.gather.progress;
    out.gatherNeed = state_.gather.need;
    return out;
}

MoveSnapshot BattleSession::moves() const
{
    MoveSnapshot out = {};
    // The duel pick row reads the resident attack mask directly; the legacy
    // 4-slot move snapshot carries no duel meaning, so every cell is empty.
    for (uint8_t slot = 0; slot < 4; ++slot) out.moveIds[slot] = 255;
    return out;
}

PartySnapshot BattleSession::partyChoices() const
{
    PartySnapshot out = {};
    const uint8_t side = static_cast<uint8_t>(Side::Player);
    const uint8_t count = state_.partyCount[side] > PARTY_SIZE
        ? PARTY_SIZE : state_.partyCount[side];
    const uint8_t activeSlot = state_.activeSlot[side];
    uint8_t bench = 0;
    for (uint8_t original = 0; original < count; ++original) {
        if (original == activeSlot) continue;
        const BenchSlot &slot = state_.bench[side][bench++];
        if (out.count < 2 && slot.hp != 0) {
            out.choices[out.count++] = {slot.id, original, slot.hp};
        }
    }
    out.forced = static_cast<uint8_t>(needsReplacement(Side::Player));
    return out;
}

ReplacementChoices BattleSession::replacementChoices() const
{
    ReplacementChoices out = {};
    const uint8_t side = static_cast<uint8_t>(Side::Player);
    const uint8_t count = state_.partyCount[side] > PARTY_SIZE
        ? PARTY_SIZE : state_.partyCount[side];
    const uint8_t activeSlot = state_.activeSlot[side];
    if (count < 2 || activeSlot >= count) return out;

    uint8_t bench = 0;
    for (uint8_t original = 0; original < count; ++original) {
        if (original == activeSlot) continue;
        const BenchSlot &member = state_.bench[side][bench++];
        if (member.hp == 0) continue;
        out.slots[out.count] = original;
        out.ids[out.count] = member.id;
        ++out.count;
    }
    return out;
}

const BattleState &BattleSession::state() const
{
    return state_;
}

#ifdef TEST
BattleState &BattleSession::stateForTest()
{
    return state_;
}
#endif

} // namespace battle
