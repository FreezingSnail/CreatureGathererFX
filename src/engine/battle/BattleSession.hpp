#pragma once

#include "ActionResult.hpp"
#include "BattleRng.hpp"
#include "BattleState.hpp"
#include "BattleView.hpp"
#include "../menu/MenuIntent.hpp"
#ifdef CGFX_DUEL_BATTLE
#include "DuelKernel.hpp"
#endif

namespace battle {

enum class SessionPhase : uint8_t {
    Choice,
    Concealed,   // duel: hidden commitment, 29 accepted frame updates
    Result,      // duel: fixed 68-frame result dwell
    Ready,
    Presenting,
    Replacement,
    Terminal,
    Inactive,
};

struct TurnCursor {
    TurnPlan plan;
    Side first;
    uint8_t next;
    uint8_t invalidMask;
    uint8_t flags;
    SessionPhase phase;
};

static_assert(sizeof(TurnCursor) == 9, "turn cursor must remain nine bytes");

class BattleSession {
  public:
    BattleSession();

    void beginWild(uint8_t id, uint8_t level, bool gatherable, uint8_t tierRate);
    void beginTrainer(uint8_t id);
#ifdef BATTLE_SIMULATOR
    // Host simulator entry point. Bench HP limits travel with the prepared
    // state so validation can stay data-driven without adding resident fields.
    bool beginPrepared(const BattleState &prepared,
                       const uint8_t (&benchMaxHp)[2][PARTY_SIZE - 1]);
#endif

    void setRng(Rng rng);
    bool submitIntent(MenuIntent intent);
    bool advance();
    const ActionResult &result() const;
    void finishPresentation();

    bool isActive() const;
    bool awaitingPlayer() const;
    bool awaitingReplacement() const;
    bool isPresenting() const;
    bool exitReady() const;

    void syncPlayerHp();
    BattleView view() const;
    MoveSnapshot moves() const;
    PartySnapshot partyChoices() const;
    ReplacementChoices replacementChoices() const;

    const BattleState &state() const;
#ifdef TEST
    BattleState &stateForTest();
#endif

  private:
    BattleState state_;
    ActionResult result_;
    TurnCursor cursor_;
    Rng rng_;

    void resetCursor(SessionPhase phase);
    void beginResultIfNeeded();
    bool needsReplacement(Side side) const;
    bool emitReplacement(Side side, uint8_t originalSlot);
    uint8_t firstLiveBenchSlot(Side side) const;
#ifndef CGFX_BATTLE_COMPACT_RESULT
    void captureState(ActionResult &out) const;
#endif
    bool submitPlayerAction(BattleAction action);
    void finishNormalResult();

#ifdef CGFX_DUEL_BATTLE
    // Spike-inc.1 resident duel state (11 bytes). Options are derived on demand
    // from the active member's usable authored attack slots (status/utility/
    // empty filtered, original slot order), so no duplicate option maps are
    // stored; only the concealed opponent pick and the revealed history are
    // resident.
    duel::Action duelCommittedOpponent_; // committed once on Choice entry
    uint8_t duelHistory_[duel::HISTORY_CAP];
    uint8_t duelHistoryCount_;

    void duelCommitOpponent();
    void duelRecordHistory(uint8_t type);
    bool duelResolveRound();

  public:
    // True once per fresh duel Choice so the flow can rebuild its menu on the
    // transition (transition-time FX reads only, never per frame).
    bool consumeChoiceEntry();
    // True once when the duel result dwell begins, so the flow can prepare the
    // feedback captions on the transition rather than per frame.
    bool consumeResultEntry();
    bool isResultDwell() const { return cursor_.phase == SessionPhase::Result; }
    // True while the feedback UI arm must be drawn: the result dwell and the
    // pending terminal-exit frame (the menu arm is stale by then).
    bool duelFeedbackActive() const {
        return cursor_.phase == SessionPhase::Result ||
               cursor_.phase == SessionPhase::Terminal;
    }
#endif
};

#ifdef __AVR__
#ifdef CGFX_DUEL_BATTLE
// The duel session keeps the existing overlay: BattleSession + 27-byte UI must
// stay within the 191-byte BattleMode / 193-byte ModeState union.
static_assert(sizeof(BattleSession) <= 164, "duel session must fit the battle overlay");
#elif defined(CGFX_BATTLE_COMPACT_RESULT)
static_assert(sizeof(BattleSession) == 142, "compact battle session budget");
#else
static_assert(sizeof(BattleSession) == 163, "battle session AVR budget");
#endif
#endif

} // namespace battle
