#include "action_test.hpp"
#include <cstring>

#include "../../src/vm/ScriptVM.hpp"
#include "../../src/vm/opcodes.hpp"
#include "../../src/GameState.hpp"
#include "../../src/flags/flag_bit_array.hpp"
#define TEST
#include "../../src/globals.hpp"

#include <cstdint>

namespace {
uint16_t scriptTextCount = 4;

void clearFlags() {
    for (int i = 0; i < sizeof(FLAG_BIT_ARRAY); i++) {
        FLAG_BIT_ARRAY[i] = 0;
    }
}

void loadSlot(uint8_t (&slot)[128], const uint8_t *script, uint8_t length) {
    std::memset(slot, 0, sizeof(slot));
    std::memcpy(slot, script, length);
    slot[length] = static_cast<uint8_t>(VmOpcode::End);
}

void setAddress(uint8_t *slot, uint8_t &offset, uint16_t value) {
    slot[offset++] = static_cast<uint8_t>(value >> 8);
    slot[offset++] = static_cast<uint8_t>(value);
}
}

uint16_t readScriptTextCountForTest() {
    return scriptTextCount;
}

void tpCommandTest(TestSuite &t) {
    Test test = Test(__func__);
    ScriptVm vm = {};
    gameState = GameState();
    uint8_t slot[128];
    const uint8_t script[] = {
        static_cast<uint8_t>(VmOpcode::TpIf), 0, 0, 0, 1, 0, 0, 0, 2
    };
    loadSlot(slot, script, sizeof(script));

    vm.initVM(slot, 0, 0);
    gameState.playerLocation = 0;
    vm.run();
    test.assert(gameState.playerLocation, 0, "teleported off tile 0 incorrectly");
    gameState.playerLocation = To1D(0, 1);
    vm.run();
    test.assert(gameState.playerLocation, To1D(0, 2), "did not teleport to tile 0,2");

    t.addTest(test);
}

void parsedBlobTest(TestSuite &t) {
    Test test = Test(__func__);
    ScriptVm vm = {};
    gameState = GameState();
    uint8_t slot[128];
    const uint8_t script[] = {
        static_cast<uint8_t>(VmOpcode::TpIf),
        0, 4, 0, 4,   // 4,4
        0, 12, 0, 7   // 12,7
    };
    loadSlot(slot, script, sizeof(script));

    vm.initVM(slot, 0, 0);
    gameState.playerLocation = 0;
    vm.run();
    test.assert(gameState.playerLocation, 0, "did not teleport");
    gameState.playerLocation = To1D(4, 4);
    vm.run();
    test.assert(gameState.playerLocation, To1D(12, 7), "did not teleport");

    t.addTest(test);
}

void parsedBlob2ScriptsTest(TestSuite &t) {
    Test test = Test(__func__);
    ScriptVm vm = {};
    gameState = GameState();
    uint8_t slot[128];
    const uint8_t script[] = {
        static_cast<uint8_t>(VmOpcode::TpIf),
        0, 4, 0, 4,   // 4,4
        0, 12, 0, 7,  // 12,7
        static_cast<uint8_t>(VmOpcode::TpIf),
        0, 12, 0, 8,  // 12,8
        0, 4, 0, 4    // 4,4
    };
    loadSlot(slot, script, sizeof(script));

    vm.initVM(slot, 0, 0);
    gameState.playerLocation = 0;
    vm.run();
    test.assert(gameState.playerLocation, 0, "teleported");
    gameState.playerLocation = To1D(4, 4);
    vm.run();
    test.assert(gameState.playerLocation, To1D(12, 7), "did not teleport to 12, 7");
    gameState.playerLocation = To1D(12, 8);
    vm.run();
    test.assert(gameState.playerLocation, To1D(4, 4), "did not teleport to 4, 4");

    t.addTest(test);
}

void ifThenTpTest(TestSuite &t) {
    Test test = Test(__func__);
    ScriptVm vm = {};
    gameState = GameState();
    uint8_t slot[128];

    // if flag_door then tp 1 1 endif;
    const uint8_t script[] = {
        static_cast<uint8_t>(VmOpcode::If),
        0,             // flag set
        0, 0,           // flag
        1,              // then
        5,              // jump length
        static_cast<uint8_t>(VmOpcode::Tp),
        0, 1, 0, 1      // 1,1
    };
    loadSlot(slot, script, sizeof(script));

    vm.initVM(slot, 0, 0);
    gameState.playerLocation = 0;
    vm.run();
    test.assert(gameState.playerLocation, 0, "teleported");
    gameState.setFlag(0);
    vm.run();
    test.assert(gameState.playerLocation, To1D(1, 1), "did not teleport to 1, 1");

    clearFlags();
    t.addTest(test);
}

