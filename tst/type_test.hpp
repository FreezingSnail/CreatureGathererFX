#pragma once
#include "test.hpp"

#include "../src/lib/ReadData.hpp"
#include "../src/lib/Type.hpp"
#include "fxdatatest/type_chart_fixture.hpp"

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

    for (uint8_t attack = 0; attack < 8; ++attack) {
        for (uint8_t defend = 0; defend < 8; ++defend) {
            const Type attackType = static_cast<Type>(attack);
            const Type defendType = static_cast<Type>(defend);
            test.assert(getModifier(attackType, defendType),
                        static_cast<Modifier>(type_chart_fixture::cell(attack, defend)),
                        "all 64 runtime type-chart cells match the pinned baseline");
            test.assert(getModifier(attackType, DualType(defendType, Type::NONE)),
                        static_cast<Modifier>(type_chart_fixture::cell(attack, defend)),
                        "single-type defender with NONE matches its chart cell");
            for (uint8_t second = 0; second < 8; ++second) {
                const uint8_t expected = type_chart_fixture::combined(
                    type_chart_fixture::cell(attack, defend),
                    type_chart_fixture::cell(attack, second));
                test.assert(getModifier(attackType,
                                        DualType(defendType,
                                                 static_cast<Type>(second))),
                            static_cast<Modifier>(expected),
                            "dual-type modifier combines both chart cells with immunity preserved");
            }
        }
    }

    t.addTest(test);
}

void ElementalStatusModifierTest(TestSuite &suite) {
    Test test(__func__);
    const Type types[] = {Type::SPIRIT, Type::WATER, Type::WIND, Type::EARTH,
                          Type::FIRE, Type::LIGHTNING, Type::PLANT, Type::ELDER};
    const Effect down[] = {Effect::DPRSD, Effect::SOAKED, Effect::BUFTD, Effect::SOILED,
                           Effect::SCRCHD, Effect::ZAPPED, Effect::TANGLD, Effect::REDCD};
    const Effect up[] = {Effect::ENLTND, Effect::DRNCHD, Effect::AIRSWPT, Effect::GRNDED,
                         Effect::KINDLD, Effect::CHRGD, Effect::ENRCHD, Effect::EVOLVD};
    for (uint8_t effect = 0; effect < 8; ++effect) {
        for (uint8_t first = 0; first < 8; ++first) {
            for (uint8_t second = 0; second < 10; ++second) {
                const Type secondary = second < 8 ? types[second]
                    : second == 8 ? Type::STATUS : Type::NONE;
                const bool matches = types[first] == types[effect] ||
                                     secondary == types[effect];
                const DualType dual(types[first], secondary);
                test.assert(typeEffectModifier(down[effect], dual),
                            matches ? Modifier::Half : Modifier::Same,
                            "suppression affects only its element in either type slot");
                test.assert(typeEffectModifier(up[effect], dual),
                            matches ? Modifier::Double : Modifier::Same,
                            "boost affects only its element in either type slot");
            }
        }
        test.assert(typeEffectModifier(down[effect], DualType()), Modifier::Same,
                    "untyped combatant ignores suppression");
        test.assert(typeEffectModifier(up[effect], DualType(Type::STATUS)), Modifier::Same,
                    "status type ignores elemental boost");
    }
    for (uint16_t id = 16; id <= 255; ++id) {
        test.assert(typeEffectModifier(static_cast<Effect>(id),
                                      DualType(Type::SPIRIT, Type::ELDER)),
                    Modifier::Same, "non-elemental effects and NONE leave type power unchanged");
    }
    const Modifier inverses[] = {Modifier::None, Modifier::Quadruple, Modifier::Double,
                                 Modifier::Same, Modifier::Half, Modifier::Quarter};
    for (uint16_t id = 0; id <= 255; ++id) {
        test.assert(inverseModifier(static_cast<Modifier>(id)),
                    id < 6 ? inverses[id] : Modifier::None,
                    "modifier inverse preserves immunity and rejects invalid values");
    }
    suite.addTest(test);
}

void TypeSuite(TestRunner &r) {
    TestSuite t = TestSuite("Type Suite");
    TypeTest(t);
    ElementalStatusModifierTest(t);
    r.addTestSuite(t);
}
