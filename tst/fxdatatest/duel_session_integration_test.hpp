#pragma once
// Device proof for the RPS duel real session (CreatureGathererFX-d73i.5).
//
// Drives the resident battle::BattleSession through BattleFlow::update: the
// Choice-entry concealed commitment, the 29 Concealed frame updates, the 68
// Result dwell frames, the atomic round, WAIT and the gather ordering. The pure
// kernel relations/personas stay in battle_duel_test.hpp; this suite proves the
// session/flow wiring on AVR.
//
// Wired by the sketch tst/fxdatatest/test_duel_session.ino:
//   #include "harness/fx_globals.hpp"   // non-minimal globals
//   #include "duel_session_integration_test.hpp"
//   ... test_duel_session(test); test.report(F("test_duel_session"));
//
// Fixtures are built from authored BattlePresets kits (applyPlayerPreset) and
// their real loaded Move/id pairs; only the packed move TYPE is retyped, which
// the plan allows as a demo fixture.

#if !defined(CGFX_DUEL_BATTLE) || !defined(CGFX_BATTLE_COMPACT_RESULT) || \
    !defined(CGFX_BATTLE_NO_FACTS)
#error "duel_session_integration_test requires the duel compact profile: CGFX_DUEL_BATTLE, CGFX_BATTLE_COMPACT_RESULT, CGFX_BATTLE_NO_FACTS"
#endif

// The suite's metadata-read assertions are meaningless as no-ops. Require the
// counter so a future build that drops FX_READ_COUNTER fails loudly instead of
// passing vacuously.
#ifndef FX_READ_COUNTER
#error "duel_session_integration_test requires FX_READ_COUNTER so metadata-read assertions cannot pass as no-ops"
#endif

#include "fxtest.hpp"
#include "generated/reward_data.hpp"
#include "src/engine/battle/DuelView.hpp"
#include "src/engine/battle/BattleFlow.hpp"
#include "src/engine/battle/BattlePresets.hpp"
#include "src/engine/battle/BattleScene.hpp"
#include "src/engine/battle/BattleSession.hpp"
#include "src/engine/ModeState.hpp"
#include "src/engine/game/Gamestate.hpp"
#include "src/engine/menu/MenuV2.hpp"
#include "src/lib/MoveIds.hpp"
#include "src/lib/FxReadCounter.hpp"
#include "src/item/Inventory.hpp"
#include "src/player/Player.hpp"
#include "src/save/Compaction.hpp"
#include "src/creature/LevelCurve.hpp"

