#pragma once
// Resource-only device proof for the duel session chain (CreatureGathererFX-d73i.1).
//
// Paints the AVR stack and drives the *real* shipping BattleSession through the
// deep chain: trainer3 setup + 8 revealed rounds (frequency/anti-repeat history),
// per-phase update+render+FX::display frame timing (newChoice / conceal / result
// / terminal), a real both-switch round, incoming-KO and ordinary-KO HP1 cases,
// and the save chain. The stock test_stack loop stops after the first false
// advance, so it is not session proof; this suite closes that gap.

#if !defined(CGFX_DUEL_BATTLE) || !defined(CGFX_BATTLE_COMPACT_RESULT) || \
    !defined(CGFX_BATTLE_NO_FACTS)
#error "test_duel_resources requires the duel compact profile: CGFX_DUEL_BATTLE, CGFX_BATTLE_COMPACT_RESULT, CGFX_BATTLE_NO_FACTS"
#endif
#if !defined(FX_READ_COUNTER)
#error "test_duel_resources requires -DFX_READ_COUNTER so metadata-read assertions are non-vacuous"
#endif

#include "fxtest.hpp"
#include "duel_session_integration_test.hpp"
#include "stack_test.hpp"
#include "generated/opponent_data.hpp"
#include "src/lib/ReadData.hpp"

namespace duel_resource_detail {

// Deterministic always-zero draw: every read fires and ties resolve to the
// lowest index, so the AI-switch branch below is reproducible.
inline uint8_t zeroRng(uint8_t)
{
    return 0;
}

enum : uint8_t { PH_NEWCHOICE = 0, PH_CONCEAL, PH_RESULT, PH_TERMINAL, PH_COUNT };

const __FlashStringHelper *phaseName(uint8_t phase)
{
    switch (phase) {
    case PH_NEWCHOICE: return F("newChoice");
    case PH_CONCEAL: return F("conceal");
    case PH_RESULT: return F("result");
    default: return F("terminal");
    }
}

uint32_t worstByPhase[PH_COUNT];
uint32_t worstOverall;
uint8_t worstPhase;

inline uint8_t phaseOf()
{
    using namespace duel_session_test_detail;
    if (gameState.state != GameState_t::BATTLE) return PH_TERMINAL;
    if (!battleSession().isActive() || battleSession().exitReady())
        return PH_TERMINAL;
    if (battleSession().isResultDwell()) return PH_RESULT;
    if (battleSession().awaitingPlayer()) return PH_NEWCHOICE;
    return PH_CONCEAL;
}

inline void record(uint8_t phase, uint32_t dt)
{
    if (dt > worstByPhase[phase]) worstByPhase[phase] = dt;
    if (dt > worstOverall) {
        worstOverall = dt;
        worstPhase = phase;
    }
}

// Draw exactly like CreatureGathererFX.ino: scene, then the feedback arm while
// the duel result dwell/terminal is active, otherwise the (concealed or choice)
// menu arm.
inline void renderFrame()
{
    using namespace duel_session_test_detail;
    battle::BattleScene::draw(battleSession().state());
    if (battleSession().duelFeedbackActive())
        modeState.battle.ui.feedback.draw();
    else if (modeState.battle.ui.menu.initialized())
        modeState.battle.ui.menu.draw();
}

// Every timed frame goes through BattleFlow::update so the feedback/menu union
// is constructed exactly as in shipping. Render/session reads are guarded by
// the BATTLE game state: after finishBattle the battle union is inactive World.
inline void timeFlowFrame(uint8_t edges)
{
    using namespace duel_session_test_detail;
    const uint8_t phase = phaseOf();
    const unsigned long t0 = micros();
    BattleFlow::update(edges);
    if (gameState.state == GameState_t::BATTLE) {
        renderFrame();
        FX::display(CLEAR_BUFFER);
    }
    record(phase, static_cast<uint32_t>(micros() - t0));
}

// Timed trainer setup transition (beginTrainer + first-choice commit).
inline void beginTrainerTimed(uint8_t trainerId)
{
    using namespace duel_session_test_detail;
    const unsigned long t0 = micros();
    battleSession().beginTrainer(trainerId);
    const uint32_t dt = static_cast<uint32_t>(micros() - t0);
    record(PH_NEWCHOICE, dt);
}

// Drive a submitted round through BattleFlow until the next Choice/terminal.
inline void driveRound()
{
    using namespace duel_session_test_detail;
    for (uint16_t i = 0; i < 400; ++i) {
        if (gameState.state != GameState_t::BATTLE) break;
        if (!battleSession().isActive() || battleSession().exitReady() ||
            battleSession().awaitingPlayer())
            break;
        timeFlowFrame(0);
    }
}

} // namespace duel_resource_detail

