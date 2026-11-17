#define FX_GLOBALS_MINIMAL
#include "harness/fx_globals.hpp"
#include "items_test.hpp"
#include "src/engine/battle/BattleMenu.hpp"
#include "src/engine/battle/BattleState.hpp"
#include "src/engine/battle/BattleView.hpp"
#include "src/engine/menu/InventoryMenu.hpp"
#include "src/item/Inventory.hpp"
#include "src/lib/FxReadCounter.hpp"

// jp8.4.10: the four-row bag page (world and duel Heal filter) resolves item
// names and the authored Heal kinds only on open/scroll. Drawing, in-window
// moves, an empty bag and an all-refused Heal page are steady: zero lookups.
namespace items_read_test_detail {

void bagPage(FxTest &test) {
    item::Inventory inv;
    item::inventoryClear(inv);
    InventoryMenu menu = {};

    FxReadCounter::resetFrame();
    inventoryMenuOpen(menu, inv);
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(0),
                  F("empty bag open reads nothing"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("empty bag open is steady"));
    drawInventoryMenu(menu, inv);
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(0),
                  F("empty bag draw reads nothing"));
    test.expectEq(FxReadCounter::renderExact(0), true,
                  F("empty bag render reads no metadata"));
    test.expectEq(static_cast<uint8_t>(inventoryMenuSelect(menu).kind),
                  static_cast<uint8_t>(MenuIntentKind::None),
                  F("empty bag selects nothing"));

    // Four lures keep two name parts, the consumable one.
    for (uint8_t id = 0; id < 4; ++id)
        item::inventoryAdd(inv, item::ItemKind::Lure, id, 1);
    item::inventoryAdd(inv, item::ItemKind::Consumable, 4, 1);

    FxReadCounter::resetFrame();
    inventoryMenuOpen(menu, inv);
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(8),
                  F("bag open resolves four lure rows"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("bag open is a transition"));
    FxReadCounter::resetFrame();
    for (uint8_t step = 0; step < 3; ++step)
        inventoryMenuMove(menu, inv, 1);
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(0),
                  F("in-window bag moves read nothing"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("in-window bag move is steady"));
    FxReadCounter::resetFrame();
    inventoryMenuMove(menu, inv, 1);
    test.expectEq(menu.view.windowStart, static_cast<uint8_t>(1),
                  F("fourth step scrolls the window"));
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(7),
                  F("bag scroll resolves the shifted window"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("bag scroll is a transition"));
    drawInventoryMenu(menu, inv);
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(7),
                  F("bag draw reads only the cache"));
    test.expectEq(FxReadCounter::renderExact(0), true,
                  F("bag render reads no metadata"));
}

void healPage(FxTest &test) {
    item::Inventory inv;
    item::inventoryClear(inv);
    item::inventoryAdd(inv, item::ItemKind::Lure, 4, 1);
    item::inventoryAdd(inv, item::ItemKind::Consumable, 0, 1); // Heal
    item::inventoryAdd(inv, item::ItemKind::Consumable, 3, 1); // Cure
    InventoryMenu menu = {};

    FxReadCounter::resetFrame();
    inventoryMenuOpenHeal(menu, inv);
    // Two authored kind records (ids 0 and 3) + the one visible Heal name.
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(3),
                  F("Heal open resolves its filter and rows"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("Heal open is a transition"));
    FxReadCounter::resetFrame();
    inventoryMenuMoveHeal(menu, inv, 1);
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(0),
                  F("Heal cursor move reads nothing"));
    drawInventoryMenu(menu, inv);
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(0),
                  F("Heal draw reads nothing"));
    test.expectEq(FxReadCounter::renderExact(0), true,
                  F("Heal render reads no metadata"));
    const MenuIntent chosen = inventoryMenuSelectHeal(menu);
    test.expectEq(static_cast<uint8_t>(chosen.kind),
                  static_cast<uint8_t>(MenuIntentKind::SelectItem),
                  F("A selects the cached Heal row"));
    test.expectEq(chosen.index, static_cast<uint8_t>(0),
                  F("selection keeps the semantic consumable id"));

    // Lures only: the Heal page is a real empty page and opens at zero.
    item::inventoryClear(inv);
    item::inventoryAdd(inv, item::ItemKind::Lure, 0, 1);
    FxReadCounter::resetFrame();
    inventoryMenuOpenHeal(menu, inv);
    test.expectEq(menu.view.itemCount, static_cast<uint8_t>(0),
                  F("Heal page lists no lures"));
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(0),
                  F("empty Heal page opens at zero"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("empty Heal open is steady"));
    test.expectEq(static_cast<uint8_t>(inventoryMenuSelectHeal(menu).kind),
                  static_cast<uint8_t>(MenuIntentKind::None),
                  F("empty Heal page selects nothing"));
}

