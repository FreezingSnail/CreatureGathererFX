#pragma once

#include "test.hpp"
#include "../src/engine/menu/DialogMenu.hpp"
#include "../src/globals.hpp"

inline PopUpDialog dialogFixture(uint24_t text, DialogType type = SCRIPT_TEXT) {
    return PopUpDialog{0, 43, 128, 24, text, type};
}

void DialogTest(TestSuite &suite) {
    Test test = Test(__func__);
    DialogMenu dialog;
    dialog.clear();
    test.assert(dialog.peek(), false, "empty after clear");
    test.assert(dialog.push(dialogFixture(11)), true, "first push accepted");
    test.assert(dialog.push(dialogFixture(22)), true, "second push accepted");
    test.assert(dialog.push(dialogFixture(33)), true, "third push accepted");
    test.assert(dialog.head().type, SCRIPT_TEXT, "script text retained");
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
    dialog.pushEvent(event);
    test.assert(dialog.head().type, TEXT, "event is TEXT");
    test.assert(dialog.head().textAddress, event.textAddress, "event text address");
    test.assert(dialog.head().x, static_cast<uint8_t>(0), "event origin x");
    test.assert(dialog.head().y, static_cast<uint8_t>(34), "event origin y");
    test.assert(dialog.head().width, static_cast<uint8_t>(120), "event width");
    test.assert(dialog.head().height, static_cast<uint8_t>(30), "event height");

    dialog.clear();
    test.assert(dialog.peek(), false, "clear empties queue");
    test.assert(dialog.popDialogStack[0].textAddress, static_cast<uint24_t>(0), "clear erases payload");
    test.assert(dialog.popDialogStack[0].type, TEXT, "clear resets type");
    suite.addTest(test);
}

void DialogSuite(TestRunner &runner) {
    TestSuite suite = TestSuite("Dialog Suite");
    DialogTest(suite);
    runner.addTestSuite(suite);
}