inline void duel_resource_test(FxTest &test)
{
    using namespace duel_session_test_detail;
    using namespace duel_resource_detail;

    const uint8_t WaterAuthored = static_cast<uint8_t>(Type::WATER);
    const uint8_t EarthAuthored = static_cast<uint8_t>(Type::EARTH);
    const uint8_t PlantAuthored = static_cast<uint8_t>(Type::PLANT);
    const uint8_t EarthDuelBit = static_cast<uint8_t>(1u << duel::foldType(EarthAuthored));

    for (uint8_t i = 0; i < PH_COUNT; ++i) worstByPhase[i] = 0;
    worstOverall = 0;
    worstPhase = 255;

    const uint16_t base =
        reinterpret_cast<uint16_t>(&stack_fx_test_detail::__bss_end);
    const uint16_t top = stack_fx_test_detail::paintStack();

    // --- Trainer3 setup + 8 revealed rounds for EACH temporary persona group ---
    // Opponent ids come from the generated table (valid when the first creature
    // level is nonzero); id % 3 selects the temporary Reader/Frequency/
    // AntiRepeat fixture persona. switch_drill keeps enough HP for 8 rounds.
    uint8_t personaTrainerId[3] = {255, 255, 255};
    for (uint8_t id = 0; id < opponentSeedCount; ++id) {
        const OpponentSeed probe = readOpponentSeed(id);
        if (probe.firstCreature.lvl == 0) continue;
        const uint8_t group = static_cast<uint8_t>(id % 3);
        if (personaTrainerId[group] == 255) personaTrainerId[group] = id;
    }
    for (uint8_t group = 0; group < 3; ++group) {
        test.expectEq(personaTrainerId[group] != 255, true,
                      F("a valid trainer id exists for each persona group"));
        if (personaTrainerId[group] == 255) continue;

        preparePlayer();
        battle::applyPlayerPreset(player, BattlePresets::switch_drill);
        menu.clear();
        dialogMenu.clear();
        enterBattle();
        gameState.state = GameState_t::BATTLE;
        rngDraws = 0;
        battleSession().setRng({zeroRng});
        FxReadCounter::resetFrame();
        beginTrainerTimed(personaTrainerId[group]);

        timeFlowFrame(0); // newChoice: AI commit + menu text cache
        uint8_t revealed = 0;
        for (uint8_t round = 0; round < 8; ++round) {
            timeFlowFrame(0);           // choice rebuild (edge ignored)
            timeFlowFrame(MENU_EDGE_A); // open the move page
            timeFlowFrame(MENU_EDGE_A); // submit the lowest usable move
            uint16_t guard = 0;
            while (gameState.state == GameState_t::BATTLE &&
                   battleSession().isActive() &&
                   !battleSession().awaitingPlayer() &&
                   !battleSession().exitReady() && guard++ < 200)
                timeFlowFrame(0);
            if (gameState.state != GameState_t::BATTLE ||
                !battleSession().isActive() || battleSession().exitReady())
                break;
            ++revealed;
        }
        test.expectEq(revealed, static_cast<uint8_t>(8),
                      F("8 revealed rounds completed for the persona group"));
        if (gameState.state == GameState_t::BATTLE && battleSession().isActive())
            exitBattle();
    }

    // --- Real both-switch round: player SelectParty + AI-committed switch ---
    // Foe active water has no plant counter; its bench mask is forced to earth
    // (which beats plant) so the reading AI switches. Player also switches.
    {
        preparePlayer();
        battle::applyPlayerPreset(player, BattlePresets::utility);
        menu.clear();
        dialogMenu.clear();
        enterBattle();
        gameState.state = GameState_t::BATTLE;
        battleSession().setRng({zeroRng});
        beginTrainerTimed(BattlePresets::opening.trainerId);

        battle::BattleState &state =
            const_cast<battle::BattleState &>(battleSession().state());
        // Player active plant with a plant attack; history must read plant, so
        // round 1 attacks with plant.
        state.active[0].types = DualType(static_cast<Type>(PlantAuthored));
        state.active[0].hp = state.active[0].maxHp = state.active[0].stats.hp = 100;
        state.active[0].moves[0] = attackMove(PlantAuthored);
        // Foe active water, water-only kit (no plant counter).
        state.active[1].types = DualType(static_cast<Type>(WaterAuthored));
        state.active[1].hp = state.active[1].maxHp = state.active[1].stats.hp = 100;
        for (uint8_t slot = 0; slot < 4; ++slot)
            if (state.active[1].moveIds[slot] != EMPTY_MOVE_ID)
                state.active[1].moves[slot] = attackMove(WaterAuthored);
        // Foe bench (original slot 1) provides the plant counter.
        state.moveUsesSpent[1][1] = EarthDuelBit;

        // Round 1: attack with plant (records plant history).
        battleSession().submitIntent({MenuIntentKind::SelectMove, 0});
        driveRound();

        const uint8_t foeActiveBefore = state.activeSlot[1];
        const uint8_t playerActiveBefore = state.activeSlot[0];
        const uint8_t playerTarget = playerActiveBefore == 0 ? 1 : 0;
        // Snapshot each INCOMING target's own original-slot HP (different
        // creatures from the outgoing active).
        const uint8_t playerTargetBench = playerTarget < state.activeSlot[0]
            ? playerTarget : static_cast<uint8_t>(playerTarget - 1);
        const uint8_t playerTargetHp = state.bench[0][playerTargetBench].hp;
        const uint8_t foeTargetHp = state.bench[1][0].hp; // bench counter slot

        // Round 2: player switches; the reading foe commits a switch.
        const bool playerSwitchOk =
            battleSession().submitIntent({MenuIntentKind::SelectParty, playerTarget});
        test.expectEq(playerSwitchOk, true, F("player switch accepted"));
        driveRound();
        test.expectEq(state.activeSlot[1] != foeActiveBefore, true,
                      F("foe original slot changed (AI committed switch)"));
        test.expectEq(state.activeSlot[0] != playerActiveBefore, true,
                      F("player original slot changed (both switch)"));
        test.expectEq(state.active[0].hp == playerTargetHp, true,
                      F("both-switch: player incoming keeps its own HP"));
        test.expectEq(state.active[1].hp == foeTargetHp, true,
                      F("both-switch: foe incoming keeps its own HP"));
        exitBattle();
    }

    // --- ordinary KO on a real 3-member trainer: promote first living slot ---
    {
        preparePlayer();
        menu.clear();
        dialogMenu.clear();
        enterBattle();
        gameState.state = GameState_t::BATTLE;
        battleSession().setRng({zeroRng});
        battleSession().beginTrainer(BattlePresets::opening.trainerId);
        battle::BattleState &state =
            const_cast<battle::BattleState &>(battleSession().state());
        state.active[0].types = DualType(static_cast<Type>(PlantAuthored));
        state.active[0].moves[0] = attackMove(PlantAuthored); // beats fire -> KO on hp1
        state.active[0].hp = state.active[0].maxHp = state.active[0].stats.hp = 100;
        state.active[1].types = DualType(static_cast<Type>(FireAuthored));
        state.active[1].hp = state.active[1].maxHp = state.active[1].stats.hp = 1;
        // beginTrainer gave the foe real bench members (living).
        battleSession().submitIntent({MenuIntentKind::SelectMove, 0});
        driveRound();
        test.expectEq(state.activeSlot[1], 1,
                      F("ordinary KO promotes first living original slot"));
        exitBattle();
    }

    // --- Terminal case driven to exitReady ---
    {
        preparePlayer();
        beginWildDuel(false, 0);
        configureMatchup(FireAuthored, SpiritAuthored, FireAuthored, FireAuthored, 100, 5);
        BattleFlow::update(0);
        BattleFlow::update(MENU_EDGE_A);
        BattleFlow::update(MENU_EDGE_A);
        for (uint16_t i = 0; i < 400 && gameState.state == GameState_t::BATTLE &&
                            battleSession().isActive() &&
                            !battleSession().exitReady(); ++i)
            timeFlowFrame(0);
        // One more Flow update includes finishBattle/exit and its world restore.
        if (gameState.state == GameState_t::BATTLE) timeFlowFrame(0);
        // After the world restore only the world state is valid to read.
        test.expectEq(gameState.state == GameState_t::WORLD, true,
                      F("terminal reaches the world restore"));
    }

    // --- Subsequent save chain ---
    const SaveStep step = stack_fx_test_detail::runSave();

    const uint16_t low = stack_fx_test_detail::lowWater(base, top);
    const uint16_t headroom = low - base;
    Serial.print(F("headroom="));
    Serial.print(headroom);
    Serial.print(F(" frame="));
    Serial.println(worstOverall);
    test.expectEq(headroom >= 219, true, F("duel session headroom >= 219 B"));
    test.expectEq(static_cast<uint8_t>(step), static_cast<uint8_t>(SaveStep::Done),
                  F("save chain completes"));
    test.expectEq(worstOverall <= 19230, true, F("worst frame <= 19230 us"));
}
