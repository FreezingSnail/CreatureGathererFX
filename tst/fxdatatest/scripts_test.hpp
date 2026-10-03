#pragma once

#include <string.h>

#include "fxtest.hpp"
#include "src/fxdata.h"
#include "src/engine/world/Chunk.hpp"
#include "src/engine/menu/DialogMenu.hpp"
#include "src/flags/flag_bit_array.hpp"
#include "src/vm/ScriptVM.hpp"
#include "src/vm/opcodes.hpp"

namespace scripts_test_detail {

inline uint16_t readScriptTextCount() {
    uint8_t bytes[2];
    FX::readDataBytes(raw_map_text, bytes, sizeof(bytes));
    return static_cast<uint16_t>(bytes[0]) |
           (static_cast<uint16_t>(bytes[1]) << 8);
}

inline void expectRealScriptSlot(FxTest &test, uint8_t *slot) {
    static const uint8_t expected[] PROGMEM = {
        5, 0, 0, 0, 1, 9, 4, 0, 1, 0, 1, 0, 0, 0, 0, 255,
    };
    const uint24_t address = Chunk::scriptSlotAddr(scripts, 0);
    uint8_t addressBytes[sizeof(address)];
    memcpy(addressBytes, &address, sizeof(address));

    test.expectEq(address == scripts, true, F("chunk zero uses scripts base"));
    test.expectEq(addressBytes[sizeof(address) - 1] != 0, true,
                  F("real script slot address is above 0x010000"));

    // This is one raw 128-byte FX read, the same slot-sized operation used by
    // interact(). The remaining bytes are padding; compare only the encoded
    // nonempty script and its End sentinel.
    FX::readDataBytes(address, slot, Chunk::SCRIPT_SLOT_BYTES);
    for (uint8_t index = 0; index < sizeof(expected); ++index) {
        test.expectEqIdx(slot[index], pgm_read_byte(expected + index),
                         F("chunk zero generated script byte"), index);
    }

    test.expectEq(slot[0], static_cast<uint8_t>(VmOpcode::If),
                  F("real script begins with If"));
    test.expectEq(slot[6], static_cast<uint8_t>(VmOpcode::TpIf),
                  F("real script contains TpIf"));
    test.expectEq(slot[7], 0, F("TpIf source x high byte"));
    test.expectEq(slot[8], 1, F("TpIf source x low byte"));
    test.expectEq(slot[9], 0, F("TpIf source y high byte"));
    test.expectEq(slot[10], 1, F("TpIf source y low byte"));
    test.expectEq(slot[11], 0, F("TpIf target x high byte"));
    test.expectEq(slot[12], 0, F("TpIf target x low byte"));
    test.expectEq(slot[13], 0, F("TpIf target y high byte"));
    test.expectEq(slot[14], 0, F("TpIf target y low byte"));
    test.expectEq(slot[15], static_cast<uint8_t>(VmOpcode::End),
                  F("real script terminates"));
}

inline void testRealScriptExecution(FxTest &test, ScriptVm &vm, uint8_t *slot) {
    FLAG_BIT_ARRAY[0] = 0;
    gameState.playerLocation = To1D(1, 1);
    vm.initVM(slot, To1D(1, 1), To1D(1, 2));
    vm.run();
    test.expectEq(gameState.playerLocation, To1D(1, 1),
                  F("real If skips TpIf while flag is clear"));

    FLAG_BIT_ARRAY[0] = 1;
    gameState.playerLocation = To1D(1, 1);
    vm.initVM(slot, To1D(1, 1), To1D(1, 2));
    vm.run();
    test.expectEq(gameState.playerLocation, To1D(0, 0),
                  F("real TpIf decodes BE coordinates and teleports"));
    FLAG_BIT_ARRAY[0] = 0;
}

inline void testScriptTextDispatch(FxTest &test, ScriptVm &vm, uint8_t *slot) {
    dialogMenu.clear();
    const uint16_t count = readScriptTextCount();

    // The current generated map has no script text. Host ScriptVM tests cover
    // valid Msg/TMsg/SMsg payloads and coordinate filtering; if later map data
    // adds text, this device suite also exercises a valid Msg and its renderer.
    if (count != 0) {
        slot[0] = static_cast<uint8_t>(VmOpcode::Msg);
        slot[1] = 0;
        slot[2] = 0;
        slot[3] = static_cast<uint8_t>(VmOpcode::End);
        vm.initVM(slot, 0, 0);
        vm.run();
        test.expectEq(dialogMenu.peek(), true, F("valid Msg queues a dialog"));
        if (dialogMenu.peek()) {
            test.expectEq(dialogMenu.head().type, SCRIPT_TEXT,
                          F("Msg uses typed script text dialog"));
            test.expectEq(dialogMenu.head().textAddress == static_cast<uint24_t>(0), true,
                          F("script text dialog stores its index"));
            dialogMenu.drawPopMenu();
        }
        dialogMenu.clear();
    }

    const uint16_t invalidIndex = count;
    slot[0] = static_cast<uint8_t>(VmOpcode::Msg);
    slot[1] = static_cast<uint8_t>(invalidIndex >> 8);
    slot[2] = static_cast<uint8_t>(invalidIndex);
    slot[3] = static_cast<uint8_t>(VmOpcode::End);
    vm.initVM(slot, 0, 0);
    vm.run();
    test.expectEq(dialogMenu.peek(), false,
                  F("Msg rejects an index past the generated text table"));
    dialogMenu.drawPopMenu();
    dialogMenu.clear();
}

} // namespace scripts_test_detail

inline void test_scripts(FxTest &test, ScriptVm &vm, uint8_t *slot) {
    scripts_test_detail::expectRealScriptSlot(test, slot);
    scripts_test_detail::testRealScriptExecution(test, vm, slot);
    scripts_test_detail::testScriptTextDispatch(test, vm, slot);
}
