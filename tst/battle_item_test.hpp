#pragma once

#include "test.hpp"
#include "src/FXDataFake.hpp"
#include "../src/engine/battle/Resolve.hpp"
#include "../src/item/ConsumableDef.hpp"
#include "../src/item/Inventory.hpp"
#include "../src/fxdata.h"

namespace battle_item_test_detail {

inline battle::BattleState stateFixture()
{
    battle::BattleState state = {};
    for (uint8_t side = 0; side < 2; ++side) {
        state.active[side].id = static_cast<uint8_t>(side + 1);
        state.active[side].types = DualType(Type::SPIRIT, Type::NONE);
        state.active[side].level = 1;
        state.active[side].hp = state.active[side].maxHp = 100;
        state.active[side].stats.hp = 100;
        state.active[side].stats.attack = 40;
        state.active[side].stats.defense = 20;
        state.active[side].stats.speed = 10;
        state.active[side].moveIds[0] = 7;
        state.active[side].moves[0] = Move(MoveBitSet{
            static_cast<uint8_t>(Type::SPIRIT), 10, 1, 0, 0
        });
        for (uint8_t slot = 1; slot < 4; ++slot) {
            state.active[side].moveIds[slot] = 255;
        }
        state.partyCount[side] = 1;
        state.activeSlot[side] = 0;
    }
    return state;
}

inline uint8_t factCount(const battle::ActionResult &result)
{
    uint8_t count = 0;
    while (count < 4 && result.consequences[count].effect != Effect::NONE) {
        ++count;
    }
    return count;
}

} // namespace battle_item_test_detail

