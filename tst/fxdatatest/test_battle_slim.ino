#include "harness/fx_globals.hpp"
#include "fxtest.hpp"
#include "src/engine/battle/BattleMenu.hpp"
#include "src/engine/battle/BattleFeedback.hpp"
#include "src/engine/battle/BattleFlow.hpp"
#include "src/engine/battle/MoveUses.hpp"
#include "src/engine/battle/BattleScene.hpp"
#include "src/engine/ModeState.hpp"
#include "src/engine/menu/MenuNav.hpp"
#include "src/lib/MoveIds.hpp"
#include "src/lib/FxReadCounter.hpp"

#if !CGFX_BATTLE_AUTO_REPLACE
static bool reachPlayerReplacement(FxTest &test, uint8_t playerCount) {
    player = Player();
    player.loadCreature(0, 1);
    player.loadCreature(1, 2);
    player.loadCreature(2, 3);
    enterBattle();
    gameState.state = GameState_t::BATTLE;
    battleSession().beginTrainer(0);

    battle::BattleState &state =
        const_cast<battle::BattleState &>(battleSession().state());
    state.active[0].hp = 1;
    state.active[0].stats.defense = 1;
    state.active[0].stats.spcDef = 1;
    state.active[0].stats.speed = 1;
    state.partyCount[0] = playerCount;
    state.bench[0][0].hp = playerCount > 1 ? 24 : 0;
    state.bench[0][1].hp = playerCount > 2 ? 25 : 0;
    state.active[1].stats.attack = 255;
    state.active[1].stats.spcAtk = 255;
    state.active[1].stats.speed = 255;
    state.active[1].moveIds[0] = state.active[0].moveIds[0];
    state.active[1].moves[0] = state.active[0].moves[0];
    state.moveUsesSpent[0][1] = 1;
    state.moveUsesSpent[0][2] = 1;
    state.partyModifiers[0][1] = 0x15;
    state.partyModifiers[0][2] = 0x2a;

    BattleFlow::update(0); // Open the options page.
    test.expectEq(modeState.battle.ui.menu.initialized(), true,
                  F("battle flow initializes the compact menu"));
    BattleFlow::update(MENU_EDGE_A); // Open the move page.
    const uint8_t playerUsesBefore = battle::remainingMoveUses(
        battleSession().state(), battle::Side::Player, 0);
    const uint8_t activeSpentBefore = state.moveUsesSpent[0][0];
    BattleFlow::update(MENU_EDGE_A); // The player action is frozen here.
    test.expectEq(battleSession().isPresenting(), false,
                  F("player edge submits without resolving immediately"));
    BattleFlow::update(0); // Faster opponent knocks the submitted actor out.
    test.expectEq(static_cast<uint8_t>(battleSession().result().kind),
                  static_cast<uint8_t>(battle::ResultKind::Attack),
                  F("opponent attack resolves before the frozen player action"));
    test.expectEq(static_cast<uint8_t>(battleSession().result().actor),
                  static_cast<uint8_t>(battle::Side::Opponent),
                  F("opponent owns the knockout action"));
    test.expectEq(battleSession().state().active[0].hp, 0,
                  F("opponent attack knocks out the frozen player actor"));
    test.expectEq(battle::remainingMoveUses(battleSession().state(),
                  battle::Side::Player, 0), playerUsesBefore,
                  F("invalidated frozen actor spends no move use"));

    for (uint8_t frame = 0; frame < 40 &&
         !battleSession().awaitingReplacement(); ++frame)
        BattleFlow::update(MENU_EDGE_A);
    test.expectEq(battleSession().awaitingReplacement(), true,
                  F("flow reaches the forced replacement boundary"));
    BattleFlow::update(0); // Construct the filtered, original-slot menu.
    test.expectEq(modeState.battle.ui.menu.initialized(), true,
                  F("replacement boundary opens the direct menu"));
    if (playerCount == 2) {
        BattleFlow::update(MENU_EDGE_B);
        test.expectEq(battleSession().awaitingReplacement(), true,
                      F("sole survivor cannot leave without confirmation"));
        test.expectEq(battleSession().isPresenting(), false,
                      F("sole-survivor cancel edge does not resolve"));
    }
    if (playerCount > 2) BattleFlow::update(MENU_NAV_DOWN);
    const uint8_t selectedOriginal = playerCount > 2 ? 2 : 1;
    BattleFlow::update(MENU_EDGE_A);
    test.expectEq(battleSession().isPresenting(), false,
                  F("replacement selection edge does not apply the switch"));
    BattleFlow::update(0);
    test.expectEq(static_cast<uint8_t>(battleSession().result().kind),
                  static_cast<uint8_t>(battle::ResultKind::Switch),
                  F("next boundary applies the accepted replacement"));
    test.expectEq(static_cast<uint8_t>(battleSession().result().actor),
                  static_cast<uint8_t>(battle::Side::Player),
                  F("player owns the forced replacement"));
    test.expectEq(battleSession().state().activeSlot[0], selectedOriginal,
                  F("second visible row maps to original party slot two"));
    test.expectEq(battleSession().state().active[0].id,
                  player.party[selectedOriginal].id,
                  F("replacement loads the selected original creature"));
    test.expectEq(battleSession().state().active[0].statMods.modifiers,
                  state.partyModifiers[0][selectedOriginal],
                  F("replacement restores modifiers by original slot"));
    test.expectEq((battleSession().state().moveUsesSpent[0][selectedOriginal] >> 0) & 3,
                  1, F("replacement retains PP ownership by original slot"));
    const uint8_t replacementUsesBefore = battle::remainingMoveUses(
        battleSession().state(), battle::Side::Player, 0);
    const uint8_t replacementSpentBefore =
        battleSession().state().moveUsesSpent[0][selectedOriginal];

    // Complete replacement feedback and advance the remaining frozen turn.
    for (uint8_t frame = 0; frame < 40 && battleSession().isPresenting(); ++frame)
        BattleFlow::update(MENU_EDGE_A);
    BattleFlow::update(0);
    test.expectEq(static_cast<uint8_t>(battleSession().result().kind),
                  static_cast<uint8_t>(battle::ResultKind::EndTurn),
                  F("invalidated actor is skipped after replacement"));
    test.expectEq(battleSession().state().moveUsesSpent[0][0], activeSpentBefore,
                  F("replacement gains no extra attack in the frozen turn"));
    test.expectEq(battleSession().state().moveUsesSpent[0][selectedOriginal],
                  replacementSpentBefore,
                  F("replacement actor spends no PP before a new choice"));
    test.expectEq(battle::remainingMoveUses(battleSession().state(),
                  battle::Side::Player, 0), replacementUsesBefore,
                  F("replacement move PP stays unchanged through continuation"));
    exitBattle();
    gameState.state = GameState_t::WORLD;
    return true;
}
#endif