namespace duel_session_test_detail {

// Frozen duel timing owned by BattleSession (see its DUEL_*_TICKS).
constexpr uint16_t kConcealTicks = 29;
constexpr uint16_t kResultTicks = 68;

uint8_t rngDraws = 0;

uint8_t scriptedRng(uint8_t bound) {
    ++rngDraws;
    return bound == 0 ? 0 : static_cast<uint8_t>((rngDraws - 1) % bound);
}

const uint8_t SpiritAuthored = static_cast<uint8_t>(Type::SPIRIT);
const uint8_t FireAuthored = static_cast<uint8_t>(Type::FIRE);

void preparePlayer() {
    battle::applyPlayerPreset(player, BattlePresets::opening);
}

void beginWildDuel(bool gatherable, uint8_t tierRate) {
    menu.clear();
    dialogMenu.clear();
    enterBattle();
    gameState.state = GameState_t::BATTLE;
    rngDraws = 0;
    battleSession().setRng({scriptedRng});
    FxReadCounter::resetFrame();
    battleSession().beginWild(4, 1, gatherable, tierRate);
}

// Known matchup on the resident session. The meaningful player slot keeps its
// authored move id; only its packed type changes (retype fixture). The foe's
// authored-usable slots are all retyped to one type, so whichever option the
// Choice-entry commit picked maps here. No record indices are invented.
__attribute__((noinline)) void configureMatchup(uint8_t playerType, uint8_t playerMove,
                                                uint8_t opponentType, uint8_t opponentMove,
                                                uint8_t playerHp, uint8_t opponentHp) {
    battle::BattleState &state =
        const_cast<battle::BattleState &>(battleSession().state());
    battle::Combatant &hero = state.active[static_cast<uint8_t>(battle::Side::Player)];
    hero.types = DualType(static_cast<Type>(playerType));
    hero.hp = hero.maxHp = playerHp;
    // One attack option: its folded type is option 0 (the lowest set bit).
    hero.attackMask = static_cast<uint8_t>(1u << duel::foldType(playerMove));

    battle::Combatant &foe = state.active[static_cast<uint8_t>(battle::Side::Opponent)];
    foe.types = DualType(static_cast<Type>(opponentType));
    foe.hp = foe.maxHp = opponentHp;
    // Collapse the foe's authored kit to one type so whichever option the
    // Choice-entry commit picked maps here; an unusable kit stays WAIT.
    if (foe.attackMask != 0)
        foe.attackMask = static_cast<uint8_t>(1u << duel::foldType(opponentMove));
}

// Advance through the Concealed phase to the reveal (Result phase begins).
bool runToResult(battle::BattleSession &session, uint16_t &ticks) {
    for (ticks = 0; ticks < 400; ++ticks) {
        if (session.isResultDwell()) return true;
        session.advance();
    }
    return session.isResultDwell();
}

// Mirror the production creature sketch draw dispatch exactly (CreatureGathererFX.ino
// BATTLE case): feedback owns the Result dwell and the pending-terminal frame
// (isResultDwell || exitReady), otherwise the initialized menu arm draws. Kept in
// lockstep with that predicate so the read check measures the real union arm.
__attribute__((noinline)) void renderBattle() {
    // Lockstep with the production BATTLE dispatch: the scene owns the pick row,
    // centre and result footer from resident state; the menu paints only its
    // picker/action pages.
    battle::BattleScene::draw(battleSession().state(),
                              modeState.battle.ui.menu.duelCursor());
    if (battleSession().state().feedback == 0 &&
        modeState.battle.ui.menu.initialized())
        modeState.battle.ui.menu.draw();
}

// One ordinary round through the live controller: 29 concealed frames with no
// early HP change and no held-edge skip, then 68 metadata-free result frames.
__attribute__((noinline)) void duelFlowConcealAndDwell(FxTest &test) {
    preparePlayer();
    beginWildDuel(false, 0);
    configureMatchup(FireAuthored, SpiritAuthored, FireAuthored, FireAuthored, 100, 100);

    test.expectEq(BattleFlow::update(0), 0, F("choice frame stays in battle"));
    test.expectEq(modeState.battle.ui.menu.initialized(), 1, F("pick row opens"));
    BattleFlow::update(MENU_EDGE_A); // pick row: commit the selected attack cell
    test.expectEq(battleSession().isResultDwell(), 0,
                  F("selection never resolves on its own edge"));
    test.expectEq(static_cast<uint32_t>(battleSession().result().kind),
                  static_cast<uint32_t>(battle::ResultKind::None),
                  F("no result before the reveal"));

    const uint8_t playerHp = battleSession().state().active[0].hp;
    const uint8_t opponentHp = battleSession().state().active[1].hp;
    uint16_t conceal = 0;
    bool concealReads = true;
    while (!battleSession().isResultDwell() && conceal < 200) {
        if (battleSession().state().active[0].hp != playerHp) concealReads = false;
        if (battleSession().state().active[1].hp != opponentHp) concealReads = false;
        FxReadCounter::resetFrame();
        BattleFlow::update(MENU_EDGE_A); // a held edge must not skip the interval
        if (!battleSession().isResultDwell() && FxReadCounter::count() != 0)
            concealReads = false;
        ++conceal;
    }
    test.expectEq(conceal, kConcealTicks, F("exactly 29 concealed frame updates"));
    test.expectEq(concealReads, 1, F("no early HP change or concealed-frame FX read"));
    test.expectEq(battleSession().consumeResultEntry(), 0,
                  F("result entry consumed on the reveal frame"));
    test.expectEq(battleSession().isResultDwell(), 1, F("reveal begins the result dwell"));
    test.expectEq(battleSession().state().active[1].hp, 79, F("win lands 21 at reveal"));
    test.expectEq(battleSession().state().active[0].hp, 96, F("chip lands 4 at reveal"));

    uint16_t dwell = 0;
    bool dwellReads = true;
    bool boundaryConsumed = false;
    while (battleSession().isResultDwell() && dwell < 400) {
        FxReadCounter::resetFrame();
        BattleFlow::update(0);
        const bool stillResult = battleSession().isResultDwell();
        renderBattle();
        if (stillResult && gameState.state == GameState_t::BATTLE) {
            // A steady result frame (still Result after the update) performs no
            // metadata reads; captions were prepared on the reveal transition.
            if (FxReadCounter::count() != 0) {
                dwellReads = false;
                Serial.print(F("duel dwell read leak tick="));
                Serial.print(dwell);
                Serial.print(F(" reads="));
                Serial.println(FxReadCounter::count());
            }
        } else {
            // The 68th update transitions to Choice and legitimately reads
            // move-name metadata to build the next menu. It must also have
            // consumed the choice UI entry on that same frame.
            boundaryConsumed = !battleSession().consumeChoiceEntry();
        }
        ++dwell;
    }
    test.expectEq(dwell, kResultTicks, F("exactly 68 result frame updates"));
    test.expectEq(dwellReads, 1, F("steady result frames stay metadata-free"));
    test.expectEq(boundaryConsumed, 1, F("the boundary frame consumes the choice entry"));
    test.expectEq(battleSession().awaitingPlayer(), 1, F("returns to a fresh choice"));
}

// An unusable kit exposes only empty cells; the normal menu route then submits
// WAIT (Pass), which commits no attack and takes the opponent's default win.
__attribute__((noinline)) void duelFlowWaitRoute(FxTest &test) {
    preparePlayer();
    beginWildDuel(false, 0);
    battle::BattleState &state =
        const_cast<battle::BattleState &>(battleSession().state());
    state.active[0].attackMask = 0;
    state.active[0].types = DualType(static_cast<Type>(FireAuthored));
    state.active[0].hp = state.active[0].maxHp = 100;
    state.active[1].types = DualType(static_cast<Type>(FireAuthored));
    state.active[1].hp = state.active[1].maxHp = 100;
    state.active[1].attackMask = static_cast<uint8_t>(1u << duel::foldType(SpiritAuthored));

    test.expectEq(duel::usableOptionCount(state, 0), 0,
                  F("an unusable kit has no attack options"));
    const battle::MoveSnapshot snapshot = battleSession().moves();
    bool allEmpty = true;
    for (uint8_t slot = 0; slot < 4; ++slot)
        if (snapshot.moveIds[slot] != EMPTY_MOVE_ID) allEmpty = false;
    test.expectEq(allEmpty, 1, F("an unusable kit exposes only empty cells"));

    BattleFlow::update(0);               // consume the choice entry: pick row
    BattleFlow::update(MENU_NAV_RIGHT);  // cursor -> WAIT (empty kit: swap then wait)
    BattleFlow::update(MENU_EDGE_A);     // WAIT cell: Pass
    test.expectEq(battleSession().isResultDwell(), 0, F("WAIT starts concealed"));
    uint16_t revealTicks = 0;
    while (!battleSession().isResultDwell() && revealTicks < 200) {
        BattleFlow::update(0);
        ++revealTicks;
    }
    test.expectEq(revealTicks, kConcealTicks, F("WAIT also reveals after 29 frames"));
    // Spirit beats Fire -> 14 x1.5 = 21 into the waiting side, zero back.
    test.expectEq(battleSession().state().active[0].hp, 79, F("WAIT takes the default win"));
    test.expectEq(battleSession().state().active[1].hp, 100, F("WAIT deals no damage"));
}

// Immediate wild escape through the normal menu route resolves straight into
// the Result phase; the flow must build the feedback arm on that same update.
__attribute__((noinline)) void duelFlowEscapeEntrySameFrame(FxTest &test) {
    preparePlayer();
    beginWildDuel(true, 1);
    BattleFlow::update(0);              // choice entry -> pick row
    BattleFlow::update(MENU_EDGE_B);    // open the wild action page
    BattleFlow::update(MENU_NAV_DOWN);  // cursor -> ESCAPE
    BattleFlow::update(MENU_EDGE_A);    // submit Escape -> immediate Result
    test.expectEq(battleSession().isResultDwell(), 1,
                  F("escape enters the result dwell immediately"));
    test.expectEq(static_cast<uint32_t>(battleSession().result().kind),
                  static_cast<uint32_t>(battle::ResultKind::Escape),
                  F("the escape result is published"));
    test.expectEq(battleSession().consumeResultEntry(), 0,
                  F("escape constructs feedback on the same update"));
    test.expectEq(battleSession().state().feedback,
                  static_cast<uint8_t>(duel::Feedback::Escape),
                  F("the footer shows the escaped caption"));
}

__attribute__((noinline)) void duelDeviceSimultaneousDraw(FxTest &test) {
    preparePlayer();
    player.creatureHPs[1] = 0;
    player.creatureHPs[2] = 0;
    beginWildDuel(false, 0);
    configureMatchup(SpiritAuthored, SpiritAuthored, SpiritAuthored, SpiritAuthored, 5, 5);

    test.expectEq(battleSession().submitIntent({MenuIntentKind::SelectMove, 0}), 1,
                  F("the mirror trade submits"));
    uint16_t revealTicks = 0;
    test.expectEq(runToResult(battleSession(), revealTicks), 1, F("the trade reveals"));
    test.expectEq(battleSession().state().active[0].hp, 0, F("player active faints"));
    test.expectEq(battleSession().state().active[1].hp, 0, F("opponent active faints"));
    test.expectEq(static_cast<uint32_t>(battleSession().result().outcome),
                  static_cast<uint32_t>(battle::Outcome::Draw),
                  F("simultaneous KO draws"));
    test.expectEq((battleSession().result().flags & battle::PLAYER_FAINTED) != 0, 1,
                  F("player faint flagged"));
    test.expectEq((battleSession().result().flags & battle::OPPONENT_FAINTED) != 0, 1,
                  F("opponent faint flagged"));
    for (uint16_t tick = 0; tick < 400 && battleSession().isResultDwell(); ++tick)
        battleSession().advance();
    test.expectEq(battleSession().exitReady(), 1, F("draw is terminal"));
    test.expectEq(player.creatureHPs[0], 0, F("draw syncs original slot 0"));
    test.expectEq(battleSession().state().feedback,
                  static_cast<uint8_t>(duel::Feedback::Draw),
                  F("a simultaneous KO shows the DRAW caption"));
}

__attribute__((noinline)) void duelDeviceSwapKoRevert(FxTest &test) {
    preparePlayer();
    beginWildDuel(false, 0);

    battle::BattleState &state =
        const_cast<battle::BattleState &>(battleSession().state());
    // Physically real post-switch shape: a 0 -> 2 switch leaves original slot 2
    // active with originals 0 and 1 living on the bench in party order.
    const uint8_t slot0Id = player.party[0].id;
    const uint8_t slot0Level = player.party[0].level;
    player.party[0].types = DualType(static_cast<Type>(FireAuthored));

    state.partyCount[0] = 3;
    state.activeSlot[0] = 2;
    state.active[0].id = player.party[2].id;
    state.active[0].level = player.party[2].level;
    state.active[0].types = DualType(static_cast<Type>(SpiritAuthored));
    state.active[0].hp = state.active[0].maxHp = 50;

    state.bench[0][0].id = slot0Id;
    state.bench[0][0].level = slot0Level;
    state.bench[0][0].hp = 10; // incoming: dies to the default win
    state.bench[0][1].id = player.party[1].id;
    state.bench[0][1].level = player.party[1].level;
    state.bench[0][1].hp = 50; // living first-live slot that must NOT be chosen

    state.activeSlot[1] = 0;
    state.active[1].types = DualType(static_cast<Type>(SpiritAuthored));
    state.active[1].hp = state.active[1].maxHp = 100;
    state.active[1].attackMask = static_cast<uint8_t>(1u << duel::foldType(SpiritAuthored));

    test.expectEq(battleSession().submitIntent({MenuIntentKind::SelectParty, 0}), 1,
                  F("the living incoming slot is selectable"));
    uint16_t revealTicks = 0;
    test.expectEq(runToResult(battleSession(), revealTicks), 1, F("the swap round reveals"));
    test.expectEq(state.activeSlot[0], 2,
                  F("incoming KO restores the outgoing original slot"));
    test.expectEq(state.active[0].id, player.party[2].id,
                  F("the restored member is the outgoing creature"));
    test.expectEq(state.active[0].hp, 50,
                  F("the restored member keeps its original-slot HP"));
}

__attribute__((noinline)) void duelDeviceGatherKoKeepsProgress(FxTest &test) {
    preparePlayer();
    player.creatureHPs[1] = 0;
    player.creatureHPs[2] = 0;
    beginWildDuel(true, 8); // gatherable, need 8, tierRate 8
    configureMatchup(FireAuthored, FireAuthored, FireAuthored, SpiritAuthored, 5, 100);

    test.expectEq(battleSession().submitIntent({MenuIntentKind::Gather, 0}), 1,
                  F("a wild gather submits"));
    uint16_t revealTicks = 0;
    test.expectEq(runToResult(battleSession(), revealTicks), 1, F("the gather round reveals"));
    test.expectEq(static_cast<uint32_t>(battleSession().result().kind),
                  static_cast<uint32_t>(battle::ResultKind::Gather),
                  F("result kind is the gather"));
    test.expectEq(battleSession().state().gather.progress, 8,
                  F("progress is banked before the hit"));
    test.expectEq(static_cast<uint32_t>(battleSession().result().outcome),
                  static_cast<uint32_t>(battle::Outcome::Lose),
                  F("the knockout blocks acquisition"));
    test.expectEq(battleSession().state().feedback,
                  static_cast<uint8_t>(duel::Feedback::Lose),
                  F("a terminal knockout shows the defeat caption, not gathered"));
}

// A gather that KOs the active while a reserve lives pauses the flee countdown
// and does not acquire; the reserve promotes.
__attribute__((noinline)) void duelDeviceGatherActiveKoPausesFlee(FxTest &test) {
    preparePlayer();
    player.creatureHPs[0] = 1;
    player.creatureHPs[1] = 50;
    player.creatureHPs[2] = 0;
    beginWildDuel(true, 1); // need 8, tierRate 1
    battle::BattleState &state =
        const_cast<battle::BattleState &>(battleSession().state());
    state.gather.progress = 5;
    state.gather.fleeTurns = 1;
    configureMatchup(FireAuthored, FireAuthored, FireAuthored, SpiritAuthored, 1, 100);

    test.expectEq(battleSession().submitIntent({MenuIntentKind::Gather, 0}), 1,
                  F("a wild gather submits"));
    uint16_t revealTicks = 0;
    test.expectEq(runToResult(battleSession(), revealTicks), 1, F("the gather round reveals"));
    test.expectEq(battleSession().state().gather.progress, 6,
                  F("progress is banked before the hit"));
    test.expectEq(static_cast<uint32_t>(battleSession().result().outcome),
                  static_cast<uint32_t>(battle::Outcome::None),
                  F("an active KO neither acquires nor flees while a reserve lives"));
    test.expectEq(battleSession().state().gather.fleeTurns, 1,
                  F("the flee countdown pauses on an active KO"));
    test.expectEq(state.activeSlot[0], 1, F("the living reserve promotes"));
    test.expectEq(battleSession().state().feedback,
                  static_cast<uint8_t>(duel::Feedback::Gather),
                  F("a progress-only gather keeps the gather caption"));
}

// jp8.11.7: a successful wild gather commits exactly one duplicate-capable
// instance to the existing 64-slot v3 collection through the real flow terminal,
// carrying the foe's species/level/mask and its actual HP under the pinned byte7
// encoding; the terminal never re-commits.
__attribute__((noinline)) void duelDeviceGatherCommit(FxTest &test) {
    storeFresh(); // empty collection, cold journal
    preparePlayer();
    beginWildDuel(true, 1);
    battle::BattleState &state =
        const_cast<battle::BattleState &>(battleSession().state());
    state.gather.need = 1; // one surviving gather acquires
    state.gather.progress = 0;
    configureMatchup(SpiritAuthored, SpiritAuthored, FireAuthored, SpiritAuthored, 100, 100);
    battle::Combatant &foe = state.active[static_cast<uint8_t>(battle::Side::Opponent)];
    foe.maxHp = 100;
    foe.hp = 55; // damaged: byte7 must encode 56, not a full-heal zero
    const uint8_t foeId = foe.id;
    const uint8_t foeLevel = foe.level;
    const uint8_t foeMask = foe.attackMask;

    BattleFlow::update(0);              // choice entry -> pick row
    BattleFlow::update(MENU_EDGE_B);    // wild action page (GATHER first)
    BattleFlow::update(MENU_EDGE_A);    // submit GATHER
    uint16_t frames = 0;
    while (gameState.state == GameState_t::BATTLE && frames < 4000) {
        BattleFlow::update(0);
        ++frames;
    }
    test.expectEq(gameState.state, static_cast<uint32_t>(GameState_t::WORLD),
                  F("gather terminal returns to the world"));

    StoreRecord committed = {};
    test.expectEq(storeRead(0, committed), true, F("gathered instance readable"));
    test.expectEq(committed.id, foeId, F("gathered species committed"));
    test.expectEq(committed.exp, levelExpThreshold(foeLevel),
                  F("fresh level-threshold exp committed"));
    test.expectEq(committed.attackMask, foeMask, F("folded mask committed"));
    test.expectEq(committed.hpEncoded, 56, F("damaged HP uses value-1 encoding"));
    test.expectEq(storeInstanceHP(committed), 55, F("damaged gathered HP preserved"));
    uint8_t occupied = 0;
    for (uint8_t i = 0; i < 64; ++i) {
        StoreRecord slot;
        storeRead(i, slot);
        if (slot.id != 255) ++occupied;
    }
    test.expectEq(occupied, 1, F("exactly one gathered instance committed"));
    for (uint8_t i = 0; i < 8; ++i) BattleFlow::update(0);
    occupied = 0;
    for (uint8_t i = 0; i < 64; ++i) {
        StoreRecord slot;
        storeRead(i, slot);
        if (slot.id != 255) ++occupied;
    }
    test.expectEq(occupied, 1, F("later frames never re-commit"));
}

// --- jp8.11.4: ordinary wild KO material award ------------------------------

// A live wild session ready for a one-hit Win: the foe is a 1-HP Fire creature
// and the player uses its Fire kit to pick Spirit (spirit beats fire, 21 > 1) so
// the foe is KO'd on the reveal. The one shared Player bag is installed/cleared.
void beginWildKoSession(uint8_t foeSpecies, bool gatherable) {
    preparePlayer();
    item::inventoryClear(player.inventory);
    item::setBattleInventory(&player.inventory);
    menu.clear();
    dialogMenu.clear();
    enterBattle();
    gameState.state = GameState_t::BATTLE;
    rngDraws = 0;
    battleSession().setRng({scriptedRng});
    battleSession().beginWild(foeSpecies, 1, gatherable, gatherable ? 1 : 0);
    configureMatchup(FireAuthored, SpiritAuthored, FireAuthored, FireAuthored, 100, 1);
}

// Drive the live flow through the Win dwell to the world terminal, then read the
// bag. Returns the awarded count for the species' authored material.
uint8_t wildKoAwardedCount(uint8_t species) {
    BattleFlow::update(0);              // choice entry -> pick row
    BattleFlow::update(MENU_EDGE_A);    // commit the single attack cell
    for (uint16_t f = 0; f < 4096 && gameState.state != GameState_t::WORLD; ++f)
        BattleFlow::update(0);
    return item::inventoryCount(player.inventory, item::ItemKind::Lure,
                                pgm_read_byte(RewardFixture::materialItem + species));
}

// jp8.11.4: an ordinary wild KO awards the species' authored material (from the
// generated mapping the items_test already proved equals the packed Rewards
// bytes) exactly once; sentinel species and gatherable KOs award nothing; a full
// bag saturates. Every expectation uses authored values, never a literal index.
__attribute__((noinline)) void duelDeviceWildKoMaterialAward(FxTest &test) {
    const uint8_t item = pgm_read_byte(RewardFixture::materialItem + 0);
    const uint8_t count = pgm_read_byte(RewardFixture::materialCount + 0);
    test.expectEq(item < 24, true, F("authored material is a lure"));
    test.expectEq(static_cast<uint8_t>(item % 3), static_cast<uint8_t>(2),
                  F("authored material is tier 2"));

    beginWildKoSession(0, false);
    test.expectEq(wildKoAwardedCount(0), count,
                  F("ordinary wild KO awards the authored material"));
    test.expectEq(item::inventoryNonZeroCount(player.inventory), static_cast<uint8_t>(1),
                  F("the award touches exactly one stack"));
    for (uint8_t i = 0; i < 8; ++i) BattleFlow::update(0);
    test.expectEq(item::inventoryCount(player.inventory, item::ItemKind::Lure, item),
                  count, F("later frames never re-award"));

    // 0xFF sentinel species: no authored material, no phantom item.
    test.expectEq(pgm_read_byte(RewardFixture::materialItem + 4),
                  static_cast<uint8_t>(0xFF), F("species 4 is a no-material sentinel"));
    beginWildKoSession(4, false);
    wildKoAwardedCount(4);
    test.expectEq(item::inventoryNonZeroCount(player.inventory), static_cast<uint8_t>(0),
                  F("a sentinel species awards no phantom item"));

    // Gatherable-target KO: the lure-zone encounter shape awards no material.
    beginWildKoSession(0, true);
    wildKoAwardedCount(0);
    test.expectEq(item::inventoryNonZeroCount(player.inventory), static_cast<uint8_t>(0),
                  F("a gatherable-target KO awards no creature material"));

    // At the count cap the terminal still completes and the stack saturates.
    beginWildKoSession(0, false);
    item::inventoryAdd(player.inventory, item::ItemKind::Lure, item, 99);
    test.expectEq(wildKoAwardedCount(0), static_cast<uint8_t>(99),
                  F("a full bag stays at the cap"));
    test.expectEq(item::inventoryNonZeroCount(player.inventory), static_cast<uint8_t>(1),
                  F("a full bag gains no phantom stack"));
}

} // namespace duel_session_test_detail