void nameTable(FxTest &test) {
    FxReadCounter::resetFrame();
    const item::ItemName lure = item::itemNameAddr(item::ItemKind::Lure, 0);
    test.expectEq(lure.parts, static_cast<uint8_t>(2), F("lure name has two parts"));
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(2),
                  F("lure name resolves two table entries"));
    test.expectEq(FxReadCounter::transitionExact(2), true,
                  F("lure name transition is exact"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("name lookup leaves the steady budget empty"));

    FxReadCounter::resetFrame();
    const item::ItemName consumable =
        item::itemNameAddr(item::ItemKind::Consumable, 0);
    test.expectEq(consumable.parts, static_cast<uint8_t>(1),
                  F("consumable name has one part"));
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(1),
                  F("consumable name resolves one entry"));
    test.expectEq(FxReadCounter::markUpdate(), false,
                  F("an undeclared lookup fails the steady budget"));

    FxReadCounter::resetFrame();
    test.expectEq(item::itemNameAddr(item::ItemKind::Lure, 255).parts,
                  static_cast<uint8_t>(0), F("out-of-range lure has no name"));
    test.expectEq(item::itemNameAddr(item::ItemKind::Key, 0).parts,
                  static_cast<uint8_t>(0), F("unsupported kind has no name"));
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(0),
                  F("invalid names read nothing"));
}

} // namespace items_read_test_detail

// jp8.4.10: the duel BattleMenu pages. The pick row and action page build from
// resident state; the Heal bag caches its authored filter/rows on open; the
// shared party page (voluntary SWAP and forced replacement rows) caches one
// name per living row. Steady updates and every draw read no metadata.
namespace battle_read_test_detail {

constexpr uint8_t kEdgeB = 1u << 2;
constexpr uint8_t kEdgeA = 1u << 3;
constexpr uint8_t kNavDown = 1u << 4;
constexpr uint8_t kNavLeft = 1u << 5;
constexpr uint8_t kNavRight = 1u << 6;

void pages(FxTest &test) {
    battle::BattleState state = {};
    state.partyCount[0] = 3;
    state.activeSlot[0] = 0;
    state.active[0].attackMask = 1; // one attack cell, then SWAP
    state.bench[0][0].hp = 10;
    state.bench[0][1].hp = 10;

    battle::BattleMenu menu;
    FxReadCounter::resetFrame();
    menu.beginDuel(state, false, false);
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(0),
                  F("pick row builds from resident state"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("pick-row entry is steady"));
    menu.update(kNavRight);
    menu.update(kNavLeft);
    menu.update(kNavDown);
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(0),
                  F("pick-row navigation reads nothing"));

    FxReadCounter::resetFrame();
    menu.update(kEdgeB);
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(0),
                  F("action page reads nothing"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("action page is a steady transition"));

