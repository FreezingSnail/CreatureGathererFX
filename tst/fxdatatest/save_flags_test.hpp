#pragma once

#include "fxtest.hpp"
#include "src/GameState.hpp"
#include "src/globals.hpp"
#include "src/flags/flag_bit_array.hpp"
#include "src/save/Compaction.hpp"
#include "src/save/SaveController.hpp"
#include "src/save/SaveFile.hpp"

inline void test_save_flags(FxTest &test)
{
    FX::eraseSaveBlock(32);
    FX::waitWhileBusy();
    FX::eraseSaveBlock(48);
    FX::waitWhileBusy();
    storeFresh();
    uint8_t flagsAndNeighbor[2] = {0x5a, 0xc7};
    uint8_t *originalFlags = gameState.flags;
    gameState.flags = flagsAndNeighbor;
    Arduboy2Base::sBuffer[0] = 0xa5;
    SaveController::begin(GameState_t::WORLD);
    for (uint16_t i = 0; i < 256 && saveInProgress(); ++i) {
        SaveController::advance();
        FX::waitWhileBusy();
    }

    SaveFile snapshot = {};
    test.expectEq(saveFileLoad(snapshot), true, F("snapshot commits with framebuffer byte set"));
    test.expectEq(snapshot.flags[0], 0x5a, F("generated flag byte is captured"));
    for (uint8_t i = sizeof(FLAG_BIT_ARRAY); i < sizeof(snapshot.flags); ++i)
        test.expectEq(snapshot.flags[i], 0, F("unused save flag bytes stay zero"));
    test.expectEq(Arduboy2Base::sBuffer[0], 0xa5, F("save preserves framebuffer byte zero"));

    flagsAndNeighbor[0] = 0;
    Arduboy2Base::sBuffer[0] = 0x3c;
    test.expectEq(SaveController::load(), true, F("shipping save loads"));
    test.expectEq(flagsAndNeighbor[0], 0x5a, F("generated flag byte is restored"));
    test.expectEq(flagsAndNeighbor[1], 0xc7, F("neighbor byte is unchanged by restore"));
    test.expectEq(Arduboy2Base::sBuffer[0], 0x3c, F("load preserves framebuffer byte zero"));
    gameState.flags = originalFlags;

    FLAG_BIT_ARRAY[0] = 0x69;
    Arduboy2Base::sBuffer[0] = 0xa5;
    SaveController::begin(GameState_t::WORLD);
    for (uint16_t i = 0; i < 256 && saveInProgress(); ++i) {
        SaveController::advance();
        FX::waitWhileBusy();
    }
    test.expectEq(saveFileLoad(snapshot), true, F("generated flag pointer snapshot commits"));
    test.expectEq(snapshot.flags[0], 0x69, F("generated flag pointer is captured"));
    for (uint8_t i = sizeof(FLAG_BIT_ARRAY); i < sizeof(snapshot.flags); ++i)
        test.expectEq(snapshot.flags[i], 0, F("generated pointer leaves padding zero"));
    test.expectEq(Arduboy2Base::sBuffer[0], 0xa5, F("generated flag save preserves framebuffer"));

    FLAG_BIT_ARRAY[0] = 0;
    Arduboy2Base::sBuffer[0] = 0x3c;
    test.expectEq(SaveController::load(), true, F("generated flag pointer save loads"));
    test.expectEq(FLAG_BIT_ARRAY[0], 0x69, F("real generated flag byte round-trips"));
    test.expectEq(Arduboy2Base::sBuffer[0], 0x3c, F("generated flag load preserves framebuffer"));
}
