#include "action_test.hpp"
#include <cstring>   // for memcpy

#include "../../src/vm/ScriptVM.hpp"
#include "../../src/vm/opcodes.hpp"
#include "../../src/GameState.hpp"
#include "../../src/flags/flag_bit_array.hpp"
#define TEST
#include "../../src/globals.hpp"

#include <cstdint>

void clearFlags() {
    for (int i = 0; i < sizeof(FLAG_BIT_ARRAY); i++) {
        FLAG_BIT_ARRAY[0] = 0;
    }
}
void tpCommandTest(TestSuite &t) {
    Test test = Test(__func__);
    ScriptVm vm = ScriptVm();
    gameState = GameState();
    uint8_t buffer[] = {uint8_t(VmOpcode::TpIf), 0, 0, 0, 1, 0, 0, 0, 2, 255};

    vm.initVM();
    std::memcpy(sBuffer, buffer, sizeof(buffer));
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
    ScriptVm vm = ScriptVm();
    gameState = GameState();
    uint8_t buffer[] = {
        4,              // tp if
        0,  4,  0, 4,   // 4,4
        0,  12, 0, 7,   // 12,7
        255             // end
    };

    vm.initVM();
    std::memcpy(sBuffer, buffer, sizeof(buffer));
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
    ScriptVm vm = ScriptVm();
    gameState = GameState();
    uint8_t buffer[] = {
        4,              // tp if
        0,  4,  0, 4,   // 4,4
        0,  12, 0, 7,   // 12,7
        4,              // tp if
        0,  12, 0, 8,   // 12,7
        0,  4,  0, 4,   // 4,4
        255             // end
    };

    vm.initVM();
    std::memcpy(sBuffer, buffer, sizeof(buffer));
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
    ScriptVm vm = ScriptVm();
    gameState = GameState();

    // if flag_door then tp 1 1 endif;
    uint8_t buffer[] = {
        5,             // if
        0,             // flag set
        0,  0,         // flag
        1,             // then
        5,             // len
        3,             // tp
        0,  1, 0, 1,   // 1,1
        255            // end
    };

    vm.initVM();
    std::memcpy(sBuffer, buffer, sizeof(buffer));
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
    ScriptVm vm = ScriptVm();
    gameState = GameState();

    // if flag_door then tp 1 1 else tp 0 0 endif;
    uint8_t buffer[] = {
        5,             // if
        0,             // flag set
        0,  0,         // flag
        0,             // else
        5,             // len
        3,             // tp
        0,  1, 0, 1,   // 1,1
        3,             // tp
        0,  0, 0, 0,   // 0,0
        255            // end
    };

    vm.initVM();
    std::memcpy(sBuffer, buffer, sizeof(buffer));
    gameState.playerLocation = 128;
    vm.run();
    test.assert(gameState.playerLocation, 0, "did not teleport to 0,0");

    clearFlags();
    t.addTest(test);
}
void ScriptVmTest(TestRunner &r) {
    TestSuite t = TestSuite("Script VM Test");
    tpCommandTest(t);
    parsedBlobTest(t);
    parsedBlob2ScriptsTest(t);
    ifThenTpTest(t);
    ifThenelseTpTest(t);
    r.addTestSuite(t);
}