    item::Inventory bag;
    item::inventoryClear(bag);
    item::inventoryAdd(bag, item::ItemKind::Lure, 4, 3);      // never listed
    item::inventoryAdd(bag, item::ItemKind::Consumable, 0, 1); // Heal
    item::inventoryAdd(bag, item::ItemKind::Consumable, 3, 1); // Cure
    item::setBattleInventory(&bag);
    FxReadCounter::resetFrame();
    menu.update(kEdgeA); // the ITEMS row opens the Heal page
    // Two authored kind records (ids 0 and 3) plus the one visible Heal name.
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(3),
                  F("Heal page open resolves its filter and rows"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("Heal page open is a transition"));
    FxReadCounter::resetFrame();
    menu.update(kNavDown);
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(0),
                  F("in-window item move keeps the cache"));
    menu.draw();
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(0),
                  F("Heal page draw reads nothing"));
    test.expectEq(FxReadCounter::renderExact(0), true,
                  F("Heal page render reads no metadata"));
    FxReadCounter::resetFrame();
    const MenuIntent picked = menu.update(kEdgeA);
    test.expectEq(static_cast<uint8_t>(picked.kind),
                  static_cast<uint8_t>(MenuIntentKind::SelectItem),
                  F("A selects the cached Heal row"));
    test.expectEq(picked.index, static_cast<uint8_t>(0),
                  F("selection keeps the semantic consumable id"));
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(0),
                  F("item selection reads nothing"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("item selection is steady"));

    // An empty Heal page (lures only) opens at zero and refuses A.
    item::inventoryClear(bag);
    item::inventoryAdd(bag, item::ItemKind::Lure, 0, 1);
    battle::BattleMenu emptyMenu;
    emptyMenu.beginDuel(state, false, false);
    emptyMenu.update(kEdgeB);
    FxReadCounter::resetFrame();
    emptyMenu.update(kEdgeA);
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(0),
                  F("empty Heal page opens at zero"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("empty Heal open is steady"));
    const MenuIntent refused = emptyMenu.update(kEdgeA);
    test.expectEq(static_cast<uint8_t>(refused.kind),
                  static_cast<uint8_t>(MenuIntentKind::None),
                  F("empty Heal page refuses A"));
    emptyMenu.update(kEdgeB);

    // The shared party-page transition: one name per living, in-range row.
    battle::BattleView view = {};
    view.activeSlot[0] = 0;
    view.partyCount[0] = 3;
    view.party[0][0] = {2, 40, 1}; // active slot is skipped
    view.party[0][1] = {5, 60, 1}; // living bench row
    view.party[0][2] = {7, 0, 0};  // KO row resolves nothing

    battle::BattleState noAttack = state;
    noAttack.active[0].attackMask = 0; // SWAP is the first cell
    battle::BattleMenu partyMenu;
    partyMenu.beginDuel(noAttack, false, false);
    FxReadCounter::resetFrame();
    partyMenu.update(kEdgeA);
    test.expectEq(partyMenu.partyPending(), true,
                  F("SWAP opens the party page"));
    partyMenu.beginParty(view); // the flow caches rows in the same update
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(1),
                  F("party page resolves one living row"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("party page open is a transition"));
    for (uint8_t frame = 0; frame < 3; ++frame) {
        partyMenu.update(0);
        partyMenu.draw();
    }
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(1),
                  F("steady party frames and draws read nothing"));
    test.expectEq(FxReadCounter::renderExact(0), true,
                  F("party render reads no metadata"));
    FxReadCounter::resetFrame();
    partyMenu.update(kEdgeB);
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(0),
                  F("cancelling the party page reads nothing"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("party cancel is steady"));

    view.party[0][1] = {5, 0, 0};
    view.party[0][2] = {255, 20, 1};
    partyMenu.update(kEdgeA);
    FxReadCounter::resetFrame();
    partyMenu.beginParty(view);
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(0),
                  F("dead and out-of-range rows resolve nothing"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("empty party transition stays steady"));
    // The world suite owns the live pointer; this suite wires a local bag.
    item::setBattleInventory(nullptr);
}

} // namespace battle_read_test_detail

void setup() {
    fxTestSetup();
    FxTest test;
    test_items(test);
    items_read_test_detail::nameTable(test);
    items_read_test_detail::bagPage(test);
    items_read_test_detail::healPage(test);
    battle_read_test_detail::pages(test);
    test.report(F("test_items"));
}
void loop() { exit(0); }
