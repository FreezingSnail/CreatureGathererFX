#pragma once

#include "test.hpp"
#include "src/FXDataFake.hpp"
#include "../src/engine/battle/Resolve.hpp"
#include "../src/item/Inventory.hpp"

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

} // namespace battle_item_test_detail

inline void BattleItemIntegrationTest(TestSuite &suite)
{
    using namespace battle;
    using namespace battle_item_test_detail;
    Test test(__func__);
    static_assert(static_cast<uint8_t>(ActionKind::Attack) == 0 &&
                  static_cast<uint8_t>(ActionKind::Switch) == 1 &&
                  static_cast<uint8_t>(ActionKind::Gather) == 2 &&
                  static_cast<uint8_t>(ActionKind::Escape) == 3 &&
                  static_cast<uint8_t>(ActionKind::Skip) == 4 &&
                  static_cast<uint8_t>(ActionKind::UseItem) == 5,
                  "legacy action kind values remain stable");
    Rng rng = {nullptr};
    ActionResult result;
    item::Inventory inventory;
    item::inventoryClear(inventory);
    item::setBattleInventory(&inventory);

    battle::BattleState state = stateFixture();
    state.active[static_cast<uint8_t>(Side::Player)].hp = 80;
    item::inventoryAdd(inventory, item::ItemKind::Consumable, 0, 1);
    fxDataFake::readCount = 0;
    resolveAction(state, Side::Player, {ActionKind::UseItem, 0}, rng, result);
    test.assert(result.kind, ResultKind::Skip,
                "legacy battle item intent is refused as a Skip");
    test.assert((result.flags & REFUSED) != 0, true,
                "removed battle item route marks refusal");
    test.assert(state.active[static_cast<uint8_t>(Side::Player)].hp, 80,
                "refused item intent leaves battle HP unchanged");
    test.assert(item::inventoryCount(inventory, item::ItemKind::Consumable, 0), 1,
                "refused item intent preserves world inventory");
    test.assert(fxDataFake::readCount, static_cast<uint32_t>(0),
                "refused item intent reads no consumable definition");

    state = stateFixture();
    TurnPlan plan = {};
    plan.action[static_cast<uint8_t>(Side::Player)] = {ActionKind::UseItem, 0};
    plan.action[static_cast<uint8_t>(Side::Opponent)] = {ActionKind::Attack, 0};
    state.active[static_cast<uint8_t>(Side::Opponent)].stats.speed = 255;
    test.assert(firstMover(state, plan), Side::Opponent,
                "legacy item value has no battle action priority");

    item::setBattleInventory(nullptr);
    suite.addTest(test);
}

inline void BattleItemSuite(TestRunner &runner)
{
    TestSuite suite("Battle item integration");
    BattleItemIntegrationTest(suite);
    runner.addTestSuite(suite);
}