void ifThenelseTpTest(TestSuite &t) {
    Test test = Test(__func__);
    ScriptVm vm = {};
    gameState = GameState();
    uint8_t slot[128];

    // if flag_door then tp 1 1 else tp 0 0 endif;
    const uint8_t script[] = {
        static_cast<uint8_t>(VmOpcode::If),
        0,              // flag set
        0, 0,           // flag
        0,              // else
        5,              // jump length
        static_cast<uint8_t>(VmOpcode::Tp),
        0, 1, 0, 1,      // 1,1
        static_cast<uint8_t>(VmOpcode::Tp),
        0, 0, 0, 0       // 0,0
    };
    loadSlot(slot, script, sizeof(script));

    vm.initVM(slot, 0, 0);
    gameState.playerLocation = 128;
    vm.run();
    test.assert(gameState.playerLocation, 0, "did not teleport to 0,0");

    clearFlags();
    t.addTest(test);
}

void flagCommandTest(TestSuite &t) {
    Test test = Test(__func__);
    ScriptVm vm = {};
    uint8_t slot[128];
    gameState = GameState();
    clearFlags();
    const uint8_t setFlag[] = {static_cast<uint8_t>(VmOpcode::SetFlag), 0, 1};
    loadSlot(slot, setFlag, sizeof(setFlag));
    vm.initVM(slot, 0, 0);
    vm.run();
    test.assert(gameState.getFlag(1), true, "SetFlag consumes a big-endian flag id");

    const uint8_t unsetFlag[] = {static_cast<uint8_t>(VmOpcode::UnsetFlag), 0, 1};
    loadSlot(slot, unsetFlag, sizeof(unsetFlag));
    vm.initVM(slot, 0, 0);
    vm.run();
    test.assert(gameState.getFlag(1), false, "UnsetFlag clears its flag");

    const uint8_t readFlag[] = {
        static_cast<uint8_t>(VmOpcode::ReadFlag), 0, 1,
        static_cast<uint8_t>(VmOpcode::Tp), 0, 2, 0, 3
    };
    loadSlot(slot, readFlag, sizeof(readFlag));
    vm.initVM(slot, 0, 0);
    vm.run();
    test.assert(gameState.playerLocation, To1D(2, 3), "ReadFlag consumes its two-byte operand");
    clearFlags();
    t.addTest(test);
}

void scriptBufferIsolationTest(TestSuite &t) {
    Test test = Test(__func__);
    ScriptVm vm = {};
    uint8_t slot[128];
    const uint8_t script[] = {
        static_cast<uint8_t>(VmOpcode::Tp), 0, 7, 0, 9
    };
    loadSlot(slot, script, sizeof(script));
    std::memset(sBuffer, 0xA5, sizeof(sBuffer));

    vm.initVM(slot, To1D(1, 2), To1D(1, 3));
    test.assert(vm.base == slot, true, "VM base is the injected world slot");
    test.assert(vm.ptr == slot, true, "VM cursor starts at slot base");
    test.assert(vm.base != sBuffer, true, "VM does not bind to framebuffer storage");
    vm.run();
    test.assert(gameState.playerLocation, To1D(7, 9), "script executed from injected slot");
    test.assert(vm.ptr == slot, true, "end resets cursor to the slot base");
    test.assert(sBuffer[0], static_cast<uint8_t>(0xA5), "VM leaves framebuffer bytes alone");
    test.assert(sBuffer[511], static_cast<uint8_t>(0xA5), "VM leaves framebuffer scratch area alone");

    t.addTest(test);
}

