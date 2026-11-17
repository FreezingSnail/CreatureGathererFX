#pragma once
// Permanent host proof for the compact duel menu pages (CreatureGathererFX-
// d73i.2): pick-row commit, WAIT (Pass), the SWAP bench-picker open/cancel, the
// wild-only action page, trainer suppression, and held-edge ownership.
#ifdef CGFX_DUEL_BATTLE

#include "test.hpp"
#include "src/FXDataFake.hpp"
#include "src/item/Inventory.hpp"
#include "src/lib/FxReadCounter.hpp"
#include "src/fxdata.h"
#include "../src/engine/battle/BattleMenu.hpp"
#include "../src/engine/battle/BattleState.hpp"
#include "../src/engine/menu/MenuV2.hpp"
#include <string.h>

namespace duel_menu_test_detail {

battle::BattleState oneAttackState()
{
    battle::BattleState state{};
    state.partyCount[0] = 3;
    state.activeSlot[0] = 0;
    state.bench[0][0].hp = 10;
    state.bench[0][1].hp = 10; // living bench -> the SWAP cell is offered
    // A single spirit attack (fold order bit 0); the pick row's option index 0
    // is this set bit.
    state.active[0].attackMask = 1;
    return state;
}

battle::BattleState waitState()
{
    battle::BattleState state{};
    state.partyCount[0] = 3;
    state.activeSlot[0] = 0;
    state.bench[0][0].hp = 10;
    state.bench[0][1].hp = 10;
    state.active[0].attackMask = 0; // no usable attack -> WAIT
    return state;
}

battle::BattleState fullMaskState()
{
    battle::BattleState state{};
    state.partyCount[0] = 3;
    state.activeSlot[0] = 0;
    state.bench[0][0].hp = 10;
    state.bench[0][1].hp = 10;
    state.active[0].attackMask = 0x1F; // five attack cells, then SWAP
    return state;
}

// Host fake for the authored consumable kind byte (ids keep their derived
// names through the fake index-table path).
void installItemKinds(const uint8_t kinds[8])
{
    fxDataFake::dataBase = consumable_table;
    memset(fxDataFake::dataBytes, 0, sizeof(fxDataFake::dataBytes));
    for (uint8_t id = 0; id < 8; ++id)
        fxDataFake::dataBytes[static_cast<uint16_t>(id) * 2] = kinds[id];
}

} // namespace duel_menu_test_detail

