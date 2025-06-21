#pragma once
#include "../test.hpp"

#include "../../src/vm/ScriptVM.hpp"
#include "../../src/GameState.hpp"
#include <cstdint>

void tpCommandTest(TestSuite &t) {
    Test test = Test(__func__);
    ScriptVm vm = ScriptVm();
    GameState state = GameState();
    uint8_t buffer[] = {uint8_t(SciptCommands::cmd_tp), 0, 0, 0, 1, 0, 0, 0, 2, 255};
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
        2,              // tp
        0,  4,  0, 4,   // 4,4
        0,  12, 0, 7,   // 12,7
        255             // end
    };

    vm.initVM(buffer, &state);

    state.playerLocation = 0;
    vm.run();
    test.assert(state.playerLocation, 0, "did not teleport");
    state.playerLocation = To1D(0, 1);
    vm.run();
    test.assert(state.playerLocation, To1D(0, 2), "did not teleport");

    t.addTest(test);
}

void ScriptVmTest(TestRunner &r) {
    TestSuite t = TestSuite("Script VM Test");
    tpCommandTest(t);
    r.addTestSuite(t);
}
