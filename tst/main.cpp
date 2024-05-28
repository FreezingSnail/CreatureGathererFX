#include "moves_test.hpp"
#include "plantPair_test.hpp"
#include "plantStage_test.hpp"
#include "statModifier_test.hpp"
#include "creature_test.hpp"
#include "player_test.hpp"

#include "../src/globals.hpp"
Player player = Player();
BattleEvent battleEventStack[10];
BattleEventPlayer battleEventPlayer;
MenuStack menuStack;
DialogMenu dialogMenu;

int main() {
    MoveSuite();
    PlantPairSuite();
    PlantStageSuite();
    ModifierSuite();
    CreatureSuite();
    PlayerSuite();
    return 0;
}