static_assert(sizeof(battle::MoveSnapshot) == 9, "move snapshot ABI");
static_assert(sizeof(battle::PartySnapshot) == 8, "party snapshot ABI");
static_assert(sizeof(battle::BattleMenu) == 27, "compact battle menu ABI");
static_assert(sizeof(battle::BattleFeedback) == 10, "compact feedback ABI");
static_assert(sizeof(battle::BattleMode) <= 191, "session plus UI union fits overlay");

void setup() {
    fxTestSetup();
    FxTest test;
    test.expectEq(sizeof(battle::MoveSnapshot), 9, F("move snapshot ABI"));
    test.expectEq(sizeof(battle::PartySnapshot), 8, F("party snapshot ABI"));
    test.expectEq(sizeof(battle::BattleUi), 27, F("UI union ABI"));
    test.expectEq(sizeof(battle::BattleMode), 190, F("session plus UI overlay ABI"));

#if !CGFX_BATTLE_AUTO_REPLACE
    reachPlayerReplacement(test, 3);
    reachPlayerReplacement(test, 2);
#endif

    player = Player();
    player.loadCreature(0, 1);
    player.loadCreature(1, 2);
    player.loadCreature(2, 3);
    enterBattle();
    gameState.state = GameState_t::BATTLE;
    battleSession().beginTrainer(0);
    BattleFlow::update(0); // Initialize direct menu once at the choice boundary.
    test.expectEq(modeState.battle.ui.menu.initialized(), true,
                  F("trainer turn initializes compact menu"));
    BattleFlow::update(MENU_EDGE_A); // Open the move page.
    BattleFlow::update(MENU_EDGE_A);
    test.expectEq(battleSession().isPresenting(), false,
                  F("selection edge does not also resolve"));
    BattleFlow::update(0);
    test.expectEq(battleSession().isPresenting(), true,
                  F("accepted action transfers union to feedback"));
    modeState.battle.ui.feedback.draw();

    for (uint16_t frame = 0; frame < 4096 &&
         !battleSession().awaitingPlayer() && !battleSession().exitReady(); ++frame)
        BattleFlow::update(0);
    test.expectEq(battleSession().awaitingPlayer(), true,
                  F("feedback and frozen opponent action return to choice"));
    BattleFlow::update(0);
    battle::BattleScene::draw(battleSession().state());
    modeState.battle.ui.menu.draw();

    // The options root blits the packed four-item battleOptions48 art; the
    // cursor selects its matching frame.
    battle::MoveSnapshot noMoves = {};
    for (uint8_t slot = 0; slot < 4; ++slot) {
        noMoves.moveIds[slot] = EMPTY_MOVE_ID;
        noMoves.remainingUses[slot] = 0;
    }
    battle::BattleMenu actions;
    actions.beginOptions(noMoves, false, false);
    arduboy.clear();
    actions.draw();
    uint8_t art[256];
    FX::readDataBytes(battleOptions48 + 4, art, 256);
    bool optionsPixels = true;
    for (uint16_t i = 0; i < 256; ++i)
        if (Arduboy2Base::sBuffer[6 * 128 + i] != art[i]) optionsPixels = false;
    test.expectEq(optionsPixels, true,
                  F("options page blits the packed four-item menu art"));
    actions.update(MENU_NAV_RIGHT);
    arduboy.clear();
    actions.draw();
    FX::readDataBytes(battleOptions48 + 4 + 256, art, 256);
    bool cursorPixels = true;
    for (uint16_t i = 0; i < 256; ++i)
        if (Arduboy2Base::sBuffer[6 * 128 + i] != art[i]) cursorPixels = false;
    test.expectEq(cursorPixels, true,
                  F("options cursor selects the matching art frame"));

    // CHANGE opens the pending party page; its rows cache living bench names.
    actions.update(MENU_NAV_LEFT);
    actions.update(MENU_NAV_DOWN);
    actions.update(MENU_EDGE_A);
    test.expectEq(actions.partyPending(), true,
                  F("CHANGE opens the pending party page"));
    battle::BattleView partyView = {};
    partyView.partyCount[0] = 3;
    partyView.activeSlot[0] = 0;
    partyView.party[0][0] = {0, 10, 1};
    partyView.party[0][1] = {1, 12, 1};
    partyView.party[0][2] = {2, 14, 1};
    FxReadCounter::resetFrame();
    actions.beginParty(partyView);
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(2),
                  F("party transition caches both living bench names"));
    const MenuIntent party = actions.update(MENU_EDGE_A);
    test.expectEq(static_cast<uint8_t>(party.kind),
                  static_cast<uint8_t>(MenuIntentKind::SelectParty),
                  F("party row emits SelectParty"));
    test.expectEq(party.index, static_cast<uint8_t>(1),
                  F("first party row keeps its original bench slot"));
