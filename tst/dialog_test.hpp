#pragma once

#include "test.hpp"
#include "../src/engine/menu/DialogMenu.hpp"
#include "../src/globals.hpp"

inline PopUpDialog dialogFixture(uint24_t text, uint16_t damage = 0) {
    return PopUpDialog{1, 2, 3, 4, text, 0, damage, DAMAGE, 0};
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
    test.assert(dialog.popDialogStack[0].detailAddress, static_cast<uint24_t>(0), "clear erases detail address");
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

void DialogAddressTest(TestSuite &suite) {
    Test test = Test(__func__);
    DialogMenu dialog;
    constexpr uint8_t creatureId = 0;
    constexpr uint16_t moveId = 1;
    const uint24_t nameAddress = readCreatureNameAddress(creatureId);
    const uint24_t moveAddress = readMoveNameAddress(moveId);
    const uint24_t effectAddress = readEffectStringAddress();

    test.assert(dialog.push(newDialogBox(NAME, creatureId, moveId)), true, "name pushed");
    test.assert(dialog.head().textAddress, nameAddress, "name address resolved");
    test.assert(dialog.head().detailAddress, moveAddress, "move address resolved");
    test.assert(dialog.head().width, static_cast<uint8_t>(70), "name bitmap width resolved");
    test.assert(dialog.head().height, static_cast<uint8_t>(40), "move bitmap width resolved");
    test.assert(dialog.head().textAddress != 0, true, "name address nonzero");
    test.assert(dialog.head().detailAddress != 0, true, "move address nonzero");
    dialog.drawPopMenu();
    test.assert(dialog.dialogCount, static_cast<uint8_t>(1), "drawing keeps count");
    test.assert(dialog.head().textAddress, nameAddress, "drawing keeps head address");
    test.assert(dialog.head().detailAddress, moveAddress, "drawing keeps detail address");

    dialog.popMenu();
    constexpr uint16_t emptyMoveId = 32;
    test.assert(dialog.push(newDialogBox(NAME, creatureId, emptyMoveId)), true, "empty move pushed");
    test.assert(dialog.head().height, static_cast<uint8_t>(0), "empty move has no bitmap width");
    dialog.popMenu();

    test.assert(dialog.push(newDialogBox(NAME, creatureId, 0)), true, "name without move pushed");
    test.assert(dialog.head().detailAddress, static_cast<uint24_t>(0), "zero move remains unresolved");
    test.assert(dialog.head().height, static_cast<uint8_t>(0), "zero move has no bitmap width");
    dialog.popMenu();

    test.assert(dialog.push(newDialogBox(FAINT, creatureId, 0)), true, "faint pushed");
    test.assert(dialog.head().textAddress, nameAddress, "faint name resolved");
    test.assert(dialog.head().width, static_cast<uint8_t>(70), "faint bitmap width resolved");
    test.assert(dialog.head().detailAddress, static_cast<uint24_t>(0), "faint has no detail");
    dialog.popMenu();

    constexpr uint8_t otherCreatureId = 3;
    test.assert(dialog.push(newDialogBox(PLAYER_EFFECT, otherCreatureId, 0)), true, "effect pushed");
    test.assert(dialog.head().textAddress, readCreatureNameAddress(otherCreatureId), "effect creature name resolved");
    test.assert(dialog.head().width, static_cast<uint8_t>(60), "effect creature bitmap width resolved");
    test.assert(dialog.head().detailAddress, effectAddress, "effect string resolved");
    test.assert(dialog.head().height, static_cast<uint8_t>(75), "effect bitmap width resolved");
    test.assert(dialog.head().detailAddress != 0, true, "effect string nonzero");
    dialog.drawPopMenu();
    test.assert(dialog.dialogCount, static_cast<uint8_t>(1), "drawing effect keeps count");
    test.assert(dialog.head().detailAddress, effectAddress, "drawing effect keeps detail");

    dialog.popMenu();
    test.assert(dialog.push(newDialogBox(ENEMY_EFFECT, otherCreatureId, 0)), true, "enemy effect pushed");
    test.assert(dialog.head().textAddress, readCreatureNameAddress(otherCreatureId),
                "enemy effect creature name resolved");
    test.assert(dialog.head().detailAddress, effectAddress, "enemy effect uses sole effect string");
    test.assert(dialog.head().width, static_cast<uint8_t>(60), "enemy effect creature width resolved");
    test.assert(dialog.head().height, static_cast<uint8_t>(75), "enemy effect string width resolved");

    suite.addTest(test);
}

void DialogSuite(TestRunner &runner) {
    TestSuite suite = TestSuite("Dialog Suite");
    DialogTest(suite);
    DialogAddressTest(suite);
    runner.addTestSuite(suite);
}
