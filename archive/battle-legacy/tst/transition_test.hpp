#pragma once

#include "test.hpp"
#include "encounter_test.hpp"
#include "../src/engine/ModeState.hpp"
#include "../src/engine/battle/BattleFlow.hpp"
#include "../src/engine/menu/MenuV2.hpp"
#include "../src/globals.hpp"

namespace transition_test_detail {

void finishBattle(Test &test) {
    uint16_t frames = 0;
    while (frames < 4096 && gameState.state == GameState_t::BATTLE) {
        BattleFlow::update(0);
        ++frames;
    }
    test.assert(frames < 4096, true, "terminal battle presentation exits within frame budget");
    test.assert(gameState.state, GameState_t::WORLD,
                "terminal battle transition restores world mode");
    test.assert(menu.menuPointer, static_cast<int8_t>(-1),
                "terminal battle transition clears menu state");
    test.assert(dialogMenu.peek(), false,
                "terminal battle transition clears dialog state");
    test.assert(WorldEngine::location(), gameState.playerLocation,
                "rebuilt world synchronizes to persistent player location");
    test.assert(worldState().motion.step, static_cast<uint8_t>(0),
                "rebuilt world clears partial movement state");
}

void prepareEncounter(uint16_t tile) {
    if (gameState.state == GameState_t::BATTLE) exitBattle();
    encounter_test_detail::prepareWorld(tile, true);
    onStep(tile);
}

void EncounterFleeRestoresPositionAndHp(TestSuite &suite) {
    Test test(__func__);
    const uint16_t tile = 0x0203;
    prepareEncounter(tile);
    test.assert(gameState.state, GameState_t::BATTLE,
                "encounter step enters battle mode");
    test.assert(Chunk::chunkOfLocation(gameState.playerLocation),
                Chunk::chunkOfLocation(tile), "battle keeps encounter chunk location");

    battleSession().stateForTest().active[static_cast<uint8_t>(battle::Side::Player)].hp = 7;
    menu.push(BATTLE_OPTIONS);
    PopUpDialog popup = {};
    popup.type = SCRIPT_TEXT;
    popup.textAddress = 1;
    dialogMenu.pushMenu(popup);
    test.assert(battleSession().submitIntent({MenuIntentKind::Escape, 0}), true,
                "wild encounter accepts flee intent");
    BattleFlow::update(0);
    finishBattle(test);

    test.assert(gameState.playerLocation, tile,
                "flee returns to saved overworld tile");
    test.assert(Chunk::chunkOfLocation(gameState.playerLocation),
                Chunk::chunkOfLocation(tile), "flee returns to saved overworld chunk");
    test.assert(player.creatureHPs[0], static_cast<uint8_t>(7),
                "flee persists battle damage through the Player HP owner");
    suite.addTest(test);
}

void LossRestoresPosition(TestSuite &suite) {
    Test test(__func__);
    const uint16_t tile = 0x0203;
    prepareEncounter(tile);
    battle::BattleSession &session = battleSession();
    test.assert(session.submitIntent({MenuIntentKind::Gather, 0}), true,
                "wild encounter accepts an action before terminal loss");
    session.stateForTest().over = 1;
    BattleFlow::update(0);
    finishBattle(test);
    test.assert(gameState.playerLocation, tile,
                "loss returns to saved overworld tile");
    test.assert(Chunk::chunkOfLocation(gameState.playerLocation),
                Chunk::chunkOfLocation(tile), "loss returns to saved overworld chunk");
    suite.addTest(test);
}

void WinRestoresPosition(TestSuite &suite) {
    Test test(__func__);
    const uint16_t tile = 0x0203;
    prepareEncounter(tile);
    battle::BattleSession &session = battleSession();
    battle::Combatant &playerCombatant =
        session.stateForTest().active[static_cast<uint8_t>(battle::Side::Player)];
    battle::Combatant &opponent =
        session.stateForTest().active[static_cast<uint8_t>(battle::Side::Opponent)];
    playerCombatant.stats.speed = 255;
    playerCombatant.stats.attack = 255;
    playerCombatant.moves[0] = Move(MoveBitSet{0, 31, 1, 0, 0});
    opponent.stats.defense = 0;
    opponent.hp = 1;
    test.assert(session.submitIntent({MenuIntentKind::SelectMove, 0}), true,
                "wild encounter accepts winning attack");
    BattleFlow::update(0);
    finishBattle(test);
    test.assert(gameState.playerLocation, tile,
                "win returns to saved overworld tile");
    test.assert(Chunk::chunkOfLocation(gameState.playerLocation),
                Chunk::chunkOfLocation(tile), "win returns to saved overworld chunk");
    suite.addTest(test);
}

} // namespace transition_test_detail

inline void TransitionSuite(TestRunner &runner) {
    TestSuite suite("Encounter battle transitions");
    transition_test_detail::EncounterFleeRestoresPositionAndHp(suite);
    transition_test_detail::LossRestoresPosition(suite);
    transition_test_detail::WinRestoresPosition(suite);
    runner.addTestSuite(suite);
}