#if !CGFX_BATTLE_AUTO_REPLACE
    battle::ReplacementChoices choices = {};
    choices.count = 2;
    choices.ids[0] = 0;
    choices.slots[0] = 1;
    choices.ids[1] = 1;
    choices.slots[1] = 2;
    battle::BattleMenu replacement;
    FxReadCounter::resetFrame();
    replacement.beginReplacement(choices);
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(2),
                  F("replacement transition caches both names"));
    FxReadCounter::resetFrame();
    replacement.update(MENU_NAV_DOWN);
    replacement.draw();
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(0),
                  F("replacement menu stays metadata-free after transition"));
    test.expectEq(replacement.update(MENU_EDGE_A).index, static_cast<uint8_t>(2),
                  F("second visible row selects its original party slot"));
    choices.count = 1;
    choices.ids[0] = 0;
    choices.slots[0] = 1;
    replacement.beginReplacement(choices);
    test.expectEq(static_cast<uint8_t>(replacement.update(MENU_EDGE_B).kind),
                  static_cast<uint8_t>(MenuIntentKind::None),
                  F("B cannot cancel forced replacement"));
    test.expectEq(replacement.update(MENU_EDGE_A).index, static_cast<uint8_t>(1),
                  F("sole survivor still requires explicit confirmation"));
    choices.ids[0] = 255;
    FxReadCounter::resetFrame();
    replacement.beginReplacement(choices);
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(0),
                  F("sentinel species skips metadata reads"));
    test.expectEq(static_cast<uint8_t>(replacement.update(MENU_EDGE_A).kind),
                  static_cast<uint8_t>(MenuIntentKind::None),
                  F("sentinel replacement cannot be chosen"));
#endif
    exitBattle();
    gameState.state = GameState_t::WORLD;
    test.report(F("test_battle_slim"));
}

void loop() { exit(0); }