inline void DuelMenuSuite(TestSuite &suite)
{
    using namespace duel_menu_test_detail;
    Test test(__func__);
    // A battle bag pointer is a real session invariant; the suite starts with
    // an empty one and the item cases install their own filtered bags.
    item::Inventory suiteBag;
    item::inventoryClear(suiteBag);
    item::setBattleInventory(&suiteBag);

    // Pick row: A on an attack cell commits that original move slot.
    {
        battle::BattleMenu menu;
        menu.beginDuel(oneAttackState(), false, false);
        test.assert(menu.initialized(), true, "duel menu initializes the pick row");
        test.assert(menu.duelCursor(), static_cast<int8_t>(0), "cursor starts on the first cell");
        test.assert(menu.update(0).kind, MenuIntentKind::None, "a steady frame emits no intent");
        const MenuIntent intent = menu.update(MENU_EDGE_A);
        test.assert(intent.kind, MenuIntentKind::SelectMove, "A commits the attack cell");
        test.assert(intent.index, static_cast<uint8_t>(0), "the attack maps to its authored slot");
    }

    // SWAP opens the two-row picker; B cancels back to the swap cell.
    {
        battle::BattleMenu menu;
        menu.beginDuel(oneAttackState(), false, false);
        menu.update(MENU_NAV_RIGHT);
        test.assert(menu.duelCursor(), static_cast<int8_t>(1), "cursor reaches the SWAP cell");
        test.assert(menu.update(MENU_EDGE_A).kind, MenuIntentKind::None,
                    "SWAP opens the picker without an intent");
        test.assert(menu.partyPending(), true, "the picker page is pending");
        test.assert(menu.update(MENU_EDGE_B).kind, MenuIntentKind::None,
                    "B cancels the picker without an intent");
        test.assert(menu.duelCursor(), static_cast<int8_t>(1),
                    "cancel returns to the SWAP cell on the pick row");
    }

    // WAIT cell: an unusable kit offers SWAP + WAIT; A on WAIT emits Pass.
    {
        battle::BattleMenu menu;
        menu.beginDuel(waitState(), false, false);
        menu.update(MENU_NAV_RIGHT);
        test.assert(menu.duelCursor(), static_cast<int8_t>(1), "cursor reaches the WAIT cell");
        const MenuIntent intent = menu.update(MENU_EDGE_A);
        test.assert(intent.kind, MenuIntentKind::Pass, "WAIT commits Pass");
        test.assert(menu.update(MENU_EDGE_A).kind, MenuIntentKind::None,
                    "a held A does not re-send the WAIT intent");
    }

    // Wild action page: B opens it, A on the first row gathers, B returns.
    {
        battle::BattleMenu menu;
        menu.beginDuel(waitState(), true, true);
        test.assert(menu.update(MENU_EDGE_B).kind, MenuIntentKind::None,
                    "B opens the wild action page without an intent");
        test.assert(menu.update(MENU_EDGE_A).kind, MenuIntentKind::Gather,
                    "the first action row gathers");
        test.assert(menu.update(MENU_EDGE_B).kind, MenuIntentKind::None,
                    "B leaves the action page");
        test.assert(menu.duelCursor(), static_cast<int8_t>(0),
                    "the pick row is active again after the action page");
    }

    // A trainer still offers the ITEMS page; B opens it, B returns to the row.
    {
        battle::BattleMenu menu;
        menu.beginDuel(waitState(), false, false);
        menu.update(MENU_EDGE_B);
        test.assert(menu.duelCursor(), static_cast<int8_t>(-1),
                    "a trainer opens the ITEMS page on B");
        test.assert(menu.update(MENU_EDGE_B).kind, MenuIntentKind::None,
                    "B leaves the trainer action page");
        test.assert(menu.duelCursor(), static_cast<int8_t>(0),
                    "the pick row is active again after the trainer page");
        test.assert(menu.update(MENU_EDGE_A).kind, MenuIntentKind::None,
                    "A on the SWAP cell opens the bench picker");
        test.assert(menu.partyPending(), true,
                    "the trainer SWAP cell still opens the bench picker");
    }

    // Six-cell worst case: five attacks then SWAP, in order, with wrap.
    {
        battle::BattleMenu menu;
        menu.beginDuel(fullMaskState(), true, true);
        for (uint8_t option = 0; option < 5; ++option) {
            test.assert(menu.duelCursor(), static_cast<int8_t>(option),
                        "the full mask opens on the first attack cell");
            const MenuIntent intent = menu.update(MENU_EDGE_A);
            test.assert(intent.kind, MenuIntentKind::SelectMove,
                        "each attack cell selects a move");
            test.assert(intent.index, option, "attack cells keep ascending order");
            if (option < 4) menu.update(MENU_NAV_RIGHT);
        }
        menu.beginDuel(fullMaskState(), true, true);
        menu.update(MENU_NAV_LEFT); // wrap backwards onto SWAP (cell 5)
        test.assert(menu.duelCursor(), static_cast<int8_t>(5),
                    "backwards wrap lands on the sixth cell (SWAP)");
        test.assert(menu.update(MENU_EDGE_A).kind, MenuIntentKind::None,
                    "the SWAP cell opens the picker");
        test.assert(menu.partyPending(), true, "sixth cell is the SWAP picker");
    }

    // A gatherable wild keeps GATHER first on the B page, with ITEMS next and
    // ESCAPE after it.
    {
        battle::BattleMenu menu;
        battle::BattleState gatherable = fullMaskState();
        gatherable.gatherable = 1;
        menu.beginDuel(gatherable, gatherable.gatherable, 1);
        menu.update(MENU_EDGE_B);
        test.assert(menu.update(MENU_EDGE_A).kind, MenuIntentKind::Gather,
                    "GATHER stays the first action row with a full pick row");

        menu.beginDuel(gatherable, gatherable.gatherable, 1);
        menu.update(MENU_EDGE_B);
        menu.update(MENU_NAV_DOWN); // GATHER -> ITEMS
        test.assert(menu.update(MENU_EDGE_A).kind, MenuIntentKind::None,
                    "the second action row opens ITEMS without an action");
        test.assert(menu.update(MENU_EDGE_A).kind, MenuIntentKind::None,
                    "an empty Heal list selects nothing");
        menu.update(MENU_EDGE_B);   // cancel the empty bag
        test.assert(menu.duelCursor(), static_cast<int8_t>(-1),
                    "the cancel returns to the action page");
        menu.update(MENU_NAV_DOWN); // ITEMS -> ESCAPE
        test.assert(menu.update(MENU_EDGE_A).kind, MenuIntentKind::Escape,
                    "ESCAPE stays after ITEMS on the wild action page");
    }

    // The trainer ITEMS page lists only authored Heal consumables and
    // translates the visible row to its semantic consumable id.
    {
        static const uint8_t kinds[8] = {1, 1, 1, 2, 2, 2, 3, 3};
        installItemKinds(kinds);
        item::Inventory inv;
        item::inventoryClear(inv);
        item::inventoryAdd(inv, item::ItemKind::Lure, 4, 3); // never selectable
        item::inventoryAdd(inv, item::ItemKind::Consumable, 0, 2);
        item::inventoryAdd(inv, item::ItemKind::Consumable, 3, 1); // Cure
        item::setBattleInventory(&inv);

        battle::BattleMenu menu;
        menu.beginDuel(waitState(), false, false);
        menu.update(MENU_EDGE_B); // trainer action page (ITEMS only)
        FxReadCounter::resetFrame();
        test.assert(menu.update(MENU_EDGE_A).kind, MenuIntentKind::None,
                    "A on the ITEMS row opens the bag without an action");
        test.assert(FxReadCounter::count() > 0, true,
                    "opening resolves the filtered bag rows");
        test.assert(FxReadCounter::markUpdate(), true,
                    "the bag-open reads are transition-only");
        test.assert(item::inventoryCount(inv, item::ItemKind::Consumable, 0),
                    static_cast<uint8_t>(2), "opening spends no item");
        const uint8_t openReads = FxReadCounter::count();
        menu.draw(); // bag page draws from the cache only
        test.assert(FxReadCounter::count(), openReads,
                    "drawing the bag resolves no metadata");

        const MenuIntent intent = menu.update(MENU_EDGE_A);
        test.assert(intent.kind, MenuIntentKind::SelectItem,
                    "A selects the visible Heal row");
        test.assert(intent.index, static_cast<uint8_t>(0),
                    "the tonic row keeps semantic id 0");
        test.assert(item::inventoryCount(inv, item::ItemKind::Consumable, 0),
                    static_cast<uint8_t>(2), "selection alone spends nothing");
    }

    // A mixed bag skips non-Heal ids: the visible rows are the semantic ids,
    // never an aliased inventory index.
    {
        static const uint8_t kinds[8] = {2, 1, 3, 1, 2, 3, 2, 3};
        installItemKinds(kinds);
        item::Inventory inv;
        item::inventoryClear(inv);
        item::inventoryAdd(inv, item::ItemKind::Consumable, 0, 1); // Cure
        item::inventoryAdd(inv, item::ItemKind::Consumable, 1, 1); // Heal
        item::inventoryAdd(inv, item::ItemKind::Consumable, 3, 1); // Heal
        item::setBattleInventory(&inv);

        battle::BattleMenu menu;
        menu.beginDuel(waitState(), false, false);
        menu.update(MENU_EDGE_B);
        menu.update(MENU_EDGE_A);
        const MenuIntent first = menu.update(MENU_EDGE_A);
        test.assert(first.kind, MenuIntentKind::SelectItem,
                    "the first visible Heal row selects an item");
        test.assert(first.index, static_cast<uint8_t>(1),
                    "the leading Cure id is skipped, not aliased to row 0");

        menu.update(MENU_EDGE_B); // reopen the bag
        menu.update(MENU_EDGE_A);
        menu.update(MENU_NAV_DOWN);
        const MenuIntent second = menu.update(MENU_EDGE_A);
        test.assert(second.kind, MenuIntentKind::SelectItem,
                    "the second visible row selects an item");
        test.assert(second.index, static_cast<uint8_t>(3),
                    "the skipped Cure id is not aliased into row 1");

        menu.update(MENU_EDGE_B); // a third step clamps: only two rows exist
        menu.update(MENU_EDGE_A);
        menu.update(MENU_NAV_DOWN);
        menu.update(MENU_NAV_DOWN);
        menu.update(MENU_NAV_DOWN);
        const MenuIntent clamped = menu.update(MENU_EDGE_A);
        test.assert(clamped.kind, MenuIntentKind::SelectItem,
                    "the last visible row still selects");
        test.assert(clamped.index, static_cast<uint8_t>(3),
                    "the filtered list ends at the second Heal stack");
    }

    // Scrolling: a fake all-Heal bag of seven stacks moves the four-row window
    // and keeps the visible rows mapped to their ids.
    {
        static const uint8_t kinds[8] = {1, 1, 1, 1, 1, 1, 1, 2};
        installItemKinds(kinds);
        item::Inventory inv;
        item::inventoryClear(inv);
        for (uint8_t id = 0; id < 7; ++id)
            item::inventoryAdd(inv, item::ItemKind::Consumable, id, 1);
        item::setBattleInventory(&inv);

        battle::BattleMenu menu;
        menu.beginDuel(waitState(), false, false);
        menu.update(MENU_EDGE_B);
        menu.update(MENU_EDGE_A);
        for (uint8_t step = 0; step < 4; ++step) menu.update(MENU_NAV_DOWN);
        const MenuIntent scrolled = menu.update(MENU_EDGE_A);
        test.assert(scrolled.kind, MenuIntentKind::SelectItem,
                    "a scrolled row still selects an item");
        test.assert(scrolled.index, static_cast<uint8_t>(4),
                    "the scrolled cursor row keeps its semantic id");
    }

    // Cancel and reopen cost no item; a held edge never reuses activation.
    {
        static const uint8_t kinds[8] = {1, 1, 1, 2, 2, 2, 3, 3};
        installItemKinds(kinds);
        item::Inventory inv;
        item::inventoryClear(inv);
        item::inventoryAdd(inv, item::ItemKind::Consumable, 0, 1);
        item::setBattleInventory(&inv);

        battle::BattleMenu menu;
        menu.beginDuel(oneAttackState(), false, false);
        menu.update(MENU_EDGE_B); // action page, ITEMS first for a trainer
        menu.update(MENU_EDGE_A); // bag open
        test.assert(menu.update(0).kind, MenuIntentKind::None,
                    "a steady bag frame emits no intent");
        test.assert(menu.update(MENU_EDGE_B).kind, MenuIntentKind::None,
                    "B cancels the bag to the action page");
        test.assert(menu.duelCursor(), static_cast<int8_t>(-1),
                    "the action page still owns the frame after a cancel");
        menu.update(MENU_EDGE_B); // action -> pick
        test.assert(menu.duelCursor(), static_cast<int8_t>(0),
                    "the pick row is back after cancel/reopen");
        menu.update(MENU_EDGE_B); // reopen action
        test.assert(menu.update(MENU_EDGE_A).kind, MenuIntentKind::None,
                    "the reopening A only opens the bag");
        test.assert(item::inventoryCount(inv, item::ItemKind::Consumable, 0),
                    static_cast<uint8_t>(1), "cancel/reopen consume no item");
        item::setBattleInventory(nullptr);
    }

    suite.addTest(test);
}

#endif // CGFX_DUEL_BATTLE
