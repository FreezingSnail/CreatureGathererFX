#include "BattleFlow.hpp"

#include "BattleFeedback.hpp"
#include "BattleMenu.hpp"
#include "BattleScene.hpp"
#include "BattleSession.hpp"
#include "../ModeState.hpp"
#include "../../GameState.hpp"
#include "../../globals.hpp"
#include "../world/World.hpp"

namespace BattleFlow {
namespace {

void beginFeedback(battle::BattleMode &mode) {
    ::new (static_cast<void *>(&mode.ui.feedback)) battle::BattleFeedback();
    mode.ui.feedback.begin(mode.result(), mode.state());
}

void beginMenu(battle::BattleMode &mode) {
    ::new (static_cast<void *>(&mode.ui.menu)) battle::BattleMenu();
}

void finishBattle(battle::BattleSession &session, TerminalCallback callback) {
    const battle::Outcome outcome = session.result().outcome;
#if defined(TEST) || defined(FX_READ_COUNTER)
    menu.clear();
#endif
#if CGFX_DIALOG_QUEUE_ENABLED
    dialogMenu.clear();
#endif
    if (callback != nullptr) {
        callback(outcome);
        return;
    }
    // Terminal Lose is recoverable: the next world frame restores the occupied
    // party and returns to the authored first-beta start. Deferring past
    // exitBattle keeps the single return edge out of a fresh interaction and
    // lets the world rebuild its motion/property cache first.
    if (outcome == battle::Outcome::Lose) WorldEngine::queueDefeatRecovery();
    exitBattle();
    gameState.state = GameState_t::WORLD;
}

} // namespace

bool update(uint8_t edgeButtons, TerminalCallback onFinished) {
    battle::BattleMode &mode = modeState.battle;
    battle::BattleSession &session = mode;

    if (!session.isActive()) return false;
#if CGFX_DIALOG_QUEUE_ENABLED
    if (dialogMenu.peek()) dialogMenu.clear();
#endif

    // Presentations own every edge until they finish. Reconstructing the menu
    // happens after the boundary, so the completion edge cannot select again.
    if (session.isPresenting()) {
        mode.ui.feedback.update((edgeButtons & MENU_EDGE_A) != 0);
        if (!mode.ui.feedback.done()) return false;

        session.finishPresentation();
        if (session.exitReady()) {
            finishBattle(session, onFinished);
            return true;
        }
        beginMenu(mode);
        return false;
    }

    if (session.awaitingPlayer()) {
        if (!mode.ui.menu.initialized()) {
#if !CGFX_BATTLE_AUTO_REPLACE
            if (session.awaitingReplacement()) {
                mode.ui.menu.beginReplacement(session.replacementChoices());
            } else {
#endif
                const battle::BattleState &state = session.state();
                mode.ui.menu.beginOptions(session.moves(), state.gatherable,
                                          !state.trainer);
#if !CGFX_BATTLE_AUTO_REPLACE
            }
#endif
            return false;
        }

        const MenuIntent intent = mode.ui.menu.update(edgeButtons);
        // The party page caches its living rows on the CHANGE transition.
        if (mode.ui.menu.partyPending())
            mode.ui.menu.beginParty(session.view());
        if (intent.kind != MenuIntentKind::None && session.submitIntent(intent)) {
            // Resolving starts on the next frame; this edge belongs only to the
            // accepted selection and is never replayed into another phase.
        }
        return false;
    }

    // Ready/automatic replacement: advance once. A non-terminal end-turn step
    // has no caption or dwell, so it is acknowledged immediately; everything
    // else hands the compact UI arm to feedback. There is no additional
    // controller phase byte in BattleMode.
    if (session.advance()) {
        const battle::ActionResult &result = session.result();
        if (result.kind == battle::ResultKind::EndTurn &&
            result.outcome == battle::Outcome::None && !session.state().over) {
            session.finishPresentation();
            beginMenu(mode);
            return false;
        }
        beginFeedback(mode);
    }
    return false;
}

} // namespace BattleFlow
