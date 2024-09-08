#pragma once
#include "test.hpp"

#include "../src/lib/ReadData.hpp"
#include "../src/lib/Type.hpp"

void TypeTest(TestSuite &t) {
    Test test = Test(__func__);
    DualType dt = DualType(Type::EARTH, Type::NONE);
    test.assert(dt.getType1(), Type::EARTH, "Primary Type");
    test.assert(dt.getType2(), Type::NONE, "Secondary Type");

    dt = DualType(Type::EARTH, Type::FIRE);
    test.assert(dt.getType1(), Type::EARTH, "Primary Type");
    test.assert(dt.getType2(), Type::FIRE, "Secondary Type");

    DualType dt2 = DualType(Type::FIRE, Type::EARTH);
    test.assert(dt2.getType1(), Type::FIRE, "Primary Type");
    test.assert(dt2.getType2(), Type::EARTH, "Secondary Type");
    test.assert(dt2.hasType(Type::FIRE), true, "Has Fire Type");
    test.assert(dt2.hasType(Type::EARTH), true, "Has Earth Type");

    dt = DualType(Type::EARTH, Type::FIRE);
    test.assert(dt.getType1(), Type::EARTH, "Primary Type");
    test.assert(dt.getType2(), Type::FIRE, "Secondary Type");

    dt = DualType(Type::SPIRIT, Type::WATER);
    test.assert(dt.getType1(), Type::SPIRIT, "Primary Type");
    test.assert(dt.getType2(), Type::WATER, "Secondary Type");

    dt = DualType(Type::WIND, Type::EARTH);
    test.assert(dt.getType1(), Type::WIND, "Primary Type");
    test.assert(dt.getType2(), Type::EARTH, "Secondary Type");

    dt = DualType(Type::LIGHTNING, Type::PLANT);
    test.assert(dt.getType1(), Type::LIGHTNING, "Primary Type");
    test.assert(dt.getType2(), Type::PLANT, "Secondary Type");

    dt = DualType(Type::ELDER, Type::NONE);
    test.assert(dt.getType1(), Type::ELDER, "Primary Type");
    test.assert(dt.getType2(), Type::NONE, "Secondary Type");

    dt = DualType(Type::NONE, Type::NONE);
    test.assert(dt.getType1(), Type::NONE, "Primary Type");
    test.assert(dt.getType2(), Type::NONE, "Secondary Type");

    t.addTest(test);
}

void TypeSuite(TestRunner &r) {
    TestSuite t = TestSuite("Type Suite");
    TypeTest(t);
    r.addTestSuite(t);
}