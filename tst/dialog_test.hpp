#pragma once

#include "test.hpp"
#include "../src/engine/menu/DialogMenu.hpp"

inline PopUpDialog dialogFixture(uint24_t text, uint16_t damage = 0) {
    return PopUpDialog{1, 2, 3, 4, text, damage, DAMAGE, 0};
}

void DialogTest(TestSuite &suite) {
    Test test = Test(__func__);
    DialogMenu dialog;
    dialog.clear();
    test.assert(dialog.peek(), false, "empty after clear");
    test.assert(dialog.push(dialogFixture(11)), true, "first push accepted");
    test.assert(dialog.push(dialogFixture(22)), true, "second push accepted");
    test.assert(dialog.push(dialogFixture(33)), true, "third push accepted");
    test.assert(dialog.head().textAddress, static_cast<uint24_t>(11), "head is oldest");
    dialog.popMenu();
    test.assert(dialog.head().textAddress, static_cast<uint24_t>(22), "pop reveals next dialog");
    dialog.popMenu();
    test.assert(dialog.head().textAddress, static_cast<uint24_t>(33), "second pop reveals third dialog");
    dialog.popMenu();
    test.assert(dialog.peek(), false, "empty after final pop");

    for (uint8_t i = 0; i < 6; ++i) {
        test.assert(dialog.push(dialogFixture(i + 1)), true, "push within capacity");
    }
    test.assert(dialog.push(dialogFixture(99)), false, "seventh push rejected");
    test.assert(dialog.head().textAddress, static_cast<uint24_t>(1), "rejected push preserves head");

    Event event;
    event.textAddress = 0x12345;
    dialog.clear();
    test.assert(dialog.peek(), false, "clear resets queue");
    test.assert(dialog.push(dialogFixture(7, 42)), true, "push after clear");
    // Clear the populated item and verify all stale payload fields are erased.
    dialog.clear();
    test.assert(dialog.popDialogStack[0].x, static_cast<uint8_t>(0), "clear erases x");
    test.assert(dialog.popDialogStack[0].y, static_cast<uint8_t>(0), "clear erases y");
    test.assert(dialog.popDialogStack[0].width, static_cast<uint8_t>(0), "clear erases width");
    test.assert(dialog.popDialogStack[0].height, static_cast<uint8_t>(0), "clear erases height");
    test.assert(dialog.popDialogStack[0].textAddress, static_cast<uint24_t>(0), "clear erases text address");
    test.assert(dialog.popDialogStack[0].damage, static_cast<uint16_t>(0), "clear erases damage");
    test.assert(dialog.popDialogStack[0].type, TEXT, "clear erases dialog type");
    test.assert(dialog.popDialogStack[0].animation, static_cast<uint24_t>(0), "clear erases animation");
    dialog.pushEvent(event);
    test.assert(dialog.head().type, TEXT, "event is TEXT");
    test.assert(dialog.head().textAddress, event.textAddress, "event text address");
    test.assert(dialog.head().damage, static_cast<uint16_t>(0), "event damage is zero");
    test.assert(dialog.head().y, static_cast<uint8_t>(34), "event position retained");
    suite.addTest(test);
}

void DialogSuite(TestRunner &runner) {
    TestSuite suite = TestSuite("Dialog Suite");
    DialogTest(suite);
    runner.addTestSuite(suite);
}
