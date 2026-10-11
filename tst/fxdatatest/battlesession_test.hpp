#pragma once

// Device-suite reconciliation for the compact single-phase BattleFlow.
//
// The staged presenter and voluntary-switch paths are retired. This fixture
// drives the current session controller through BattleFlow::update, its direct
// move/actions/replacement menu pages and its single cached feedback footer.
// The legacy AVR replacement regression lives in test_battle_slim.ino; the
// host suites battle_slim_*_test.hpp cover the same contracts natively.
//
// Retained legacy coverage (now expressed against the compact flow):
//   * party/trainer entry, damage, PP ownership and RNG determinism
//   * forced (not voluntary) replacement, original-slot mapping and ownership
//   * one feedback phase per result; no per-frame metadata reads during render
//   * terminal Win/Lose/Esp/Refused outcomes and exactly-once world restore
// Replaced legacy coverage:
//   * staged PresenterStage pacing -> BattleFeedback single-phase hold
//   * voluntary SelectParty while awaiting a move -> forced-replacement menu
//   * legacy BATTLE_OPTIONS/MOVE_SELECT menu stack assertions -> compact pages
//   * trainer Gather/Escape REFUSED via the old always-open actions menu ->
//     the compact actions page disables unavailable cells; the resolver
//     refusal boundary is asserted directly against the session and is also
//     covered by tst/battle_resolve_test.hpp.

#include "fxtest.hpp"
#include "src/engine/battle/MoveUses.hpp"
#include "src/engine/battle/BattleFlow.hpp"
#include "src/engine/battle/BattlePresets.hpp"
#include "src/engine/battle/BattleScene.hpp"
#include "src/engine/battle/BattleSession.hpp"
#include "src/engine/ModeState.hpp"
#include "src/engine/game/Gamestate.hpp"
#include "src/engine/menu/MenuNav.hpp"
#include "src/engine/menu/MenuV2.hpp"
#include "src/lib/FxReadCounter.hpp"
#include "src/lib/MoveIds.hpp"
#include "src/lib/ReadData.hpp"
#include "src/player/Player.hpp"

