#pragma once

#include "test.hpp"
#include "../src/lib/ListView.hpp"

void ListViewTest(TestSuite &suite) {
    Test test = Test(__func__);

    ListView empty = {0, 4, 0, 0};
    listViewMove(empty, 1);
    listViewMove(empty, -1);
    test.assert(listViewVisible(empty, 0), false, "empty list has no visible rows");
    test.assert(listViewItemAt(empty, 0), static_cast<uint8_t>(0), "empty list lookup is safe");

    ListView one = {1, 4, 0, 0};
    listViewMove(one, 1);
    test.assert(one.cursor, static_cast<uint8_t>(0), "single item cursor remains at zero");
    test.assert(one.windowStart, static_cast<uint8_t>(0), "oversized window starts at zero");
    test.assert(listViewVisible(one, 0), true, "single item is visible");
    test.assert(listViewVisible(one, 1), false, "rows beyond item count are hidden");
    test.assert(listViewItemAt(one, 0), static_cast<uint8_t>(0), "first row resolves to item zero");

    ListView down = {6, 3, 0, 0};
    listViewMove(down, 3);
    test.assert(down.cursor, static_cast<uint8_t>(3), "cursor advances through visible rows");
    test.assert(down.windowStart, static_cast<uint8_t>(1), "window follows at lower edge");
    listViewMove(down, 20);
    test.assert(down.cursor, static_cast<uint8_t>(5), "movement clamps to final item");
    test.assert(down.windowStart, static_cast<uint8_t>(3), "window clamps at final range");
    test.assert(listViewItemAt(down, 2), static_cast<uint8_t>(5), "last visible row is final item");
    test.assert(listViewVisible(down, 3), false, "row after window is hidden");

    listViewMove(down, -20);
    test.assert(down.cursor, static_cast<uint8_t>(0), "movement clamps to first item");
    test.assert(down.windowStart, static_cast<uint8_t>(0), "window returns to first item");
    listViewMove(down, -1);
    test.assert(down.cursor, static_cast<uint8_t>(0), "negative movement at start does not underflow");

    ListView traversal = {32, 4, 0, 0};
    for (uint8_t item = 0; item < traversal.itemCount; ++item) {
        test.assert(traversal.cursor, item, "traversal cursor visits every item");
        test.assert(traversal.windowStart <= traversal.cursor, true, "window starts before cursor");
        test.assert(static_cast<uint16_t>(traversal.cursor) <
                        static_cast<uint16_t>(traversal.windowStart) + traversal.rows,
                    true, "cursor remains inside window");
        test.assert(traversal.windowStart <= traversal.itemCount - traversal.rows, true,
                    "window stays within final range");
        if (item + 1 < traversal.itemCount) listViewMove(traversal, 1);
    }
    test.assert(traversal.windowStart, static_cast<uint8_t>(28), "final window ends at item 31");

    suite.addTest(test);
}

void ListViewSuite(TestRunner &runner) {
    TestSuite suite = TestSuite("ListView Suite");
    ListViewTest(suite);
    runner.addTestSuite(suite);
}
