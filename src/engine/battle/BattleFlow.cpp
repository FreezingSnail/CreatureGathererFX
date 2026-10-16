#include "BattleFlow.hpp"

#include "BattleFeedback.hpp"
#include "BattleMenu.hpp"
#include "BattleScene.hpp"
#include "BattleSession.hpp"
#include "../ModeState.hpp"
#include "../../GameState.hpp"
#include "../../globals.hpp"
#include "../../creature/LevelCurve.hpp"
#include "../../save/Compaction.hpp"
#include "../../item/Inventory.hpp"
#include "../../lib/FxRead.hpp"
#include "../../fxdata.h"
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

#ifdef CGFX_DUEL_BATTLE
// Construct the correct UI union arm in the SAME update the session enters the
// phase, so the frame's draw never reads a stale arm. Returns true when a fresh
// arm was built this frame (the caller then consumes no button edge).
bool duelSyncUi(battle::BattleMode &mode, battle::BattleSession &session) {
    if (session.consumeResultEntry()) {
        // The result footer is drawn by BattleScene directly from the resident
        // round facts, so no feedback arm is constructed; the entry still owns
        // this frame's edge.
        return true;
    }
    if (session.consumeChoiceEntry()) {
        beginMenu(mode);
        const battle::BattleState &state = session.state();
        mode.ui.menu.beginDuel(state, state.gatherable, !state.trainer);
        return true;
    }
    return false;
}
#endif

// Commit a gathered wild instance to the existing 64-slot collection. The foe is
// read from the still-resident session before exitBattle destroys the battle
// union; a refused add (full 64 or a journal/index/log refusal) leaves the
// collection untouched. HP uses the pinned byte7 encoding: 0 is the legacy/full
// interpretation, otherwise the stored value is HP - 1.
void commitGathered(const battle::BattleSession &session) {
    const battle::BattleState &state = session.state();
    const battle::Combatant &foe =
        state.active[static_cast<uint8_t>(battle::Side::Opponent)];
    StoreRecord record = {};
    record.id = foe.id;
    record.exp = levelExpThreshold(foe.level);
    record.attackMask = foe.attackMask;
    record.hpEncoded =
        foe.hp >= foe.maxHp ? 0 : static_cast<uint8_t>(foe.hp + 1);
    uint8_t slot = 255;
    storeAdd(record, slot);
}

// jp8.11.4: an ordinary non-gatherable, non-trainer wild KO yields the foe
// species' authored material exactly once at the terminal edge. The item id and
// count come from the generated Rewards mapping (0xFF item sentinel = no
// material); Gathered/Fled/Escaped/Lose/Draw, gatherable-target KOs and
// trainer/boss victories award nothing. A full bag saturates via inventoryAdd and
// never corrupts state.
void awardWildKoMaterial(const battle::BattleState &state) {
    if (state.trainer || state.gatherable) return;
    item::Inventory *bag = item::battleInventory();
    if (bag == nullptr) return;
    const uint8_t species =
        state.active[static_cast<uint8_t>(battle::Side::Opponent)].id;
    uint8_t itemId = 0xFF;
    FxRead::bytes(Rewards::materialItem + species, &itemId, 1);
    if (itemId == 0xFF) return; // 0xFF sentinel: no authored material
    uint8_t count = 0;
    FxRead::bytes(Rewards::materialCount + species, &count, 1);
    if (count == 0) return;
    item::inventoryAdd(*bag, item::ItemKind::Lure, itemId, count);
}

void finishBattle(battle::BattleSession &session, TerminalCallback callback) {
    const battle::Outcome outcome = session.result().outcome;
    if (outcome == battle::Outcome::Gathered) commitGathered(session);
    else if (outcome == battle::Outcome::Win) awardWildKoMaterial(session.state());
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
    // Lose is recoverable. A Draw grants no victory either and uses the same
    // recovery so an exhausted party never returns to the world.
    if (outcome == battle::Outcome::Lose || outcome == battle::Outcome::Draw)
        WorldEngine::queueDefeatRecovery();
    exitBattle();
    gameState.state = GameState_t::WORLD;
}

} // namespace

bool update(uint8_t edgeButtons, TerminalCallback onFinished) {
    battle::BattleMode &mode = modeState.battle;
    battle::BattleSession &session = mode;

    if (!session.isActive()) return false;
#ifdef CGFX_DUEL_BATTLE
    // A finished duel result dwell is terminal (the duel never enters the
    // Presenting phase).
    if (session.exitReady()) {
        finishBattle(session, onFinished);
        return true;
    }
    // Construct any pending duel UI arm (result feedback or a fresh Choice
    // menu) in this same update; consume no edge on a freshly built arm.
    if (duelSyncUi(mode, session)) return false;
#endif
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
#ifdef CGFX_DUEL_BATTLE
            const battle::BattleState &state = session.state();
            mode.ui.menu.beginDuel(state, state.gatherable, !state.trainer);
#else
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
#ifdef CGFX_DUEL_BATTLE
            // An immediate Escape enters the result dwell on this frame, so
            // build its feedback arm now rather than a frame late.
            duelSyncUi(mode, session);
#endif
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
#ifdef CGFX_DUEL_BATTLE
    // advance() may have entered the result dwell this frame; build its arm now
    // (unless the dwell already finalized to terminal, whose feedback arm is
    // still the active one).
    if (!session.exitReady()) duelSyncUi(mode, session);
#endif
    return false;
}

} // namespace BattleFlow
