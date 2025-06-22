#include "moves_test.hpp"
#include "plantPair_test.hpp"
#include "plantStage_test.hpp"
#include "statModifier_test.hpp"
#include "creature_test.hpp"
#include "player_test.hpp"
#include "engine_test.hpp"
#include "opponent_test.hpp"
#include "type_test.hpp"
#include "effect_test.hpp"
#include "gamestate_test.hpp"

#include "../src/globals.hpp"

Player player = Player();
BattleEvent battleEventStack[10];
BattleEventPlayer battleEventPlayer;
MenuStack menuStack;
DialogMenu dialogMenu;

int main() {
    std::cout << "Starting Runner" << std::endl;
    TestRunner tests;
    std::cout << "Starting Tests" << std::endl;

    // Run test suites
    MoveSuite(tests);
    std::cout << "MoveSuite finished" << std::endl;
    TypeSuite(tests);
    std::cout << "TypeSuite finished" << std::endl;
    PlantPairSuite(tests);
    std::cout << "PlantPairSuite finished" << std::endl;
    PlantStageSuite(tests);
    std::cout << "PlantStageSuite finished" << std::endl;
    ModifierSuite(tests);
    std::cout << "ModifierSuite finished" << std::endl;
    CreatureSuite(tests);
    std::cout << "CreatureSuite finished" << std::endl;
    PlayerSuite(tests);
    std::cout << "PlayerSuite finished" << std::endl;
    EngineSuite(tests);
    std::cout << "EngineSuite finished" << std::endl;
    OpponentSuite(tests);
    std::cout << "OpponentSuite finished" << std::endl;
    EffectSuite(tests);
    std::cout << "EffectSuite finished" << std::endl;
    GameStateSuite(tests);
    std::cout << "GameStateSuite finished" << std::endl;

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