#pragma once
#include "../test.hpp"

#include "../../src/vm/ScriptVM.hpp"
#include "../../src/vm/opcodes.hpp"
#include "../../src/GameState.hpp"
#include <cstdint>

void clearFlags() {
    for (int i = 0; i < sizeof(FLAG_BIT_ARRAY); i++) {
        FLAG_BIT_ARRAY[0] = 0;
    }
}
void tpCommandTest(TestSuite &t) {
    Test test = Test(__func__);
    ScriptVm vm = ScriptVm();
    GameState state = GameState();
    uint8_t buffer[] = {uint8_t(VmOpcode::TpIf), 0, 0, 0, 1, 0, 0, 0, 2, 255};
    vm.initVM(buffer, &state);

    state.playerLocation = 0;
    vm.run();
    test.assert(state.playerLocation, 0, "teleported off tile 0 incorrectly");
    state.playerLocation = To1D(0, 1);
    vm.run();
    test.assert(state.playerLocation, To1D(0, 2), "did not teleport to tile 0,2");

    t.addTest(test);
}

void parsedBlobTest(TestSuite &t) {
    Test test = Test(__func__);
    ScriptVm vm = ScriptVm();
    GameState state = GameState();
    uint8_t buffer[] = {
        3,              // tp
        0,  4,  0, 4,   // 4,4
        0,  12, 0, 7,   // 12,7
        255             // end
    };

    vm.initVM(buffer, &state);

    state.playerLocation = 0;
    vm.run();
    test.assert(state.playerLocation, 0, "did not teleport");
    state.playerLocation = To1D(4, 4);
    vm.run();
    test.assert(state.playerLocation, To1D(12, 7), "did not teleport");

    t.addTest(test);
}

void parsedBlob2ScriptsTest(TestSuite &t) {
    Test test = Test(__func__);
    ScriptVm vm = ScriptVm();
    GameState state = GameState();
    uint8_t buffer[] = {
        3,              // tp
        0,  4,  0, 4,   // 4,4
        0,  12, 0, 7,   // 12,7
        3,              // tp
        0,  12, 0, 8,   // 12,7
        0,  4,  0, 4,   // 4,4
        255             // end
    };

    vm.initVM(buffer, &state);

    state.playerLocation = 0;
    vm.run();
    test.assert(state.playerLocation, 0, "teleported");
    state.playerLocation = To1D(4, 4);
    vm.run();
    test.assert(state.playerLocation, To1D(12, 7), "did not teleport to 12, 7");
    state.playerLocation = To1D(12, 8);
    vm.run();
    test.assert(state.playerLocation, To1D(4, 4), "did not teleport to 4, 4");

    t.addTest(test);
}

// if flag_test1 then tp 1 1 endif;
uint8_t blob0[] = {
    4,             // if
    0,             // flag set
    0,  0,         // flag
    1,             // then
    5,             // len
    2,             // tp
    0,  1, 0, 1,   // 1,1
    255            // end
};

void ifThenTpTest(TestSuite &t) {
    Test test = Test(__func__);
    ScriptVm vm = ScriptVm();
    GameState state = GameState();

    vm.initVM(blob0, &state);

    state.playerLocation = 0;
    vm.run();
    test.assert(state.playerLocation, 0, "teleported");
    state.setFlag(0);
    vm.run();
    test.assert(state.playerLocation, To1D(1, 1), "did not teleport to 1, 1");

    clearFlags();
    t.addTest(test);
}

// if flag_test1 then tp 1 1 else tp 0 0 endif;
uint8_t blob65[] = {
    4,             // if
    0,             // flag set
    0,  0,         // flag
    0,             // else
    5,             // len
    2,             // tp
    0,  1, 0, 1,   // 1,1
    2,             // tp
    0,  0, 0, 0,   // 0,0
    255            // end
};
void ifThenelseTpTest(TestSuite &t) {
    Test test = Test(__func__);
    ScriptVm vm = ScriptVm();
    GameState state = GameState();

    vm.initVM(blob65, &state);

    state.playerLocation = 128;
    vm.run();
    test.assert(state.playerLocation, 0, "did not teleport to 0,0");

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
