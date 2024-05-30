#pragma once
#include "test.hpp"
#include "../src/creature/Creature.hpp"
#include "../src/lib/ReadData.hpp"

void CreatureLoadTest(TestSuite &t) {
    Test test = Test(__func__);
    Creature creature = Creature();
    creature.load(getCreatureFromStore(0));
    test.assert(static_cast<int>(creature.id), 0, "Creature ID");
    test.assert(static_cast<int>(creature.level), 31, "Creature Level");
    test.assert(static_cast<int>(creature.types.getType1()), static_cast<int>(Type::WIND), "Creature Type 1");
    test.assert(static_cast<int>(creature.types.getType2()), static_cast<int>(Type::NONE), "Creature Type 2");
    test.assert(static_cast<int>(creature.moves[0]), 8, "Creature Move 1");
    test.assert(static_cast<int>(creature.moves[1]), 32, "Creature Move 2");
    test.assert(static_cast<int>(creature.moves[2]), 32, "Creature Move 3");
    test.assert(static_cast<int>(creature.moves[3]), 32, "Creature Move 4");
    test.assert(static_cast<int>(creature.statlist.attack), static_cast<int>(creature.seedToStat(3)), "Creature Attack");
    test.assert(static_cast<int>(creature.statlist.defense), static_cast<int>(creature.seedToStat(3)), "Creature Defense");
    test.assert(static_cast<int>(creature.statlist.speed), static_cast<int>(creature.seedToStat(3)), "Creature Speed");
    test.assert(static_cast<int>(creature.statlist.hp), static_cast<int>(creature.seedToStat(3) + 30), "Creature HP");
    test.assert(static_cast<int>(creature.statlist.spcAtk), static_cast<int>(creature.seedToStat(2)), "Creature Special Attack");
    test.assert(static_cast<int>(creature.statlist.spcDef), static_cast<int>(creature.seedToStat(2)), "Creature Special Defense");

    t.addTest(test);
}

void CreatureSuite(TestRunner &r) {
    TestSuite t = TestSuite("Creature Suite");
    CreatureLoadTest(t);
    r.addTestSuite(t);
}