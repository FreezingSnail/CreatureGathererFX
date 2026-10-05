#pragma once

#include "fxtest.hpp"
#include "src/engine/menu/DialogMenu.hpp"

inline void test_dialogs(FxTest &test) {
    dialogMenu.clear();

    const PopUpDialog invalidScriptText{0, 43, 128, 24, 0xFFFF, 0, 0,
                                        SCRIPT_TEXT, 0};
    test.expectEq(dialogMenu.push(invalidScriptText), true,
                  F("script text dialog queued"));
    test.expectEq(dialogMenu.head().type, SCRIPT_TEXT,
                  F("script text type retained"));
    test.expectEq(dialogMenu.head().width, 0,
                  F("invalid script text resolves to empty"));

    // The current generated map has no script text entries; scripts_test
    // exercises real Msg dispatch when data provides one.
    dialogMenu.drawPopMenu();
    test.expectEq(dialogMenu.peek(), true,
                  F("drawing leaves queued dialog in place"));
    test.expectEq(dialogMenu.head().textAddress, static_cast<uint24_t>(0xFFFF),
                  F("drawing keeps the prepared head"));

    dialogMenu.clear();
    test.expectEq(dialogMenu.peek(), false, F("clear empties world dialog queue"));
}
