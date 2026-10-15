#pragma once
// Permanent host proof for the duel compact pick row (CreatureGathererFX-d73i.2).

#include "test.hpp"
#include "../src/engine/battle/DuelPickRow.hpp"

void DuelPickRowSuite(TestRunner &runner)
{
    TestSuite suite("Duel pick row");
    Test test(__func__);

    // Four attacks + living bench: four option cells then SWAP.
    duel::PickRow row;
    duel::pickRowInit(row, 4, true);
    test.assert(row.cells, static_cast<uint8_t>(5), "four attacks + swap");
    test.assert(row.options, static_cast<uint8_t>(4), "all four attack cells kept");
    test.assert(row.swap, static_cast<uint8_t>(1), "swap cell offered with bench");
    duel::pickRowMove(row, 1);
    duel::pickRowMove(row, 1);
    duel::pickRowMove(row, 1);
    duel::pickRowMove(row, 1);
    test.assert(duel::pickRowIsSwap(row), true, "cursor reaches the swap cell");

    // One attack + bench.
    duel::pickRowInit(row, 1, true);
    test.assert(row.cells, static_cast<uint8_t>(2), "one attack + swap");
    duel::pickRowMove(row, 1);
    test.assert(duel::pickRowIsSwap(row), true, "swap after the single attack");

    // Four attacks, no living bench: SWAP suppressed.
    duel::pickRowInit(row, 4, false);
    test.assert(row.cells, static_cast<uint8_t>(4), "no bench suppresses swap");
    test.assert(row.swap, static_cast<uint8_t>(0), "swap suppressed");
    duel::pickRowMove(row, 1);
    duel::pickRowMove(row, 1);
    duel::pickRowMove(row, 1);
    duel::pickRowMove(row, 1);
    test.assert(row.cursor, static_cast<uint8_t>(0), "cursor wraps without swap");

    // Zero attacks + living bench: SWAP and WAIT both present.
    duel::pickRowInit(row, 0, true);
    test.assert(row.cells, static_cast<uint8_t>(2), "unusable kit: swap + wait");
    test.assert(row.wait, static_cast<uint8_t>(1), "wait cell present");
    test.assert(duel::pickRowIsSwap(row), true, "unusable kit still offers swap");
    duel::pickRowMove(row, 1);
    test.assert(duel::pickRowIsWait(row), true, "wait cell reachable");
    test.assert(duel::pickRowIsAttack(row), false, "no attack cells for empty kit");

    // Zero attacks, no bench: WAIT only.
    duel::pickRowInit(row, 0, false);
    test.assert(row.cells, static_cast<uint8_t>(1), "empty kit no bench: wait only");
    test.assert(duel::pickRowIsWait(row), true, "wait is the only cell");
    test.assert(duel::pickRowIsSwap(row), false, "no swap without bench");

    // Cursor wrap both directions.
    duel::pickRowInit(row, 2, true);
    duel::pickRowMove(row, -1);
    test.assert(duel::pickRowIsSwap(row), true, "wrap to the last cell");
    duel::pickRowMove(row, 1);
    test.assert(row.cursor, static_cast<uint8_t>(0), "wrap forward to the first cell");

    // Zero-cell guard never yields an invalid index.
    duel::pickRowInit(row, 0, false);
    duel::pickRowMove(row, 1);
    test.assert(row.cursor, static_cast<uint8_t>(0), "single cell stays at index 0");

    suite.addTest(test);
    runner.addTestSuite(suite);
}