// Asymmetric trade through the real session (CreatureGathererFX-d73i.8 parity):
// damage is RECEIVED. Player creature Fire, foe Water, both pick Spirit -> player
// outgoing weak (3) hits the foe, foe outgoing bonus (9) hits the player. The
// footer's outgoing order draws damage[opponent] first, so state.damage[0]=9 and
// state.damage[1]=3.
__attribute__((noinline)) void duelDeviceTradeScalesOutgoing(FxTest &test) {
    using namespace duel_session_test_detail;
    preparePlayer();
    beginWildDuel(false, 0);
    configureMatchup(FireAuthored, SpiritAuthored,
                     static_cast<uint8_t>(Type::WATER), SpiritAuthored, 100, 100);

    test.expectEq(battleSession().submitIntent({MenuIntentKind::SelectMove, 0}), 1,
                  F("the spirit mirror trade submits"));
    uint16_t revealTicks = 0;
    test.expectEq(runToResult(battleSession(), revealTicks), 1, F("the trade reveals"));
    test.expectEq(battleSession().state().active[0].hp, 91,
                  F("player receives the foe's bonus outgoing"));
    test.expectEq(battleSession().state().active[1].hp, 97,
                  F("foe receives the player's weak outgoing"));
    test.expectEq(battleSession().state().damage[0], 9, F("footer damage player"));
    test.expectEq(battleSession().state().damage[1], 3, F("footer damage foe"));
    test.expectEq(battleSession().state().feedback,
                  static_cast<uint8_t>(duel::Feedback::Trade), F("trade caption"));
}

inline void test_duel_session(FxTest &test) {
    using namespace duel_session_test_detail;
    duelFlowConcealAndDwell(test);
    duelFlowWaitRoute(test);
    duelFlowEscapeEntrySameFrame(test);
    duelDeviceSimultaneousDraw(test);
    duelDeviceSwapKoRevert(test);
    duelDeviceGatherKoKeepsProgress(test);
    duelDeviceGatherActiveKoPausesFlee(test);
    duelDeviceTradeScalesOutgoing(test);
    duelDeviceGatherCommit(test);
    duelDeviceWildKoMaterialAward(test);
}
