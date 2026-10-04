#include "BattleSession.hpp"

#include "Ai.hpp"
#include "BattleSetup.hpp"
#include "Resolve.hpp"
#include "../../player/Player.hpp"

extern Player player;

namespace battle {
namespace {

constexpr uint8_t SIDE_COUNT = 2;
constexpr uint8_t NEXT_END_TURN = 2;
constexpr uint8_t NEXT_COMPLETE = 3;
constexpr uint8_t INVALID_PLAYER = 1u << 0;
constexpr uint8_t INVALID_OPPONENT = 1u << 1;

uint8_t sideIndex(Side side)
{
    return static_cast<uint8_t>(side);
}

Side otherSide(Side side)
{
    return side == Side::Player ? Side::Opponent : Side::Player;
}

uint8_t sideBit(Side side)
{
    return side == Side::Player ? INVALID_PLAYER : INVALID_OPPONENT;
}

BattleAction skipAction()
{
    return {ActionKind::Skip, 255};
}

} // namespace

BattleSession::BattleSession()
    : state_{}, result_{}, cursor_{}, rng_{nullptr}
{
    resetActionResult(result_);
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
}

void BattleSession::beginWild(uint8_t id, uint8_t level, bool gatherable,
                              uint8_t tierRate)
{
    ::battle::beginWild(state_, id, level, gatherable, tierRate);
    resetActionResult(result_);
    resetCursor(state_.over ? SessionPhase::Ready : SessionPhase::Choice);
    if (state_.over) cursor_.next = NEXT_COMPLETE;
}

void BattleSession::beginTrainer(uint8_t id)
{
    ::battle::beginTrainer(state_, id);
    resetActionResult(result_);
    resetCursor(state_.over ? SessionPhase::Ready : SessionPhase::Choice);
    if (state_.over) cursor_.next = NEXT_COMPLETE;
}

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

void BattleSession::beginResultIfNeeded()
{
    if (result_.kind != ResultKind::None) return;
    resetActionResult(result_);
    captureState(result_);
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

bool BattleSession::emitReplacement(Side side, uint8_t originalSlot,
                                    bool forced)
{
    const bool switched = applySwitch(state_, side, originalSlot, forced, result_);
    if (!switched) cursor_.plan.action[sideIndex(side)] = skipAction();
    cursor_.phase = SessionPhase::Presenting;
    return true;
}

bool BattleSession::submitPlayerAction(BattleAction action)
{
    if (state_.over) return false;
    cursor_.plan.action[static_cast<uint8_t>(Side::Player)] = action;
    cursor_.plan.action[static_cast<uint8_t>(Side::Opponent)] =
        chooseAction(state_, Side::Opponent);
    cursor_.first = firstMover(state_, cursor_.plan);
    cursor_.next = 0;
    cursor_.invalidMask = 0;
    cursor_.flags = 0;
    cursor_.phase = SessionPhase::Ready;
    return true;
}

bool BattleSession::submitIntent(MenuIntent intent)
{
    if (cursor_.phase == SessionPhase::Choice) {
        BattleAction action = skipAction();
        switch (intent.kind) {
        case MenuIntentKind::SelectMove:
            action = {ActionKind::Attack, intent.index};
            break;
        case MenuIntentKind::SelectParty:
            if (!canSwitch(state_, Side::Player, intent.index)) return false;
            action = {ActionKind::Switch, intent.index};
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

    if (cursor_.phase == SessionPhase::Replacement &&
        needsReplacement(Side::Player) &&
        intent.kind == MenuIntentKind::SelectParty &&
        canSwitch(state_, Side::Player, intent.index)) {
        cursor_.plan.action[static_cast<uint8_t>(Side::Player)] =
            {ActionKind::Switch, intent.index};
        cursor_.phase = SessionPhase::Ready;
        return true;
    }
    return false;
}

bool BattleSession::advance()
{
    if (cursor_.phase == SessionPhase::Replacement) {
        if (needsReplacement(Side::Player)) {
            if (cursor_.plan.action[static_cast<uint8_t>(Side::Player)].kind !=
                ActionKind::Switch) {
                return false;
            }
        }
        cursor_.phase = SessionPhase::Ready;
    }
    if (cursor_.phase != SessionPhase::Ready) return false;

    if (state_.over) {
        beginResultIfNeeded();
        cursor_.phase = SessionPhase::Presenting;
        return true;
    }

    // Replacement is a transition result and always precedes any remaining
    // frozen actor. Player replacement waits for SelectParty; opponent picks
    // the lowest live original slot without recomputing turn order.
    if (needsReplacement(Side::Player)) {
        const BattleAction action =
            cursor_.plan.action[static_cast<uint8_t>(Side::Player)];
        return emitReplacement(Side::Player, action.index, true);
    }
    if (needsReplacement(Side::Opponent)) {
        const uint8_t original = firstLiveBenchSlot(Side::Opponent);
        if (original == 255) return false;
        cursor_.plan.action[static_cast<uint8_t>(Side::Opponent)] =
            {ActionKind::Switch, original};
        return emitReplacement(Side::Opponent, original, true);
    }

    for (;;) {
        if (cursor_.next >= NEXT_END_TURN) break;
        const Side actor = cursor_.next == 0 ? cursor_.first
                                             : otherSide(cursor_.first);
        const uint8_t actorIndex = sideIndex(actor);
        if ((cursor_.invalidMask & sideBit(actor)) != 0 ||
            state_.active[actorIndex].hp == 0) {
            cursor_.invalidMask |= sideBit(actor);
            ++cursor_.next;
            continue;
        }

        const BattleAction action = cursor_.plan.action[actorIndex];
        if (action.kind == ActionKind::Switch) {
            applySwitch(state_, actor, action.index, false, result_);
        } else {
            resolveAction(state_, actor, action, rng_, result_);
        }

        if (result_.flags & PLAYER_FAINTED) {
            cursor_.invalidMask |= INVALID_PLAYER;
        }
        if (result_.flags & OPPONENT_FAINTED) {
            cursor_.invalidMask |= INVALID_OPPONENT;
        }
        if (result_.kind == ResultKind::Gather &&
            state_.gather.need != 0 &&
            state_.gather.progress >= state_.gather.need) {
            cursor_.flags |= ACQUISITION_PENDING;
        }

        ++cursor_.next;
        if (result_.outcome != Outcome::None || state_.over) {
            cursor_.next = NEXT_COMPLETE;
        }
        cursor_.phase = SessionPhase::Presenting;
        return true;
    }

    if (cursor_.next == NEXT_END_TURN) {
        resolveEndTurn(state_, rng_,
                       (cursor_.flags & ACQUISITION_PENDING) != 0, result_);
        cursor_.next = NEXT_COMPLETE;
        cursor_.phase = SessionPhase::Presenting;
        return true;
    }
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
    return cursor_.phase == SessionPhase::Replacement &&
           needsReplacement(Side::Player);
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
        out.moveIds[slot] = state_.active[static_cast<uint8_t>(Side::Player)]
            .moveIds[slot];
    }
    out.gatherProgress = state_.gather.progress;
    out.gatherNeed = state_.gather.need;
    return out;
}

MoveSnapshot BattleSession::moves() const
{
    MoveSnapshot out = {};
    for (uint8_t slot = 0; slot < 4; ++slot) {
        out.moveIds[slot] = state_.active[static_cast<uint8_t>(Side::Player)]
            .moveIds[slot];
    }
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
