#define ABG_IMPLEMENTATION
#define SPRITESU_IMPLEMENTATION
#include "src/common.hpp"

// Minimum linker-required globals; game-state, menu, world, and battle globals
// are intentionally absent. DialogMenu.cpp is linked with the copied src tree.
decltype(arduboy) arduboy;
#include "src/engine/menu/DialogMenu.hpp"
DialogMenu dialogMenu;

#include "rawread_test.hpp"

void setup() {
    Serial.begin(9600);
    arduboy.begin();

    FX::begin(FX_DATA_PAGE);
    FX::setCursorRange(rawReadCursorFirstPage, rawReadCursorLastPage);

    FxTest test;
    test_rawread(test);
    test.report(F("test_rawread"));
}

void loop() {
    exit(0);
}
