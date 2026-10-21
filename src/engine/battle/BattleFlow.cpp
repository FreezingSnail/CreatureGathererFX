#include "BattleFlow.hpp"

#include "BattleFeedback.hpp"
#include "BattleMenu.hpp"
#include "BattleScene.hpp"
#include "BattleSession.hpp"
#include "../ModeState.hpp"
#include "../../GameState.hpp"
#include "../../audio/Audio.hpp"
#include "../../globals.hpp"
#include "../../creature/LevelCurve.hpp"
#include "../../creature/Evolution.hpp"
#include "../../save/Compaction.hpp"
#include "../../item/Inventory.hpp"
#include "../../lib/FxRead.hpp"
#include "../../lib/ReadData.hpp"
#include "../../fxdata.h"
#include "../world/World.hpp"

namespace BattleFlow {
namespace {

void beginMenu(battle::BattleMode &mode) {
    ::new (static_cast<void *>(&mode.ui.menu)) battle::BattleMenu();
}

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

// jp8.11.10: an ordinary (non-trainer, non-gatherable) wild Win grants exactly
// 8 * opponentLevel to every living occupied party instance, once, at the single
// terminal edge. Gathered/Escape/Fled/Lose/Draw, trainer/boss and gatherable
// victories grant nothing and a KO'd member never receives the reward. Each
// member's stable store slot is read once: the stored cumulative exp is the
// growth authority, saturates at the level-31 threshold and derives the capped
// level. A level increase preserves the member's missing HP (the live HP grows
// by the maximum-HP delta, capped at the new maximum). Every mutation is
// journaled before the live creature is installed, so a refused write leaves the
// accepted per-instance state exactly as it was and never invents awarded exp.
// jp8.11.12: on the same terminal edge, a member whose derived level has reached
// its authored evolution threshold replaces its species with the explicit target
// in the same stable record, keeping exp, stable slot and attack mask (moves);
// the target's stats/types are recomputed at the unchanged derived level and a
// living member keeps its missing HP (a KO member never reaches this path). A
// refused write leaves the old species in place, so the pending evolution stays
// derivable from exp and retryable on the next eligible transition.
void awardOrdinaryWildExp(const battle::BattleState &state) {
    const uint16_t gain = static_cast<uint16_t>(
        state.active[static_cast<uint8_t>(battle::Side::Opponent)].level) * 8u;
    const uint16_t cap = levelExpThreshold(LEVEL_COUNT - 1);
    for (uint8_t i = 0; i < PARTY_SIZE; ++i) {
        if (player.creatureHPs[i] == 0) continue; // empty slot or KO: no reward
        const uint8_t slot = storePartySlot(i);
        StoreRecord record;
        // A 255/absent stable slot refuses here: storeRead's bound check.
        if (!storeRead(slot, record)) continue;
        const uint8_t oldMax = player.party[i].hp;
        uint8_t newMax = oldMax;
        uint8_t newHP = player.creatureHPs[i];
        const uint16_t room = static_cast<uint16_t>(cap - record.exp);
        record.exp = room <= gain ? cap : static_cast<uint16_t>(record.exp + gain);
        Creature grown;
        grown.load(record); // derived level and stats (one species read)
        if (grown.level > player.party[i].level) {
            newMax = grown.hp;
            const uint16_t grownHp = static_cast<uint16_t>(newHP) +
                                     static_cast<uint16_t>(newMax - oldMax);
            newHP = grownHp > newMax ? newMax : static_cast<uint8_t>(grownHp);
        }
        const uint8_t target = CreatureEvolution::targetFor(record.id, grown.level);
        const bool evolved = target != CreatureEvolution::NO_TARGET &&
                             target != record.id;
        if (evolved) {
            const uint8_t missing = static_cast<uint8_t>(newMax - newHP);
            record.id = target;
            // Adopt the target species authored kit: its mask replaces the source
            // instance mask, so the record and the live member agree again.
            record.attackMask = readCreatureMask(target);
            grown.load(record); // target species at the same derived level
            newMax = grown.hp;
            newHP = missing >= newMax ? 0 : static_cast<uint8_t>(newMax - missing);
        }
        record.hpEncoded = newHP >= newMax ? 0 : static_cast<uint8_t>(newHP + 1);
        if (!storeWrite(slot, record)) continue; // refused: keep accepted state
        if (evolved || grown.level > player.party[i].level) {
            player.party[i] = grown;
            player.creatureHPs[i] = newHP;
        }
    }
}

void finishBattle(battle::BattleSession &session, TerminalCallback callback) {
    const battle::Outcome outcome = session.result().outcome;
    if (outcome == battle::Outcome::Gathered) commitGathered(session);
    else if (outcome == battle::Outcome::Win) {
        const battle::BattleState &state = session.state();
        if (!state.trainer && !state.gatherable) {
            awardWildKoMaterial(state);
            awardOrdinaryWildExp(state);
        }
    }
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
    // Bounded result cue after the world theme is restored: it replaces the
    // overworld track and service() returns to it after kResultCueFrames. Other
    // outcomes (Fled/Escaped) keep the plain world theme.
    if (outcome == battle::Outcome::Win || outcome == battle::Outcome::Gathered)
        audio::playResult(MusicTrack_Victory, kResultCueFrames);
    else if (outcome == battle::Outcome::Lose || outcome == battle::Outcome::Draw)
        audio::playResult(MusicTrack_Defeat, kResultCueFrames);
}

} // namespace

bool update(uint8_t edgeButtons, TerminalCallback onFinished) {
    battle::BattleMode &mode = modeState.battle;
    battle::BattleSession &session = mode;

    if (!session.isActive()) return false;
    // A finished duel result dwell is terminal (the duel never enters the
    // Presenting phase).
    if (session.exitReady()) {
        finishBattle(session, onFinished);
        return true;
    }
    // Construct any pending duel UI arm (result feedback or a fresh Choice
    // menu) in this same update; consume no edge on a freshly built arm.
    if (duelSyncUi(mode, session)) return false;
#if CGFX_DIALOG_QUEUE_ENABLED
    if (dialogMenu.peek()) dialogMenu.clear();
#endif

    if (session.awaitingPlayer()) {
        if (!mode.ui.menu.initialized()) {
            const battle::BattleState &state = session.state();
            mode.ui.menu.beginDuel(state, state.gatherable, !state.trainer);
            return false;
        }

        const MenuIntent intent = mode.ui.menu.update(edgeButtons);
        // The party page caches its living rows on the CHANGE transition.
        if (mode.ui.menu.partyPending())
            mode.ui.menu.beginParty(session.view());
        if (intent.kind != MenuIntentKind::None && session.submitIntent(intent)) {
            // Resolving starts on the next frame; this edge belongs only to the
            // accepted selection and is never replayed into another phase.
            // An immediate Escape enters the result dwell on this frame, so
            // build its feedback arm now rather than a frame late.
            duelSyncUi(mode, session);
        }
        return false;
    }

    // Drive the duel timing phases (concealed interval / result dwell). In the
    // duel this never returns true: the dwell's terminal exit is detected by
    // exitReady() above on the following frame.
    session.advance();
    // advance() may have entered the result dwell this frame; build its arm now
    // (unless the dwell already finalized to terminal, whose feedback arm is
    // still the active one).
    if (!session.exitReady()) duelSyncUi(mode, session);
    return false;
}

} // namespace BattleFlow
