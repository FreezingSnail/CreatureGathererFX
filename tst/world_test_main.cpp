#include "test.hpp"
#include "world_test.hpp"

#include "../src/GameState.hpp"

GameState gameState;

int main() {
    TestRunner tests;
    WorldSuite(tests);
    tests.printSummary();
    return tests.fail() ? 1 : 0;
}
