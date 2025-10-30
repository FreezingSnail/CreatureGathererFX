#include "../test.hpp"
#include "../../src/GameState.hpp"
#include "../../src/globals.hpp"

#include "action_test.hpp"

GameState gameState;
uint8_t sBuffer[1024];

int main() {
    std::cout << "Starting Runner" << std::endl;
    TestRunner tests;
    std::cout << "Starting Tests" << std::endl;

    // Run test suites
    ScriptVmTest(tests);
    std::cout << "EffectSuite finished" << std::endl;

    std::cout << "Tests Finished" << std::endl;
    tests.printSummary();
    if (tests.fail()) {
        std::cout << "Tests failed!" << std::endl;
        return 1;
    } else {
        std::cout << "All tests passed!" << std::endl;
    }
    return 0;
}
