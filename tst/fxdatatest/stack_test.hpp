#pragma once

#include "fxtest.hpp"
#include "src/audio/Audio.hpp"
#include "src/engine/battle/BattleFlow.hpp"
#include "src/engine/menu/CollectionMenu.hpp"
#include "src/engine/menu/InventoryMenu.hpp"
#include "src/engine/world/LureZone.hpp"
#include "src/item/Inventory.hpp"
#include "src/save/Compaction.hpp"
#include "src/save/FlashBackend.hpp"
#include "src/save/SaveFile.hpp"
#include "src/vm/opcodes.hpp"

namespace stack_fx_test_detail {
extern "C" uint8_t __bss_end;

constexpr uint8_t PAINT_BYTE = 0xC5;
constexpr uint8_t PAINT_MARGIN = 64;
// Preserve the project's 150 B minimum reserve after budgeting 69 B for the
// USB ISR. This suite now measures 359 B for the save chain (290 B after the
// ISR) and 271 B for the menu chain (202 B after the ISR).
// rhs7: the menu chain is measured with the same paint after the save chain.
// Linking the real world/collection/heal menu paths also links their file
// statics, so __bss_end (the base of every headroom number here) moves up; the
// menu-chain print separates that from the save-chain figure.
constexpr uint16_t MIN_HEADROOM = 219;

inline uint16_t paintStack()
{
    const uint16_t base = reinterpret_cast<uint16_t>(&__bss_end);
    const uint16_t top = static_cast<uint16_t>(SP) - PAINT_MARGIN;
    for (volatile uint8_t *cursor = reinterpret_cast<volatile uint8_t *>(base);
         reinterpret_cast<uint16_t>(cursor) < top; ++cursor) {
        *cursor = PAINT_BYTE;
    }
    return top;
}

inline uint16_t lowWater(uint16_t base, uint16_t top)
{
    if (top <= base) {
        return base;
    }
    for (volatile uint8_t *cursor = reinterpret_cast<volatile uint8_t *>(base);
         reinterpret_cast<uint16_t>(cursor) < top; ++cursor) {
        if (*cursor != PAINT_BYTE) {
            return reinterpret_cast<uint16_t>(cursor);
        }
    }
    return top;
}

inline void printAddress(const __FlashStringHelper *name, uint16_t value)
{
    Serial.print(name);
    Serial.print(F("=0x"));
    Serial.print(value, HEX);
    Serial.print(F(" ("));
    Serial.print(value);
    Serial.println(F(")"));
}

// Keep the large save snapshot off test_stack's entry frame so the paint starts
// above the live stack. The snapshot is allocated only for the save chain,
// which is measured on its own before the menu chain runs.
__attribute__((noinline)) inline SaveStep runSave()
{
    SaveFile state = {};
#if CGFX_AUDIO
    // The save flash window runs with music paused so its measured stack does
    // not include the audio timer ISR.
    audio::pause();
#endif
    saveBegin();
    SaveStep step = SaveStep::Idle;
    // The v3 checkpoint verifies both 4096-byte regions in 64-byte chunks, so a
    // full save needs well over the former 64-step bound.
    for (uint16_t attempts = 0; attempts < 256 && saveInProgress(); ++attempts) {
        step = saveStepAdvance(state);
        FX::waitWhileBusy();
    }
#if CGFX_AUDIO
    audio::resume();
#endif
    return step;
}

inline StoreRecord menuCreature(uint8_t id, uint16_t exp, uint8_t hpEncoded)
{
    StoreRecord record = {};
    record.id = id;
    record.exp = exp;
    record.attackMask = 2;
    record.hpEncoded = hpEncoded;
    return record;
}

// rhs7: the world BAG/COLLECTION root, list and party-slot pages plus the
// confirmed swap run through the same entry points WorldEngine::update calls:
// collectionMenuWorldInput()/collectionMenuWorldApplyChoice() with the real
// four-row caches, the real v3 store and the real FX name lookups. The store is
// provisioned the same way the collection device suite does it (erase both
// checkpoint sectors, start fresh, add two instances) because the painted suite
// otherwise starts from an uninitialised store. Rows are ascending stable
// slots: row 0 is the outgoing member, row 1 the incoming duplicate. No
// shipping code runs differently; this frame only adds call depth below the
// paint.
__attribute__((noinline)) inline void runWorldMenuChain(FxTest &test)
{
    const uint16_t pages[] = {32, 48};
    for (uint8_t i = 0; i < 2; ++i) {
        flash.eraseSector(pages[i]);
        while (flash.busy()) {}
    }
    storeFresh();
    player = Player();
    item::Inventory &bag = LureZone::inventory();
    item::inventoryAdd(bag, item::ItemKind::Lure, item::lureId(0, 0), 1);

    // Outgoing member in party slot 0, unassigned duplicate of the same species
    // one stable slot later: the confirmed swap mirrors the outgoing HP and
    // installs the incoming record, exactly like the shipping collection page.
    StoreRecord outgoing = menuCreature(3, 126, 8); // 7 current HP
    StoreRecord incoming = menuCreature(3, 200, 4); // 3 current HP
    uint8_t outgoingSlot = 255;
    uint8_t incomingSlot = 255;
    storeAdd(outgoing, outgoingSlot);
    storeAdd(incoming, incomingSlot);
    storeAssign(0, outgoingSlot, 7);
    player.party[0].load(outgoing);
    player.creatureHPs[0] = 7;
    player.party[1].id = 255;
    player.creatureHPs[1] = 0;
    player.party[2].id = 255;
    player.creatureHPs[2] = 0;

    // Fresh B opens the root; A opens the bag page over the file-static bag
    // cache; the bag draw is the world overlay's own render call.
    collectionMenuWorldInput(INVENTORY_MENU_B, true, bag);
    collectionMenuWorldDraw(bag);
    collectionMenuWorldInput(INVENTORY_MENU_A, false, bag);
    collectionMenuWorldInput(INVENTORY_MENU_DOWN, false, bag);
    collectionMenuWorldDraw(bag);
    collectionMenuWorldInput(INVENTORY_MENU_B, false, bag);

    // Root -> COLLECTION opens the four-row list (bounded store scan + the
    // visible name lookups); scrolling refreshes the window.
    collectionMenuWorldInput(INVENTORY_MENU_DOWN, false, bag);
    collectionMenuWorldInput(INVENTORY_MENU_A, false, bag);
    collectionMenuWorldDraw(bag);
    collectionMenuWorldInput(INVENTORY_MENU_DOWN, false, bag);
    collectionMenuWorldInput(INVENTORY_MENU_UP, false, bag);
    collectionMenuWorldInput(INVENTORY_MENU_DOWN, false, bag);

    // A opens the party-slot page for the incoming row; the slot cursor moves
    // before the confirm, so the chosen party slot is 1.
    collectionMenuWorldInput(INVENTORY_MENU_A, false, bag);
    collectionMenuWorldInput(INVENTORY_MENU_DOWN, false, bag);
    collectionMenuWorldInput(INVENTORY_MENU_UP, false, bag);
    collectionMenuWorldDraw(bag);
    collectionMenuWorldInput(INVENTORY_MENU_A, false, bag);
    // The world update consumes the confirmed choice on the following frame.
    collectionMenuWorldApplyChoice(player);
    collectionMenuWorldDraw(bag);
    // The fixture must reach the full swap path, not the InParty refusal: the
    // outgoing HP mirror and the incoming install are the deep part of the
    // measured chain.
    test.expectEq(static_cast<uint8_t>(collectionMenuWorldFeedback()),
                  static_cast<uint8_t>(CollectionFeedback::None),
                  F("party swap consumes the choice"));
    test.expectEq(storePartySlot(0), incomingSlot,
                  F("party swap installs the incoming slot"));

    // List -> root -> closed so a battle entry never runs behind the overlay.
    collectionMenuWorldInput(INVENTORY_MENU_B, false, bag);
    collectionMenuWorldInput(INVENTORY_MENU_B, false, bag);
}

// rhs7: the duel B action page and its cached Heal picker run through
// BattleFlow::update()/BattleMenu::draw(), the same seams the sketch's battle
// frame uses. One authored Heal consumable reaches the real Heal page; the
// action page is drawn before the ITEMS row opens it.
__attribute__((noinline)) inline void runDuelHealChain()
{
    player = Player();
    player.basic();
    item::inventoryClear(player.inventory);
    item::inventoryAdd(player.inventory, item::ItemKind::Consumable, 0, 1);
    enterBattle();
    // Non-gatherable wild: the B page's single row is ITEMS, so A opens the bag.
    battleSession().beginWild(4, 1, false, 0);
    BattleFlow::update(0);           // choice entry -> pick row
    BattleFlow::update(MENU_EDGE_B); // action page
    battle::BattleMenu &battleMenu = modeState.battle.ui.menu;
    battleMenu.draw();               // action page paint
    BattleFlow::update(MENU_EDGE_A); // opens the cached Heal bag
    battleMenu.draw();               // Heal picker paint
    BattleFlow::update(0);           // steady picker frame
    exitBattle();
}

__attribute__((noinline)) inline void measureModeTransition(FxTest &test)
{
    const uint16_t base = reinterpret_cast<uint16_t>(&__bss_end);
    const uint16_t top = paintStack();
    enterBattle();
    const uint16_t low = lowWater(base, top);
    const uint16_t headroom = low - base;
    printAddress(F("mode transition headroom"), headroom);
    test.expectEq(headroom >= 150, true,
                  F("mode transition preserves 150 B stack reserve"));
}
} // namespace stack_fx_test_detail