namespace battlesession_test_detail {
extern "C" uint8_t __bss_end;

static uint8_t rngDraws;

inline uint8_t scriptedRng(uint8_t bound) {
    ++rngDraws;
    return bound == 0 ? 0 : static_cast<uint8_t>((rngDraws - 1) % bound);
}

inline uint16_t paintStack() {
    const uint16_t base = reinterpret_cast<uint16_t>(&__bss_end);
    const uint16_t top = static_cast<uint16_t>(SP) - 64;
    for (volatile uint8_t *cursor = reinterpret_cast<volatile uint8_t *>(base);
         reinterpret_cast<uint16_t>(cursor) < top; ++cursor) *cursor = 0xC5;
    return top;
}

inline uint16_t headroom(uint16_t top) {
    const uint16_t base = reinterpret_cast<uint16_t>(&__bss_end);
    for (volatile uint8_t *cursor = reinterpret_cast<volatile uint8_t *>(base);
         reinterpret_cast<uint16_t>(cursor) < top; ++cursor) {
        if (*cursor != 0xC5) return reinterpret_cast<uint16_t>(cursor) - base;
    }
    return top - base;
}

// Compact, single-phase presenter: scene plus whichever union arm is live.
// Feedback owns the union while presenting; the menu owns it at a choice or
// replacement boundary. Neither arm performs metadata reads while drawing.
__attribute__((noinline)) inline void renderBattle() {
    battle::BattleScene::draw(battleSession().state());
    if (battleSession().isPresenting()) {
        modeState.battle.ui.feedback.draw();
    } else if (battleSession().awaitingPlayer() &&
               modeState.battle.ui.menu.initialized()) {
        modeState.battle.ui.menu.draw();
    }
}

__attribute__((noinline)) inline void beginPreset(const BattlePresets::Preset &stored,
                       uint8_t damage = 0, bool oneHp = false) {
    const BattlePresets::Preset preset = BattlePresets::copyPreset(stored);
    battle::applyPlayerPreset(player, stored);
    if (damage != 0 && player.creatureHPs[0] > damage) {
        player.creatureHPs[0] -= damage;
    }
    if (oneHp) {
        for (uint8_t slot = 0; slot < 3; ++slot) player.creatureHPs[slot] = 1;
    }
    menu.clear();
    dialogMenu.clear();
    enterBattle();
    gameState.state = GameState_t::BATTLE;
    rngDraws = 0;
    battleSession().setRng({scriptedRng});
    FxReadCounter::resetFrame();
    battleSession().beginTrainer(preset.trainerId);
}

inline void endPreset() {
    exitBattle();
    gameState.state = GameState_t::WORLD;
}

__attribute__((noinline)) inline uint16_t foldResult(uint16_t signature, const battle::ActionResult &result) {
    signature = static_cast<uint16_t>(signature * 33u + static_cast<uint8_t>(result.kind));
    signature = static_cast<uint16_t>(signature * 33u + static_cast<uint8_t>(result.actor));
    signature = static_cast<uint16_t>(signature * 33u + result.index);
    signature = static_cast<uint16_t>(signature * 33u + result.flags);
    signature = static_cast<uint16_t>(signature * 33u + static_cast<uint8_t>(result.outcome));
    signature = static_cast<uint16_t>(signature * 33u + result.hpAfter[0]);
    signature = static_cast<uint16_t>(signature * 33u + result.hpAfter[1]);
    return signature;
}

// Tick the compact controller until it parks at a choice or replacement
// boundary. Returns false on an unexpected terminal exit or read leak.
__attribute__((noinline)) inline bool runToChoice(bool accelerate, uint16_t &signature) {
    for (uint16_t frame = 0; frame < 1200; ++frame) {
        if (!battleSession().isPresenting() && battleSession().awaitingPlayer()) return true;
        const bool presentingBefore = battleSession().isPresenting();
        FxReadCounter::resetFrame();
        if (BattleFlow::update(accelerate ? MENU_EDGE_A : 0)) return false;
        if (!presentingBefore && battleSession().isPresenting()) {
            signature = foldResult(signature, battleSession().result());
        }
        const uint8_t readsBeforeRender = FxReadCounter::count();
        renderBattle();
        if (FxReadCounter::count() != readsBeforeRender) return false;
    }
    return false;
}

enum class Boundary : uint8_t { Choice, Replacement, Terminal, Failed };

struct RunTrace {
    uint16_t signature = 1;
    uint8_t opponentReplacements = 0;
    bool authoredMoves = true;
    bool frameReads = true;
    bool deadAttack = false;
};

// Reconstruct the authored move IDs the trainer seed names, with the same
// absent-move sentinel mapping the runtime loader applies.
__attribute__((noinline)) inline bool opponentAuthoredMoves() {
    const battle::BattleState &state = battleSession().state();
    const OpponentSeed row = readOpponentSeed(state.trainerId);
    const uint8_t slot = state.activeSlot[1];
    const CreatureSeed &seed = slot == 0 ? row.firstCreature :
                               slot == 1 ? row.secondCreature : row.thirdCreature;
    if (state.active[1].id != seed.id || state.active[1].level != seed.lvl) return false;
    for (uint8_t move = 0; move < 4; ++move) {
        const uint8_t authored = battleMoveId(parseOpponentCreatureSeedMove(seed.moves, move));
        if (state.active[1].moveIds[move] != authored) return false;
    }
    return true;
}

// Resolve once and advance the compact presentation to the next boundary.
// Folds every newly begun result, tracks forced opponent replacements and
// verifies render never performs metadata reads.
__attribute__((noinline)) inline Boundary runToBoundary(RunTrace &trace) {
    for (uint16_t frame = 0; frame < 1600; ++frame) {
        if (!battleSession().isPresenting() && battleSession().awaitingPlayer()) {
            return battleSession().awaitingReplacement()
                ? Boundary::Replacement : Boundary::Choice;
        }
        const bool presentingBefore = battleSession().isPresenting();
        FxReadCounter::resetFrame();
        if (BattleFlow::update(MENU_EDGE_A)) return Boundary::Terminal;
        const bool begun = !presentingBefore && battleSession().isPresenting();
        if (begun) {
            const battle::ActionResult &result = battleSession().result();
            trace.signature = foldResult(trace.signature, result);
            if (result.kind == battle::ResultKind::Attack &&
                result.hpBefore[static_cast<uint8_t>(result.actor)] == 0) {
                trace.deadAttack = true;
            }
            if (result.kind == battle::ResultKind::Switch &&
                result.actor == battle::Side::Opponent) {
                ++trace.opponentReplacements;
                trace.authoredMoves = trace.authoredMoves && opponentAuthoredMoves();
            }
            // Draw once per begun result: enough to prove the shared draw path
            // is metadata-read-free without pacing the whole fight through a
            // full raster redraw every controller frame.
            const uint8_t readsBeforeRender = FxReadCounter::count();
            renderBattle();
            trace.frameReads = trace.frameReads &&
                FxReadCounter::count() == readsBeforeRender;
            if (!trace.frameReads) return Boundary::Failed;
            if (result.outcome != battle::Outcome::None) return Boundary::Terminal;
        }
    }
    return Boundary::Failed;
}

// Font size probes: keep the compact menu's direct pages reachable at the
// choice boundary and submit the lowest usable move.
__attribute__((noinline)) inline void chooseMove() {
    BattleFlow::update(0);
    BattleFlow::update(MENU_NAV_RIGHT);
    BattleFlow::update(MENU_EDGE_A);
}

// Responsibility split: open the filtered replacement page and submit the
// first living row. The caller resolves through runToBoundary.
__attribute__((noinline)) inline void chooseForcedReplacement(FxTest &test) {
    BattleFlow::update(0);
    test.expectEq(modeState.battle.ui.menu.initialized(), true,
                  F("forced replacement menu"));
    BattleFlow::update(MENU_EDGE_B);
    test.expectEq(battleSession().awaitingReplacement(), true,
                  F("Back cannot cancel forced replacement"));
    BattleFlow::update(MENU_EDGE_A);
}

// Drive terminal feedback to completion and verify the exactly-once world
// restore, party HP synchronization and menu/dialog cleanup.
__attribute__((noinline)) inline bool finishTerminal(FxTest &test) {
    test.expectEq(gameState.state, GameState_t::BATTLE,
                  F("terminal feedback keeps battle active"));
    const uint16_t location = gameState.playerLocation;
    const battle::BattleView before = battleSession().view();
    bool returned = false;
    for (uint16_t frame = 0; frame < 500; ++frame) {
        FxReadCounter::resetFrame();
        returned = BattleFlow::update((frame % 11u) == 0 ? MENU_EDGE_A : 0);
        if (returned) break;
        const uint8_t readsBeforeRender = FxReadCounter::count();
        renderBattle();
        if (FxReadCounter::count() != readsBeforeRender) return false;
    }
    test.expectEq(returned, true, F("terminal feedback eventually returns to world"));
    if (!returned) return false;
    test.expectEq(gameState.state, GameState_t::WORLD, F("world state restored"));
    test.expectEq(gameState.playerLocation, location, F("world tile preserved"));
    test.expectEq(menu.menuPointer, static_cast<uint32_t>(-1),
                  F("battle menu cleared on return"));
    test.expectEq(dialogMenu.peek(), false, F("dialog cleared on return"));
    for (uint8_t slot = 0; slot < before.partyCount[0]; ++slot) {
        test.expectEq(player.creatureHPs[slot], before.party[0][slot].hp,
                      F("terminal sync preserves party HP"));
    }
    return true;
}

__attribute__((noinline)) inline void trainerSpike(FxTest &test) {
    beginPreset(BattlePresets::opening);
    test.expectEq(battleSession().state().partyCount[0], 3, F("player has three members"));
    test.expectEq(battleSession().state().partyCount[1], 3, F("trainer has three members"));
    test.expectEq(battleSession().state().trainer, true, F("trainer rules active"));
    test.expectEq(FxReadCounter::framePassed(), true, F("trainer setup read budget"));

    FxReadCounter::resetFrame();
    test.expectEq(BattleFlow::update(0), false, F("choice opens without exit"));
    test.expectEq(modeState.battle.ui.menu.initialized(), true,
                  F("compact move page open"));
    test.expectEq(FxReadCounter::count() > 0, true,
                  F("menu transition caches move metadata"));

    FxReadCounter::resetFrame();
    test.expectEq(BattleFlow::update(MENU_EDGE_A), false,
                  F("selection submits without resolving"));
    test.expectEq(battleSession().isPresenting(), false,
                  F("selection edge does not resolve in the same frame"));
    test.expectEq(FxReadCounter::count(), 0, F("selection is metadata-free"));

    FxReadCounter::resetFrame();
    test.expectEq(BattleFlow::update(0), false,
                  F("next frame resolves once and begins feedback"));
    test.expectEq(battleSession().isPresenting(), true,
                  F("result presentation begins"));
    test.expectEq(static_cast<uint8_t>(battleSession().result().kind),
                  static_cast<uint8_t>(battle::ResultKind::Attack),
                  F("first result is an attack"));
    test.expectEq(FxReadCounter::count() > 0, true,
                  F("feedback transition caches caption metadata"));
    const uint8_t drawsAfterResolve = rngDraws;

    const uint8_t readsBeforeRender = FxReadCounter::count();
    renderBattle();
    renderBattle();
    test.expectEq(rngDraws, drawsAfterResolve, F("render does not reroll"));
    test.expectEq(FxReadCounter::count(), readsBeforeRender,
                  F("render adds no metadata reads"));

    FxReadCounter::resetFrame();
    BattleFlow::update(0);
    test.expectEq(FxReadCounter::count(), 0, F("feedback timer ticks are metadata-free"));
    test.expectEq(FxReadCounter::markUpdate(), true, F("steady feedback frame stays in budget"));
    test.expectEq(rngDraws, drawsAfterResolve, F("feedback ticks do not rerun the action"));

    bool completed = false;
    for (uint16_t frame = 0; frame < 64; ++frame) {
        FxReadCounter::resetFrame();
        if (BattleFlow::update(0)) break;
        if (!battleSession().isPresenting()) { completed = true; break; }
        const uint8_t before = FxReadCounter::count();
        renderBattle();
        if (FxReadCounter::count() != before) break;
    }
    test.expectEq(completed, true, F("first result completes in bounded frames"));
    test.expectEq(rngDraws, drawsAfterResolve,
                  F("feedback completion does not reroll the action"));

    uint16_t signature = 1;
    test.expectEq(runToChoice(true, signature), true,
                  F("turn returns to the next choice"));
    test.expectEq(gameState.state, GameState_t::BATTLE,
                  F("ordinary result stays in battle"));
    endPreset();
}

__attribute__((noinline)) inline void typeStatusEffectiveness(FxTest &test) {
    beginPreset(BattlePresets::opening);
    battle::BattleState &state =
        const_cast<battle::BattleState &>(battleSession().state());
    state.active[0].types = DualType(Type::WATER, Type::WIND);
    state.active[0].moves[0] = Move(MoveBitSet{
        static_cast<uint8_t>(Type::WATER), 10, 1, 0, 0
    });
    state.active[0].stats.attack = 40;
    state.active[0].stats.speed = 255;
    state.active[0].status.effects[0] = Effect::DRNCHD;
    state.active[0].status.effects[1] = Effect::AIRSWPT;
    state.active[0].statMods.clearModifiers();
    state.active[1].types = DualType(Type::EARTH, Type::PLANT);
    state.active[1].hp = state.active[1].maxHp = 100;
    state.active[1].stats.hp = 100;
    state.active[1].stats.defense = 20;
    state.active[1].stats.speed = 0;
    state.active[1].status.effects[0] = Effect::SOILED;
    state.active[1].status.effects[1] = Effect::TANGLD;
    state.active[1].statMods.clearModifiers();

    test.expectEq(battleSession().submitIntent({MenuIntentKind::SelectMove, 0}),
                  true, F("stacked-status submits"));
    test.expectEq(battleSession().advance(), true,
                  F("stacked-status player attack resolves"));
    test.expectEq(static_cast<uint8_t>(battleSession().result().effectiveness),
                  static_cast<uint8_t>(Modifier::Double),
                  F("damage effectiveness matches"));
    test.expectEq(battleSession().result().hpBefore[1], 100,
                  F("stacked-status initial HP"));
    test.expectEq(battleSession().result().hpAfter[1], 60,
                  F("stacked-status loses40HP"));
    test.expectEq(battleSession().state().active[1].hp, 60,
                  F("stacked-status damage40"));
    endPreset();
}

// Trainer restrictions: the compact actions page disables Gather and Escape
// (a trainer is neither gatherable nor escapable), and the resolver still
// refuses those actions without RNG or acquisition if they reach it.
__attribute__((noinline)) inline void trainerActionRestrictions(FxTest &test) {
    beginPreset(BattlePresets::opening);
    const uint8_t hpBefore = battleSession().state().active[0].hp;
    BattleFlow::update(0);
    BattleFlow::update(MENU_EDGE_B);
    BattleFlow::update(MENU_EDGE_A);
    test.expectEq(battleSession().isPresenting(), false,
                  F("disabled escape cell does not resolve"));
    BattleFlow::update(MENU_NAV_LEFT);
    BattleFlow::update(MENU_EDGE_A);
    test.expectEq(battleSession().isPresenting(), false,
                  F("disabled gather cell does not resolve"));
    test.expectEq(static_cast<uint8_t>(battleSession().result().kind),
                  static_cast<uint8_t>(battle::ResultKind::None),
                  F("no action submitted from a disabled cell"));
    test.expectEq(battleSession().state().active[0].hp, hpBefore,
                  F("disabled cells do not damage the player"));
    endPreset();

    beginPreset(BattlePresets::opening);
    test.expectEq(battleSession().submitIntent({MenuIntentKind::Gather, 0}), true,
                  F("trainer gather submits"));
    test.expectEq(battleSession().advance(), true, F("trainer gather resolves"));
    test.expectEq(static_cast<uint8_t>(battleSession().result().kind),
                  static_cast<uint8_t>(battle::ResultKind::Gather),
                  F("trainer gather result"));
    test.expectEq((battleSession().result().flags & battle::REFUSED) != 0, true,
                  F("trainer gather refused"));
    test.expectEq(battleSession().state().gather.progress, 0,
                  F("refused gather has no acquisition"));
    test.expectEq(rngDraws, 0, F("refused gather consumes no RNG"));
    endPreset();

    beginPreset(BattlePresets::opening);
    test.expectEq(battleSession().submitIntent({MenuIntentKind::Escape, 0}), true,
                  F("trainer escape submits"));
    test.expectEq(battleSession().advance(), true, F("trainer escape resolves"));
    test.expectEq(static_cast<uint8_t>(battleSession().result().kind),
                  static_cast<uint8_t>(battle::ResultKind::Escape),
                  F("trainer escape result"));
    test.expectEq((battleSession().result().flags & battle::REFUSED) != 0, true,
                  F("trainer escape refused"));
    test.expectEq(battleSession().state().over, false,
                  F("escape refusal leaves trainer battle live"));
    endPreset();
}

// Forced replacement through the compact flow: a faster opponent knocks out
// the frozen player actor, the direct replacement page offers only the living
// original slots and the accepted row restores ownership by original slot.
__attribute__((noinline)) inline void trainerForcedReplacement(FxTest &test) {
    beginPreset(BattlePresets::switch_drill, 5);
    test.expectEq(battleSession().state().active[0].hp, player.creatureHPs[0],
                  F("entry imports damaged persistent HP"));
    test.expectEq(battleSession().state().active[0].hp + 5,
                  battleSession().state().active[0].maxHp,
                  F("entry does not refill damaged player"));

    battle::BattleState &state =
        const_cast<battle::BattleState &>(battleSession().state());
    state.active[0].hp = 1;
    state.active[0].stats.defense = 1;
    state.active[0].stats.spcDef = 1;
    state.active[0].stats.speed = 1;
    state.active[1].stats.attack = 255;
    state.active[1].stats.spcAtk = 255;
    state.active[1].stats.speed = 255;
    state.active[1].moves[0] = Move(MoveBitSet{
        static_cast<uint8_t>(Type::SPIRIT), 255, 1, 0, 0
    });
    state.active[1].moveIds[0] = 0;
    for (uint8_t slot = 1; slot < 4; ++slot) state.active[1].moveIds[slot] = 255;

    BattleFlow::update(0);
    test.expectEq(modeState.battle.ui.menu.initialized(), true,
                  F("compact move page ready for the frozen action"));
    const uint8_t playerUsesBefore = battle::remainingMoveUses(
        battleSession().state(), battle::Side::Player, 0);
    BattleFlow::update(MENU_EDGE_A);
    test.expectEq(battleSession().isPresenting(), false,
                  F("selection does not resolve in the same frame"));
    BattleFlow::update(0);
    test.expectEq(battleSession().isPresenting(), true,
                  F("knockout result begins presentation"));
    test.expectEq(static_cast<uint8_t>(battleSession().result().kind),
                  static_cast<uint8_t>(battle::ResultKind::Attack),
                  F("opponent knockout attack"));
    test.expectEq(static_cast<uint8_t>(battleSession().result().actor),
                  static_cast<uint8_t>(battle::Side::Opponent),
                  F("opponent owns the knockout action"));
    test.expectEq(state.active[0].hp, 0,
                  F("opponent knocks out the frozen player actor"));
    test.expectEq((battleSession().result().flags & battle::PLAYER_FAINTED) != 0,
                  true, F("faint flagged on the result"));
    test.expectEq(battle::remainingMoveUses(battleSession().state(),
                  battle::Side::Player, 0), playerUsesBefore,
                  F("invalidated frozen actor spends no move use"));

    for (uint16_t frame = 0; frame < 64 &&
         !battleSession().awaitingReplacement(); ++frame) {
        BattleFlow::update(MENU_EDGE_A);
    }
    test.expectEq(battleSession().awaitingReplacement(), true,
                  F("faint waits for forced replacement"));
    test.expectEq(BattleFlow::update(0), false,
                  F("replacement boundary opens the direct menu"));
    test.expectEq(modeState.battle.ui.menu.initialized(), true,
                  F("forced replacement menu"));
    test.expectEq(BattleFlow::update(MENU_EDGE_B), false,
                  F("Back cannot cancel forced replacement"));
    test.expectEq(battleSession().awaitingReplacement(), true,
                  F("Back leaves replacement pending"));

    const battle::ReplacementChoices choices = battleSession().replacementChoices();
    test.expectEq(choices.count, static_cast<uint8_t>(2),
                  F("both living bench rows offered"));
    test.expectEq(BattleFlow::update(MENU_NAV_DOWN), false,
                  F("down toggles to the second living row"));
    BattleFlow::update(MENU_EDGE_A);
    test.expectEq(battleSession().isPresenting(), false,
                  F("replacement selection does not apply in the same frame"));
    BattleFlow::update(0);
    test.expectEq(static_cast<uint8_t>(battleSession().result().kind),
                  static_cast<uint8_t>(battle::ResultKind::Switch),
                  F("valid forced replacement starts"));
    test.expectEq(static_cast<uint8_t>(battleSession().result().actor),
                  static_cast<uint8_t>(battle::Side::Player),
                  F("player owns the forced replacement"));
    test.expectEq((battleSession().result().flags & battle::FORCED_SWITCH) != 0,
                  true, F("replacement marked forced"));
    test.expectEq(battleSession().state().activeSlot[0], choices.slots[1],
                  F("second visible row maps to its original party slot"));
    test.expectEq(battleSession().state().active[0].id,
                  player.party[choices.slots[1]].id,
                  F("replacement loads the selected original creature"));

    uint16_t signature = foldResult(1, battleSession().result());
    test.expectEq(runToChoice(true, signature), true,
                  F("replacement turn returns to choice"));
    test.expectEq(battleSession().state().bench[0][0].hp, 0,
                  F("knocked-out original slot keeps zero HP on the bench"));
    endPreset();
}

// Wild encounter: the compact actions page disables Gather for a
// non-gatherable creature, while Escape still drives the terminal Escaped
// outcome and the world restore.
__attribute__((noinline)) inline void wildControllerReturn(FxTest &test) {
    battle::applyPlayerPreset(player, BattlePresets::opening);
    gameState.playerLocation = static_cast<uint16_t>(3) |
                               (static_cast<uint16_t>(2) << 8);
    menu.clear();
    dialogMenu.clear();
    enterBattle();
    gameState.state = GameState_t::BATTLE;
    rngDraws = 0;
    battleSession().setRng({scriptedRng});
    FxReadCounter::resetFrame();
    battleSession().beginWild(4, 1, false, 0);
    test.expectEq(battleSession().state().trainer, false,
                  F("wild encounter uses wild rules"));
    test.expectEq(battleSession().state().partyCount[1], 1,
                  F("wild encounter has one opponent"));

    BattleFlow::update(0);
    test.expectEq(modeState.battle.ui.menu.initialized(), true,
                  F("wild choice opens the compact page"));
    BattleFlow::update(MENU_EDGE_B);
    BattleFlow::update(MENU_NAV_LEFT);
    BattleFlow::update(MENU_EDGE_A);
    test.expectEq(battleSession().isPresenting(), false,
                  F("non-gatherable Gather cell is inert"));
    test.expectEq(static_cast<uint8_t>(battleSession().result().kind),
                  static_cast<uint8_t>(battle::ResultKind::None),
                  F("disabled wild Gather does not resolve"));

    BattleFlow::update(MENU_NAV_RIGHT);
    BattleFlow::update(MENU_EDGE_A);
    BattleFlow::update(0);
    test.expectEq(static_cast<uint8_t>(battleSession().result().kind),
                  static_cast<uint8_t>(battle::ResultKind::Escape),
                  F("wild Escape controller"));
    test.expectEq(static_cast<uint8_t>(battleSession().result().outcome),
                  static_cast<uint8_t>(battle::Outcome::Escaped),
                  F("wild Escape becomes terminal"));
    finishTerminal(test);
}

__attribute__((noinline)) inline void trainerVictory(FxTest &test) {
    beginPreset(BattlePresets::switch_drill);
    RunTrace trace;
    Boundary boundary = Boundary::Choice;
    uint8_t turns = 0;
    for (; turns < 96; ++turns) {
        if (boundary == Boundary::Choice) {
            chooseMove();
        } else if (boundary == Boundary::Replacement) {
            chooseForcedReplacement(test);
        } else {
            break;
        }
        boundary = runToBoundary(trace);
        if (boundary == Boundary::Terminal || boundary == Boundary::Failed) break;
    }
    Serial.print(F("trainer victory turns=")); Serial.println(turns);
    Serial.print(F("trainer victory boundary="));
    Serial.println(static_cast<uint8_t>(boundary));
    if (boundary != Boundary::Terminal) {
        const battle::BattleState &state = battleSession().state();
        Serial.print(F("trainer victory switches="));
        Serial.println(trace.opponentReplacements);
        Serial.print(F("trainer victory slots/hp="));
        Serial.print(state.activeSlot[1]);
        Serial.print(',');
        Serial.print(state.active[1].hp);
        Serial.print(',');
        Serial.print(state.bench[1][0].hp);
        Serial.print(',');
        Serial.println(state.bench[1][1].hp);
        Serial.print(F("trainer victory player hp="));
        Serial.println(state.active[0].hp);
    }
    test.expectEq(static_cast<uint8_t>(boundary),
                  static_cast<uint8_t>(Boundary::Terminal),
                  F("three-opponent trainer fight terminates"));
    if (boundary == Boundary::Terminal) {
        test.expectEq(static_cast<uint8_t>(battleSession().result().outcome),
                      static_cast<uint8_t>(battle::Outcome::Win),
                      F("third opponent defeat awards Win"));
        test.expectEq(battleSession().state().activeSlot[1], 2,
                      F("third original trainer slot was active"));
        test.expectEq(trace.opponentReplacements, 2,
                      F("both opponent bench members replaced"));
        test.expectEq(trace.authoredMoves, true,
                      F("replacement retains authored moves"));
        test.expectEq(trace.deadAttack, false,
                      F("no fainted actor attacks"));
        test.expectEq(trace.frameReads, true,
                      F("victory transition reads"));
        finishTerminal(test);
    } else {
        endPreset();
    }
}

__attribute__((noinline)) inline void trainerDefeat(FxTest &test) {
    beginPreset(BattlePresets::opening, 0, true);
    RunTrace trace;
    Boundary boundary = Boundary::Choice;
    uint8_t replacements = 0;
    for (uint8_t turn = 0; turn < 32; ++turn) {
        if (boundary == Boundary::Choice) {
            chooseMove();
        } else if (boundary == Boundary::Replacement) {
            chooseForcedReplacement(test);
            ++replacements;
        } else {
            break;
        }
        boundary = runToBoundary(trace);
        if (boundary == Boundary::Terminal || boundary == Boundary::Failed) break;
    }
    Serial.print(F("trainer defeat boundary="));
    Serial.println(static_cast<uint8_t>(boundary));
    test.expectEq(static_cast<uint8_t>(boundary),
                  static_cast<uint8_t>(Boundary::Terminal),
                  F("all-player defeat terminates"));
    if (boundary == Boundary::Terminal) {
        test.expectEq(static_cast<uint8_t>(battleSession().result().outcome),
                      static_cast<uint8_t>(battle::Outcome::Lose),
                      F("third player faint awards Lose"));
        test.expectEq(replacements, 2,
                      F("both live player replacements selected"));
        test.expectEq(trace.deadAttack, false,
                      F("fainted player never attacks"));
        test.expectEq(trace.frameReads, true,
                      F("defeat transition reads"));
        finishTerminal(test);
    } else {
        endPreset();
    }
}
} // namespace battlesession_test_detail

inline void test_battlesession(FxTest &test) {
    using namespace battlesession_test_detail;
    const uint16_t top = paintStack();
    trainerSpike(test);
    typeStatusEffectiveness(test);
    trainerActionRestrictions(test);
    trainerForcedReplacement(test);
    wildControllerReturn(test);
    const uint16_t measured = headroom(top);
    Serial.print(F("battle controller painted headroom=")); Serial.println(measured);
    Serial.print(F("battle controller effective headroom="));
    Serial.println(measured >= 69 ? measured - 69 : 0);
    test.expectEq(measured >= 219, true,
                  F("controller stack reserve150B"));
}

inline void test_battletrainer(FxTest &test) {
    battlesession_test_detail::trainerVictory(test);
    battlesession_test_detail::trainerDefeat(test);
}
