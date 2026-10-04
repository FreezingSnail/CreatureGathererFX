#pragma once

#include "test.hpp"
#include "../src/engine/world/LurePrototype.hpp"

inline void LurePrototypeIntegrationTest(TestSuite &suite)
{
    using namespace lure_prototype;
    Test test(__func__);
    Prototype prototype;

    test.assert(prototype.isGatherTile(Prototype::GATHER_TILE), true,
                "one hardcoded gather tile is reachable");
    test.assert(prototype.isGatherTile(0x0304), false,
                "other tiles are outside the throwaway zone");
    test.assert(prototype.gatherState().need, static_cast<uint8_t>(3),
                "zone uses deterministic gather need");
    test.assert(prototype.gatherState().fleeTurns, static_cast<uint8_t>(6),
                "zone starts six-turn flee timer");
    test.assert(Prototype::ENCOUNTER_TABLE[0], static_cast<uint8_t>(4),
                "encounter table entry zero is stable");
    test.assert(Prototype::ENCOUNTER_TABLE[1], static_cast<uint8_t>(7),
                "encounter table entry one is stable");

    test.assert(prototype.gatherPlant(), true, "plant gather turn one resolves");
    test.assert(prototype.gatherPlant(), true, "plant gather turn two resolves");
    test.assert(prototype.acquired(Material::Plant), false,
                "plant remains pending before need");
    test.assert(prototype.gatherPlant(), true, "plant reaches acquisition threshold");
    test.assert(prototype.acquired(Material::Plant), true,
                "plant material acquisition is reachable");
    const Metrics plant = prototype.metrics(Material::Plant);
    test.assert(plant.turnsToAcquire, static_cast<uint8_t>(3),
                "plant turns to acquire are measured");
    test.assert(plant.hpCost, static_cast<uint8_t>(0),
                "plant acquisition has no HP cost");
    test.assert(plant.fleeFires, static_cast<uint8_t>(0),
                "plant acquisition fires no flee timer");
    test.assert(prototype.hp(), static_cast<uint8_t>(100),
                "plant path preserves HP");

    prototype.reset();
    test.assert(prototype.gatherBattleDrop(), true,
                "battle-drop encounter turn one resolves");
    test.assert(prototype.encounterCreature(), static_cast<uint8_t>(7),
                "battle-drop path selects second table entry deterministically");
    test.assert(prototype.acquired(Material::BattleDrop), false,
                "battle drop remains pending after first turn");
    test.assert(prototype.gatherBattleDrop(), true,
                "battle-drop encounter turn two resolves");
    test.assert(prototype.acquired(Material::BattleDrop), true,
                "battle-drop material acquisition is reachable");
    const Metrics drop = prototype.metrics(Material::BattleDrop);
    test.assert(drop.turnsToAcquire, static_cast<uint8_t>(2),
                "battle-drop turns to acquire are measured");
    test.assert(drop.hpCost, static_cast<uint8_t>(8),
                "battle-drop HP cost is measured per acquisition");
    test.assert(drop.fleeFires, static_cast<uint8_t>(0),
                "battle-drop acquisition precedes flee expiry");
    test.assert(prototype.encounterCreature(), static_cast<uint8_t>(4),
                "battle-drop table cycles deterministically");
    test.assert(prototype.gatherBattleDrop(), false,
                "acquired material cannot be duplicated in one run");

    prototype.reset();
    for (uint8_t turn = 0; turn < Prototype::FLEE_TURNS - 1; ++turn) {
        prototype.waitTurn();
    }
    test.assert(prototype.fleeFireCount(), static_cast<uint8_t>(0),
                "flee timer does not fire early");
    prototype.waitTurn();
    test.assert(prototype.fleeFireCount(), static_cast<uint8_t>(1),
                "flee timer fires once at six turns");
    test.assert(prototype.gatherState().fleeTurns, Prototype::FLEE_TURNS,
                "flee timer retunes to six after firing");

    suite.addTest(test);
}

inline void LurePrototypeSuite(TestRunner &runner)
{
    TestSuite suite("M0.5 lure prototype");
    LurePrototypeIntegrationTest(suite);
    runner.addTestSuite(suite);
}
