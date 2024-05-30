#include "moves_test.hpp"
#include "plantPair_test.hpp"
#include "plantStage_test.hpp"
#include "statModifier_test.hpp"
#include "creature_test.hpp"
#include "player_test.hpp"
#include "engine_test.hpp"
#include "opponent_test.hpp"

#include "../src/globals.hpp"

Player player = Player();
BattleEvent battleEventStack[10];
BattleEventPlayer battleEventPlayer;
MenuStack menuStack;
DialogMenu dialogMenu;

int main() {
    TestRunner tests;
    MoveSuite(tests);
    PlantPairSuite(tests);
    PlantStageSuite(tests);
    ModifierSuite(tests);
    CreatureSuite(tests);
    PlayerSuite(tests);
    EngineSuite(tests);
    OpponentSuite(tests);

    tests.printSummary();
    if (tests.fail()) {
        return 1;
    }
    return 0;
}