#pragma once

#include "ActionResult.hpp"
#include "BattleRng.hpp"
#include "BattleState.hpp"
#include "BattleView.hpp"
#include "../menu/MenuIntent.hpp"

namespace battle {

enum class SessionPhase : uint8_t {
    Choice,
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

    void setRng(Rng rng);
    bool submitIntent(MenuIntent intent);
    bool advance();
    const ActionResult &result() const;
    void finishPresentation();

    bool isActive() const;
    bool awaitingPlayer() const;
    bool awaitingReplacement() const;
    bool exitReady() const;

    void syncPlayerHp();
    BattleView view() const;
    MoveSnapshot moves() const;
    PartySnapshot partyChoices() const;

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
    bool emitReplacement(Side side, uint8_t originalSlot, bool forced);
    uint8_t firstLiveBenchSlot(Side side) const;
    void captureState(ActionResult &out) const;
    bool submitPlayerAction(BattleAction action);
    void finishNormalResult();
};

#ifdef __AVR__
static_assert(sizeof(BattleSession) == 133, "battle session AVR budget");
#endif

} // namespace battle
