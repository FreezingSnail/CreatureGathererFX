#pragma once

#include "fxtest.hpp"
#include "src/engine/battle/MoveUses.hpp"
#include "src/engine/battle/BattleFlow.hpp"
#include "src/engine/battle/BattlePresets.hpp"
#include "src/engine/battle/BattleSession.hpp"
#include "src/engine/draw.h"
#include "src/engine/game/Gamestate.hpp"
#include "src/engine/menu/MenuNav.hpp"
#include "src/engine/menu/MenuV2.hpp"
#include "src/lib/FxReadCounter.hpp"
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

inline void renderBattle() {
    battle::BattleView view = battleSession().view();
    battlePresenter().overlay(view);
    arduboy.clear();
    drawScene(view);
    battlePresenter().draw();
}

inline void beginPreset(const BattlePresets::Preset &stored, uint8_t damage = 0,
                       bool oneHp = false) {
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

inline uint16_t foldResult(uint16_t signature, const battle::ActionResult &result) {
    signature = static_cast<uint16_t>(signature * 33u + static_cast<uint8_t>(result.kind));
    signature = static_cast<uint16_t>(signature * 33u + static_cast<uint8_t>(result.actor));
    signature = static_cast<uint16_t>(signature * 33u + result.index);
    signature = static_cast<uint16_t>(signature * 33u + result.flags);
    signature = static_cast<uint16_t>(signature * 33u + static_cast<uint8_t>(result.outcome));
    signature = static_cast<uint16_t>(signature * 33u + result.hpAfter[0]);
    signature = static_cast<uint16_t>(signature * 33u + result.hpAfter[1]);
    return signature;
}

inline bool runToChoice(bool accelerate, uint16_t &signature) {
    for (uint16_t frame = 0; frame < 1200; ++frame) {
        if (battlePresenter().stage() == battle::PresenterStage::Idle &&
            battleSession().awaitingPlayer()) return true;
        const bool idleBefore = battlePresenter().stage() == battle::PresenterStage::Idle;
        FxReadCounter::resetFrame();
        if (BattleFlow::update(accelerate ? MENU_EDGE_A : 0)) return false;
        if (idleBefore && battlePresenter().stage() != battle::PresenterStage::Idle) {
            signature = foldResult(signature, battleSession().result());
        }
        const bool updateReads = FxReadCounter::markUpdate();
        renderBattle();
        const bool renderReads = FxReadCounter::renderExact(0);
        if (!updateReads || !renderReads) return false;
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

inline bool opponentAuthoredMoves() {
    const battle::BattleState &state = battleSession().state();
    const OpponentSeed row = readOpponentSeed(state.trainerId);
    const uint8_t slot = state.activeSlot[1];
    const CreatureSeed &seed = slot == 0 ? row.firstCreature :
                               slot == 1 ? row.secondCreature : row.thirdCreature;
    if (state.active[1].id != seed.id || state.active[1].level != seed.lvl) return false;
    for (uint8_t move = 0; move < 4; ++move) {
        if (state.active[1].moveIds[move] !=
            parseOpponentCreatureSeedMove(seed.moves, move)) return false;
    }
    return true;
}

inline Boundary runToBoundary(RunTrace &trace) {
    battle::PresenterStage lastDrawnStage = battle::PresenterStage::Idle;
    for (uint16_t frame = 0; frame < 1600; ++frame) {
        if (battleSession().result().outcome != battle::Outcome::None &&
            battlePresenter().stage() != battle::PresenterStage::Idle) {
            return Boundary::Terminal;
        }
        if (battlePresenter().stage() == battle::PresenterStage::Idle) {
            if (battleSession().awaitingReplacement()) return Boundary::Replacement;
            if (battleSession().awaitingPlayer()) return Boundary::Choice;
        }
        const bool idleBefore = battlePresenter().stage() == battle::PresenterStage::Idle;
        FxReadCounter::resetFrame();
        if (BattleFlow::update(MENU_EDGE_A)) return Boundary::Failed;
        const bool begun = idleBefore &&
                           battlePresenter().stage() != battle::PresenterStage::Idle;
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
            }
        }
        const bool updateReads = FxReadCounter::markUpdate();
        // The first spike exercises every raster frame. Long 3v3 runs draw
        // each distinct production stage while keeping the serial suite
        // comfortably inside the default capture window.
        const battle::PresenterStage stage = battlePresenter().stage();
        if (begun || stage != lastDrawnStage) renderBattle();
        lastDrawnStage = stage;
        const bool renderReads = FxReadCounter::renderExact(0);
        trace.frameReads = trace.frameReads && updateReads && renderReads;
        if (!trace.frameReads) return Boundary::Failed;
        if (begun && battleSession().result().kind == battle::ResultKind::Switch &&
            battleSession().result().actor == battle::Side::Opponent) {
            trace.authoredMoves = trace.authoredMoves && opponentAuthoredMoves();
        }
    }
    return Boundary::Failed;
}

inline void selectFirstLiveReplacement(FxTest &test) {
    BattleFlow::update(0);
    test.expectEq(battleSession().awaitingReplacement(), true,
                  F("faint waits for player replacement"));
    BattleFlow::update(MENU_EDGE_B);
    test.expectEq(battleSession().awaitingReplacement(), true,
                  F("Back cannot cancel forced replacement"));
    const uint8_t dead = battleSession().state().activeSlot[0];
    test.expectEq(battleSession().submitIntent({MenuIntentKind::SelectParty, dead}),
                  false, F("fainted slot cannot replace itself"));
    test.expectEq(battleSession().submitIntent({MenuIntentKind::SelectParty, 255}),
                  false, F("out-of-range slot refused"));
    const battle::PartySnapshot &choices = menu.partySnapshot();
    if (choices.count > 1 && choices.choices[0].hp == 0) {
        BattleFlow::update(MENU_NAV_DOWN);
    }
    BattleFlow::update(MENU_EDGE_A);
    test.expectEq(static_cast<uint8_t>(battleSession().result().kind),
                  static_cast<uint8_t>(battle::ResultKind::Switch),
                  F("valid forced replacement starts"));
    test.expectEq((battleSession().result().flags & battle::FORCED_SWITCH) != 0,
                  true, F("replacement marked forced"));
}

inline bool finishTerminal(FxTest &test) {
    test.expectEq(gameState.state, GameState_t::BATTLE,
                  F("terminal feedback keeps battle active"));
    const uint16_t location = gameState.playerLocation;
    const battle::BattleView before = battleSession().view();
    bool returned = false;
    battle::PresenterStage lastDrawnStage = battle::PresenterStage::Idle;
    for (uint16_t frame = 0; frame < 500; ++frame) {
        FxReadCounter::resetFrame();
        returned = BattleFlow::update((frame % 11u) == 0 ? MENU_EDGE_A : 0);
        if (returned) break;
        FxReadCounter::markUpdate();
        const battle::PresenterStage stage = battlePresenter().stage();
        if (stage != lastDrawnStage) renderBattle();
        lastDrawnStage = stage;
        FxReadCounter::renderExact(0);
        if (!FxReadCounter::framePassed()) return false;
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

// BATTLE_OPTIONS uses two columns: attack=0, gather=1, switch=2, escape=3.
inline void selectOption(uint8_t navigation) {
    BattleFlow::update(0);
    if (navigation != 0) BattleFlow::update(navigation);
    BattleFlow::update(MENU_EDGE_A);
}

__attribute__((noinline)) inline void trainerSpike(FxTest &test) {
    beginPreset(BattlePresets::opening);
    test.expectEq(battleSession().state().partyCount[0], 3, F("player has three members"));
    test.expectEq(battleSession().state().partyCount[1], 3, F("trainer has three members"));
    test.expectEq(battleSession().state().trainer, true, F("trainer rules active"));
    test.expectEq(FxReadCounter::framePassed(), true, F("trainer setup read budget"));

    FxReadCounter::resetFrame();
    test.expectEq(BattleFlow::update(0), false, F("choice opens without exit"));
    test.expectEq(menu.menuPointer >= 0 && menu.stack[menu.menuPointer] == BATTLE_OPTIONS,
                  true, F("battle options open"));
    test.expectEq(BattleFlow::update(MENU_EDGE_A), false, F("attack submenu opens"));
    test.expectEq(menu.menuPointer >= 0 && menu.stack[menu.menuPointer] == BATTLE_MOVE_SELECT,
                  true, F("move selection open"));
    test.expectEq(BattleFlow::update(MENU_EDGE_A), false, F("move starts battle playback"));
    test.expectEq(battlePresenter().stage() != battle::PresenterStage::Idle, true,
                  F("result presentation begins"));
    test.expectEq(battlePresenter().stage() != battle::PresenterStage::Done, true,
                  F("selection A did not accelerate announcement"));
    test.expectEq(static_cast<uint8_t>(battleSession().result().kind),
                  static_cast<uint8_t>(battle::ResultKind::Attack),
                  F("first result is an attack"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("menu and presentation transition reads"));

    const uint8_t drawsBeforeRender = rngDraws;
    const uint8_t readsBeforeRender = FxReadCounter::count();
    renderBattle();
    renderBattle();
    test.expectEq(rngDraws, drawsBeforeRender, F("render does not reroll"));
    test.expectEq(FxReadCounter::count(), readsBeforeRender,
                  F("render adds no metadata reads"));
    test.expectEq(FxReadCounter::renderExact(0), true, F("render read budget"));

    bool completed = false;
    for (uint16_t frame = 0; frame < 512; ++frame) {
        FxReadCounter::resetFrame();
        BattleFlow::update(0);
        FxReadCounter::markUpdate();
        renderBattle();
        FxReadCounter::renderExact(0);
        if (!FxReadCounter::framePassed()) {
            test.expectEq(FxReadCounter::framePassed(), true, F("playback frame read budget"));
            break;
        }
        if (battlePresenter().stage() == battle::PresenterStage::Idle) {
            completed = true;
            break;
        }
    }
    test.expectEq(completed, true, F("first result completes in bounded frames"));
    test.expectEq(gameState.state, GameState_t::BATTLE, F("ordinary result stays in battle"));
    test.expectEq(rngDraws, drawsBeforeRender, F("playback did not reroll"));
    endPreset();
}

__attribute__((noinline)) inline void trainerRefusals(FxTest &test) {
    beginPreset(BattlePresets::opening);
    const uint8_t hpBefore = battleSession().state().active[0].hp;
    selectOption(MENU_NAV_RIGHT);
    test.expectEq(static_cast<uint8_t>(battleSession().result().kind),
                  static_cast<uint8_t>(battle::ResultKind::Gather), F("trainer gather result"));
    test.expectEq((battleSession().result().flags & battle::REFUSED) != 0,
                  true, F("trainer gather refused"));
    test.expectEq(battleSession().state().gather.progress, 0,
                  F("refused gather has no acquisition"));
    test.expectEq(rngDraws, 0, F("refused gather consumes no RNG"));
    uint16_t signature = foldResult(1, battleSession().result());
    test.expectEq(runToChoice(true, signature), true,
                  F("gather refusal returns to choice"));
    test.expectEq(battleSession().state().over, false,
                  F("gather refusal leaves trainer battle live"));
    test.expectEq(battleSession().state().active[0].hp <= hpBefore, true,
                  F("opponent action may damage player"));

    selectOption(static_cast<uint8_t>(MENU_NAV_DOWN | MENU_NAV_RIGHT));
    test.expectEq(static_cast<uint8_t>(battleSession().result().kind),
                  static_cast<uint8_t>(battle::ResultKind::Escape), F("trainer escape result"));
    test.expectEq((battleSession().result().flags & battle::REFUSED) != 0,
                  true, F("trainer escape refused"));
    signature = foldResult(signature, battleSession().result());
    test.expectEq(runToChoice(true, signature), true,
                  F("escape refusal returns to choice"));
    test.expectEq(battleSession().state().over, false,
                  F("escape refusal leaves trainer battle live"));
    test.expectEq(gameState.state, GameState_t::BATTLE,
                  F("refusal did not exit battle"));
    endPreset();
}

__attribute__((noinline)) inline void trainerSwitchAndImport(FxTest &test) {
    beginPreset(BattlePresets::switch_drill, 5);
    test.expectEq(battleSession().state().active[0].hp,
                  player.creatureHPs[0], F("entry imports damaged persistent HP"));
    test.expectEq(battleSession().state().active[0].hp + 5,
                  battleSession().state().active[0].maxHp,
                  F("entry does not refill damaged player"));
    selectOption(MENU_NAV_DOWN);
    test.expectEq(menu.menuPointer >= 0 && menu.stack[menu.menuPointer] ==
                  BATTLE_CREATURE_SELECT, true, F("player party menu opens"));
    test.expectEq(BattleFlow::update(MENU_EDGE_A), false,
                  F("player selects first live bench"));
    test.expectEq(static_cast<uint8_t>(battleSession().result().kind),
                  static_cast<uint8_t>(battle::ResultKind::Switch),
                  F("voluntary switch result"));
    test.expectEq(static_cast<uint8_t>(battleSession().result().actor),
                  static_cast<uint8_t>(battle::Side::Player),
                  F("player owns switch"));
    test.expectEq(battleSession().state().activeSlot[0], 1,
                  F("player switched to original slot one"));
    uint16_t signature = foldResult(1, battleSession().result());
    test.expectEq(runToChoice(true, signature), true,
                  F("switch turn returns to choice"));
    test.expectEq(battleSession().state().bench[0][0].hp,
                  player.creatureHPs[0], F("damaged bench HP preserved"));
    endPreset();
}

__attribute__((noinline)) inline void trainerVictory(FxTest &test) {
    beginPreset(BattlePresets::switch_drill);
    RunTrace trace;
    Boundary boundary = Boundary::Choice;
    uint8_t turns = 0;
    for (; turns < 96; ++turns) {
        if (boundary == Boundary::Choice) {
            selectOption(0);
            if (battle::remainingMoveUses(battleSession().state(), battle::Side::Player, 1))
                BattleFlow::update(MENU_NAV_RIGHT);
            BattleFlow::update(MENU_EDGE_A);
        } else if (boundary == Boundary::Replacement) {
            selectFirstLiveReplacement(test);
        } else {
            break;
        }
        trace.signature = foldResult(trace.signature, battleSession().result());
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
                      F("victory frames use transition-only metadata reads"));
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
    for (uint8_t turn = 0; turn < 8; ++turn) {
        if (boundary == Boundary::Choice) {
            selectOption(0);
            BattleFlow::update(MENU_EDGE_A);
        } else if (boundary == Boundary::Replacement) {
            selectFirstLiveReplacement(test);
            ++replacements;
        } else {
            break;
        }
        trace.signature = foldResult(trace.signature, battleSession().result());
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
                      F("defeat frames use transition-only metadata reads"));
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
    const uint16_t measured = headroom(top);
    Serial.print(F("trainer controller painted headroom=")); Serial.println(measured);
    Serial.print(F("trainer controller effective headroom="));
    Serial.println(measured >= 69 ? measured - 69 : 0);
    test.expectEq(measured >= 219, true,
                  F("trainer controller preserves 150 B after USB ISR"));
    trainerRefusals(test);
    trainerSwitchAndImport(test);

}

inline void test_battletrainer(FxTest &test) {
    battlesession_test_detail::trainerVictory(test);
    battlesession_test_detail::trainerDefeat(test);
}