inline void BattleItemIntegrationTest(TestSuite &suite)
{
    using namespace battle;
    using namespace battle_item_test_detail;
    Test test(__func__);
    Rng rng = {nullptr};
    ActionResult result;
    item::Inventory inventory;
    item::inventoryClear(inventory);
    item::setBattleInventory(&inventory);
    fxDataFake::dataBase = consumable_table;
    fxDataFake::dataBytes[0] = static_cast<uint8_t>(item::ConsumableKind::Heal);
    fxDataFake::dataBytes[1] = 16;
    fxDataFake::dataBytes[2] = static_cast<uint8_t>(item::ConsumableKind::Cure);
    fxDataFake::dataBytes[3] = 0;
    fxDataFake::dataBytes[4] = static_cast<uint8_t>(item::ConsumableKind::Charge);
    fxDataFake::dataBytes[5] = 0;

    battle::BattleState state = stateFixture();
    state.active[static_cast<uint8_t>(Side::Player)].hp = 80;
    item::inventoryAdd(inventory, item::ItemKind::Consumable, 0, 1);
    fxDataFake::readCount = 0;
    resolveAction(state, Side::Player, {ActionKind::UseItem, 0}, rng, result);
    test.assert(result.kind, ResultKind::UseItem,
                "UseItem emits its own one-action result");
    test.assert(state.active[static_cast<uint8_t>(Side::Player)].hp, 96,
                "Heal raises acting HP by its definition amount");
    test.assert(item::inventoryCount(inventory, item::ItemKind::Consumable, 0), 0,
                "successful Heal consumes one item");
    test.assert(factCount(result), static_cast<uint8_t>(1),
                "successful Heal emits one fact");
    test.assert(result.consequences[0].effect, Effect::INFSED,
                "Heal fact uses existing HP-increase payload");
    test.assert(result.consequences[0].side, static_cast<uint8_t>(Side::Player),
                "Heal fact names acting side");
    test.assert(result.consequences[0].value, static_cast<uint8_t>(16),
                "Heal fact carries amount actually restored");
    test.assert(fxDataFake::readCount, static_cast<uint32_t>(1),
                "successful UseItem reads definition once after take");

    state = stateFixture();
    item::inventoryAdd(inventory, item::ItemKind::Consumable, 0, 1);
    state.active[static_cast<uint8_t>(Side::Player)].hp = 95;
    fxDataFake::readCount = 0;
    resolveAction(state, Side::Player, {ActionKind::UseItem, 0}, rng, result);
    test.assert(state.active[static_cast<uint8_t>(Side::Player)].hp, 100,
                "Heal clamps at max HP");
    test.assert(result.consequences[0].value, static_cast<uint8_t>(5),
                "clamped Heal reports actual restored amount");
    test.assert(item::inventoryCount(inventory, item::ItemKind::Consumable, 0), 0,
                "clamped Heal still consumes one item");

    state = stateFixture();
    state.active[static_cast<uint8_t>(Side::Player)].hp = 100;
    item::inventoryAdd(inventory, item::ItemKind::Consumable, 0, 1);
    resolveAction(state, Side::Player, {ActionKind::UseItem, 0}, rng, result);
    test.assert(result.consequences[0].value, static_cast<uint8_t>(0),
                "full-HP Heal reports zero restored");
    test.assert(item::inventoryCount(inventory, item::ItemKind::Consumable, 0), 0,
                "full-HP Heal still consumes one item");

    state = stateFixture();
    fxDataFake::readCount = 0;
    resolveAction(state, Side::Player, {ActionKind::UseItem, 0}, rng, result);
    test.assert(result.kind, ResultKind::Skip,
                "empty UseItem is a refused Skip");
    test.assert((result.flags & REFUSED) != 0, true,
                "empty UseItem marks refusal");
    test.assert(fxDataFake::readCount, static_cast<uint32_t>(0),
                "empty UseItem reads no definition after failed take");

    state = stateFixture();
    item::inventoryAdd(inventory, item::ItemKind::Consumable, 0, 1);
    resolveAction(state, Side::Player, {ActionKind::UseItem, 1}, rng, result);
    test.assert(result.kind, ResultKind::Skip,
                "missing consumable stack refuses before definition read");
    test.assert(item::inventoryCount(inventory, item::ItemKind::Consumable, 0), 1,
                "failed item id leaves unrelated stack untouched");

    state = stateFixture();
    item::inventoryAdd(inventory, item::ItemKind::Consumable, 1, 1);
    resolveAction(state, Side::Player, {ActionKind::UseItem, 1}, rng, result);
    test.assert(result.kind, ResultKind::UseItem,
                "Cure consumes as a resolved no-op action");
    test.assert(factCount(result), static_cast<uint8_t>(0),
                "Cure emits no fact in this milestone");
    test.assert(item::inventoryCount(inventory, item::ItemKind::Consumable, 1), 0,
                "Cure consumes one item");

    state = stateFixture();
    item::inventoryAdd(inventory, item::ItemKind::Consumable, 2, 1);
    resolveAction(state, Side::Player, {ActionKind::UseItem, 2}, rng, result);
    test.assert(result.kind, ResultKind::UseItem,
                "Charge consumes as a resolved no-op action");
    test.assert(factCount(result), static_cast<uint8_t>(0),
                "Charge emits no fact in this milestone");
    test.assert(item::inventoryCount(inventory, item::ItemKind::Consumable, 2), 0,
                "Charge consumes one item");

    state = stateFixture();
    TurnPlan plan = {};
    plan.action[static_cast<uint8_t>(Side::Player)] = {ActionKind::UseItem, 0};
    plan.action[static_cast<uint8_t>(Side::Opponent)] = {ActionKind::Attack, 0};
    state.active[static_cast<uint8_t>(Side::Opponent)].stats.speed = 255;
    test.assert(firstMover(state, plan), Side::Player,
                "UseItem fast priority beats opponent attack speed");

    item::setBattleInventory(nullptr);
    suite.addTest(test);
}

inline void BattleItemSuite(TestRunner &runner)
{
    TestSuite suite("Battle item integration");
    BattleItemIntegrationTest(suite);
    runner.addTestSuite(suite);
}