void scriptMessageTest(TestSuite &t) {
    Test test = Test(__func__);
    ScriptVm vm = {};
    uint8_t slot[128];
    dialogMenu.clear();

    const uint8_t msg[] = {
        static_cast<uint8_t>(VmOpcode::Msg), 0x01, 0x23,
        static_cast<uint8_t>(VmOpcode::Tp), 0, 9, 0, 8
    };
    scriptTextCount = 0x0124;
    loadSlot(slot, msg, sizeof(msg));
    vm.initVM(slot, To1D(3, 4), To1D(3, 5));
    gameState.playerLocation = 0;
    vm.run();
    test.assert(dialogMenu.dialogCount, static_cast<uint8_t>(1), "Msg queues a dialog");
    test.assert(dialogMenu.head().type, SCRIPT_TEXT, "Msg uses typed script-text dialog");
    test.assert(dialogMenu.head().textAddress, static_cast<uint24_t>(0x0123),
                "Msg text index is decoded big endian");
    test.assert(vm.ptr == slot, true, "Msg consumes its two-byte operand then End");
    test.assert(gameState.playerLocation, To1D(9, 8), "Msg advances to the next command");

    dialogMenu.clear();
    scriptTextCount = 4;
    const uint8_t tmsg[] = {
        static_cast<uint8_t>(VmOpcode::TMsg),
        0x00, 0x03, 0x00, 0x05, // coordinate (3,5)
        0x00, 0x02,             // text index 2
        static_cast<uint8_t>(VmOpcode::Tp), 0, 9, 0, 8
    };
    loadSlot(slot, tmsg, sizeof(tmsg));
    vm.initVM(slot, To1D(3, 4), To1D(3, 5));
    gameState.playerLocation = 0;
    vm.run();
    test.assert(dialogMenu.dialogCount, static_cast<uint8_t>(1), "matching TMsg queues");
    test.assert(dialogMenu.head().textAddress, static_cast<uint24_t>(2),
                "TMsg consumes coordinates before text index");
    test.assert(gameState.playerLocation, To1D(9, 8), "TMsg advances to the next command");

    dialogMenu.clear();
    vm.initVM(slot, To1D(3, 4), To1D(4, 5));
    gameState.playerLocation = 0;
    vm.run();
    test.assert(dialogMenu.dialogCount, static_cast<uint8_t>(0), "mismatching TMsg is filtered");
    test.assert(gameState.playerLocation, To1D(9, 8), "mismatching TMsg still consumes operands");

    const uint8_t smsg[] = {
        static_cast<uint8_t>(VmOpcode::SMsg),
        0x00, 0x03, 0x00, 0x04, // coordinate (3,4)
        0x00, 0x03,             // text index 3
        static_cast<uint8_t>(VmOpcode::Tp), 0, 9, 0, 8
    };
    loadSlot(slot, smsg, sizeof(smsg));
    vm.initVM(slot, To1D(3, 4), To1D(3, 5));
    gameState.playerLocation = 0;
    vm.run();
    test.assert(dialogMenu.dialogCount, static_cast<uint8_t>(1), "matching SMsg queues");
    test.assert(dialogMenu.head().textAddress, static_cast<uint24_t>(3),
                "SMsg reads its big-endian text index");
    test.assert(gameState.playerLocation, To1D(9, 8), "SMsg advances to the next command");
    dialogMenu.clear();

    // The count comes from a little-endian table header; index == count is invalid.
    scriptTextCount = 3;
    const uint8_t invalid[] = {static_cast<uint8_t>(VmOpcode::Msg), 0x00, 0x03};
    loadSlot(slot, invalid, sizeof(invalid));
    vm.initVM(slot, 0, 0);
    vm.run();
    test.assert(dialogMenu.dialogCount, static_cast<uint8_t>(0), "out-of-range text index rejected");

    scriptTextCount = 4;
    t.addTest(test);
}

void scriptBoundsTest(TestSuite &t) {
    Test test = Test(__func__);
    ScriptVm vm = {};
    uint8_t slot[128];
    dialogMenu.clear();

    std::memset(slot, 0, sizeof(slot));
    vm.initVM(slot, 0, 0);
    test.assert(vm.valid, false, "all-zero slot has no End and is invalid");
    vm.run();
    test.assert(dialogMenu.dialogCount, static_cast<uint8_t>(0), "empty slot is not decoded as Msg");

    const uint8_t truncated[] = {static_cast<uint8_t>(VmOpcode::Msg)};
    loadSlot(slot, truncated, sizeof(truncated));
    vm.initVM(slot, 0, 0);
    vm.run();
    test.assert(dialogMenu.dialogCount, static_cast<uint8_t>(0), "truncated operand cannot read End");
    test.assert(vm.ptr == slot, true, "truncated command returns to slot base safely");

    const uint8_t highCoordinates[] = {
        static_cast<uint8_t>(VmOpcode::Tp), 0x00, 0xFF, 0x00, 0xFF
    };
    loadSlot(slot, highCoordinates, sizeof(highCoordinates));
    vm.initVM(slot, 0, 0);
    test.assert(vm.valid, true, "0xFF coordinate bytes are operands, not End");
    vm.run();
    test.assert(gameState.playerLocation, To1D(255, 255), "high coordinate operands remain big endian");

    t.addTest(test);
}

void scriptCommandCapTest(TestSuite &t) {
    Test test = Test(__func__);
    ScriptVm vm = {};
    uint8_t slot[128] = {};
    dialogMenu.clear();
    scriptTextCount = 2;

    uint8_t offset = 0;
    for (uint8_t i = 0; i < 30; ++i) {
        slot[offset++] = static_cast<uint8_t>(VmOpcode::Msg);
        setAddress(slot, offset, 2); // invalid, so queue capacity cannot hide the cap
    }
    slot[offset++] = static_cast<uint8_t>(VmOpcode::Msg);
    setAddress(slot, offset, 1); // would queue if a 31st command were dispatched
    slot[offset] = static_cast<uint8_t>(VmOpcode::End);

    vm.initVM(slot, 0, 0);
    vm.run();
    test.assert(dialogMenu.dialogCount, static_cast<uint8_t>(0), "run stops after 30 commands");
    test.assert(vm.ptr == slot, true, "command cap resets cursor to slot base");
    scriptTextCount = 4;

    t.addTest(test);
}

void ScriptVmTest(TestRunner &r) {
    TestSuite t = TestSuite("Script VM Test");
    tpCommandTest(t);
    parsedBlobTest(t);
    parsedBlob2ScriptsTest(t);
    ifThenTpTest(t);
    ifThenelseTpTest(t);
    flagCommandTest(t);
    scriptBufferIsolationTest(t);
    scriptMessageTest(t);
    scriptBoundsTest(t);
    scriptCommandCapTest(t);
    r.addTestSuite(t);
}