using namespace stack_fx_test_detail;

inline void test_stack(FxTest &test)
{
    const uint16_t base = reinterpret_cast<uint16_t>(&__bss_end);
    const uint16_t top = paintStack();

    exitBattle();
    uint8_t *script = worldState().script;
    script[0] = static_cast<uint8_t>(VmOpcode::Tp);
    script[1] = 0;
    script[2] = 4;
    script[3] = 0;
    script[4] = 4;
    script[5] = static_cast<uint8_t>(VmOpcode::End);
    vm.initVM(script, 0, 0);
    vm.run();

    script[0] = static_cast<uint8_t>(VmOpcode::TpIf);
    memset(script + 1, 0, 8);
    script[9] = static_cast<uint8_t>(VmOpcode::End);
    vm.initWarpVM(script, 0, 0);
    vm.runWarpVM();

    PopUpDialog scriptDialog = {};
    scriptDialog.type = SCRIPT_TEXT;
    scriptDialog.textAddress = 0;
    dialogMenu.pushMenu(scriptDialog);
    dialogMenu.drawPopMenu();
    dialogMenu.clear();
    enterBattle();

    player.basic();
    battleSession().beginTrainer(0);
#ifdef CGFX_DUEL_BATTLE
    // Drive the real duel chain through BattleSession::advance(): Choice entry
    // commit, the 29-frame concealed interval, reveal/resolution, the 68-frame
    // result dwell, KO replacement and the terminal boundary. advance() alone
    // returning false at the choice frames is why the legacy loop below is not
    // duel proof.
    for (uint16_t step = 0; step < 20000 && !battleSession().exitReady(); ++step) {
        if (battleSession().awaitingPlayer())
            battleSession().submitIntent({MenuIntentKind::SelectMove, 0});
        battleSession().advance();
    }
#else
    menu.printMenu(battleSession().view());
    battleSession().submitIntent({MenuIntentKind::SelectMove, 0});
    for (uint8_t action = 0; action < 8 && !battleSession().exitReady(); ++action) {
        if (!battleSession().advance()) break;
        battleSession().finishPresentation();
    }
#endif
    battleSession().syncPlayerHp();
    exitBattle();

    // The save chain is measured alone first: world mode is restored, and the
    // world/collection/heal chain below runs afterwards so its deeper frames do
    // not contaminate this existing number.
    const SaveStep step = runSave();
    const uint16_t saveLow = lowWater(base, top);
    const uint16_t saveHeadroom = saveLow - base;
    printAddress(F("stack base"), base);
    printAddress(F("stack top"), top);
    printAddress(F("stack low"), saveLow);
    printAddress(F("stack headroom"), saveHeadroom);
    test.expectEq(static_cast<uint8_t>(step), static_cast<uint8_t>(SaveStep::Done),
                  F("save compaction completes"));
    test.expectEq(saveLow > base, true, F("stack does not collide with globals"));
    test.expectEq(saveHeadroom >= MIN_HEADROOM, true, F("stack headroom >= 219 B"));

    // rhs7: paint the world root/bag/list/party/swap chain and the duel
    // action/Heal page over the same paint. The base is unchanged, so this
    // second low-water figure isolates the menu call depth.
    runWorldMenuChain(test);
    runDuelHealChain();
    const uint16_t menuLow = lowWater(base, top);
    const uint16_t menuHeadroom = menuLow - base;
    printAddress(F("menu chain low"), menuLow);
    printAddress(F("menu chain headroom"), menuHeadroom);
    test.expectEq(menuHeadroom >= MIN_HEADROOM, true,
                  F("menu chain headroom >= 219 B"));
}
