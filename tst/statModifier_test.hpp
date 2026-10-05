#pragma once
#include "test.hpp"
#include "../src/lib/StatModifier.hpp"

void SetModifierTest(TestSuite &t) {
    Test test = Test(__func__);
    StatModifer sm;
    sm.setModifier(StatType::ATTACK_M, 1);
    sm.setModifier(StatType::DEFENSE_M, 2);
    sm.setModifier(StatType::SPEED_M, 3);
    sm.setModifier(StatType::SPECIAL_ATTACK_M, 1);
    sm.setModifier(StatType::SPECIAL_DEFENSE_M, 2);
    test.assert((sm.getModifier(StatType::ATTACK_M)), 1, "Attack modifier set correctly");
    test.assert((sm.getModifier(StatType::DEFENSE_M)), 2, "Defense modifier set correctly");
    test.assert((sm.getModifier(StatType::SPEED_M)), 3, "Speed modifier set correctly");
    test.assert((sm.getModifier(StatType::SPECIAL_ATTACK_M)), 1, "Special Attack modifier set correctly");
    test.assert((sm.getModifier(StatType::SPECIAL_DEFENSE_M)), 2, "Special Defense modifier set correctly");
    t.addTest(test);
}

void SetModifierNegativeTest(TestSuite &t) {
    Test test = Test(__func__);
    StatModifer sm;
    sm.setModifier(StatType::ATTACK_M, -1);
    sm.setModifier(StatType::DEFENSE_M, -2);
    sm.setModifier(StatType::SPEED_M, -3);
    sm.setModifier(StatType::SPECIAL_ATTACK_M, -1);
    sm.setModifier(StatType::SPECIAL_DEFENSE_M, -2);
    test.assert((sm.getModifier(StatType::ATTACK_M)), -1, "Attack modifier set correctly");
    test.assert((sm.getModifier(StatType::DEFENSE_M)), -2, "Defense modifier set correctly");
    test.assert((sm.getModifier(StatType::SPEED_M)), -3, "Speed modifier set correctly");
    test.assert((sm.getModifier(StatType::SPECIAL_ATTACK_M)), -1, "Special Attack modifier set correctly");
    test.assert((sm.getModifier(StatType::SPECIAL_DEFENSE_M)), -2, "Special Defense modifier set correctly");
    t.addTest(test);
}

void NoModifierTest(TestSuite &t) {
    Test test = Test(__func__);
    StatModifer sm;
    test.assert((sm.getModifier(StatType::ATTACK_M)), 0, "Attack modifier set correctly");
    test.assert((sm.getModifier(StatType::DEFENSE_M)), 0, "Defense modifier set correctly");
    test.assert((sm.getModifier(StatType::SPEED_M)), 0, "Speed modifier set correctly");
    test.assert((sm.getModifier(StatType::SPECIAL_ATTACK_M)), 0, "Special Attack modifier set correctly");
    test.assert((sm.getModifier(StatType::SPECIAL_DEFENSE_M)), 0, "Special Defense modifier set correctly");
    t.addTest(test);
}

void ClampModifer(TestSuite &t) {
    Test test = Test(__func__);
    StatModifer sm;
    sm.setModifier(StatType::SPEED_M, 2);
    test.assert((sm.getModifier(StatType::SPEED_M)), 2, "Attack modifier set positive correctly");
    sm.incrementModifier(StatType::SPEED_M, 1);
    test.assert((sm.getModifier(StatType::SPEED_M)), 2, "Attack modifier clamped positive correctly");

    sm.setModifier(StatType::SPEED_M, -2);
    test.assert((sm.getModifier(StatType::SPEED_M)), -2, "Attack modifier set neg correctly");

    sm.incrementModifier(StatType::SPEED_M, -1);
    test.assert((sm.getModifier(StatType::SPEED_M)), -2, "Attack modifier clamped neg correctly");
    t.addTest(test);
}

void PackedModifierIsolationTest(TestSuite &suite) {
    Test test(__func__);
    for (uint8_t index = 0; index < 5; ++index) {
        for (int8_t amount = -3; amount <= 3; ++amount) {
            StatModifer stages;
            stages.modifiers = 0x6db6;
            int8_t before[5];
            for (uint8_t stat = 0; stat < 5; ++stat)
                before[stat] = stages.getModifier(static_cast<StatType>(stat));
            stages.setModifier(static_cast<StatType>(index), amount);
            for (uint8_t stat = 0; stat < 5; ++stat)
                test.assert(stages.getModifier(static_cast<StatType>(stat)),
                            stat == index ? amount : before[stat],
                            "packed stage update preserves every neighboring stage");
        }
    }
    StatModifer stages;
    stages.modifiers = 0x6db6;
    for (uint16_t id = 5; id <= 255; ++id) {
        test.assert(stages.getModifier(static_cast<StatType>(id)), 0,
                    "invalid stat reads zero without an oversized shift");
        stages.setModifier(static_cast<StatType>(id), -3);
        test.assert(stages.modifiers, uint16_t(0x6db6),
                    "invalid stat write leaves packed stages unchanged");
    }
    suite.addTest(test);
}

void ModifierSuite(TestRunner &r) {
    TestSuite t = TestSuite("Modifier Suite");
    SetModifierTest(t);
    SetModifierNegativeTest(t);
    NoModifierTest(t);
    ClampModifer(t);
    PackedModifierIsolationTest(t);
    r.addTestSuite(t);
}