#pragma once
#include "test.hpp"
#include "../src/creature/Creature.hpp"
#include "../src/lib/ReadData.hpp"

void CreatureLoadTest(TestSuite &t) {
    Test test = Test(__func__);
    Creature creature = Creature();
    creature.load(getCreatureFromStore(0));
    test.assert(creature.id, 0, "Creature ID");
    test.assert(creature.level, 31, "Creature Level");
    test.assert(creature.types.getType1(), static_cast<int>(Type::WIND), "Creature Type 1");
    test.assert(creature.types.getType2(), static_cast<int>(Type::NONE), "Creature Type 2");
    test.assert(creature.moves[0], 8, "Creature Move 1");
    test.assert(creature.moves[1], 32, "Creature Move 2");
    test.assert(creature.moves[2], 32, "Creature Move 3");
    test.assert(creature.moves[3], 32, "Creature Move 4");
    test.assert(creature.statlist.attack, creature.seedToStat(3), "Creature Attack");
    test.assert(creature.statlist.defense, creature.seedToStat(3), "Creature Defense");
    test.assert(creature.statlist.speed, creature.seedToStat(3), "Creature Speed");
    test.assert(creature.statlist.hp, creature.seedToStat(3) + 30, "Creature HP");
    test.assert(creature.statlist.spcAtk, creature.seedToStat(2), "Creature Special Attack");
    test.assert(creature.statlist.spcDef, creature.seedToStat(2), "Creature Special Defense");

    t.addTest(test);
}

void CreatureLoadFromOpponnetSeed(TestSuite &t) {
    Test test = Test(__func__);
    Creature creature = Creature();
    creature.loadFromOpponentSeed({0, 31, 4294967040});
    test.assert(creature.id, 0, "Creature ID");
    test.assert(creature.level, 31, "Creature Level");
    test.assert(creature.types.getType1(), static_cast<int>(Type::WIND), "Creature Type 1");
    test.assert(creature.types.getType2(), static_cast<int>(Type::NONE), "Creature Type 2");
    test.assert(creature.moves[0], 0, "Creature Move 1");
    test.assert(creature.moves[1], 255, "Creature Move 2");
    test.assert(creature.moves[2], 255, "Creature Move 3");
    test.assert(creature.moves[3], 255, "Creature Move 4");

    t.addTest(test);
}

void CreatureSuite(TestRunner &r) {
    TestSuite t = TestSuite("Creature Suite");
    CreatureLoadTest(t);
    CreatureLoadFromOpponnetSeed(t);
    r.addTestSuite(t);
}