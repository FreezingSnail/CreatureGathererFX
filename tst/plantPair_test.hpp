#pragma once
#include "test.hpp"
#include "../src/plants/PlantPair.hpp"

void isActiveTest(TestSuite &t) {
    Test test = Test(__func__);
    PlantPair pair = PlantPair();
    test.assert(pair.isActive(0), false, "isActive");
    pair.setActive(0, true);
    test.assert(pair.isActive(0), true, "isActive");
    pair.setActive(0, false);
    test.assert(pair.isActive(0), false, "isActive");
    t.addTest(test);
}

void plantPairTypeTest(TestSuite &t) {
    Test test = Test(__func__);
    PlantPair pair = PlantPair();
    test.assert(int(pair.type(0)), int(PlantType::BERRY), "type");
    pair.setType(0, PlantType::FLOWER);
    test.assert(int(pair.type(0)), int(PlantType::FLOWER), "type");
    pair.setType(0, PlantType::APPLE);
    test.assert(int(pair.type(0)), int(PlantType::APPLE), "type");
    t.addTest(test);
}

void PlantPairSuite(TestRunner &r) {
    TestSuite t = TestSuite("PlantPair Suite");
    isActiveTest(t);
    plantPairTypeTest(t);
    r.addTestSuite(t);
}