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
    sm.setModifier(StatType::SPEED_M, 3);
    test.assert((sm.getModifier(StatType::SPEED_M)), 3, "Attack modifier set positive correctly");
    sm.incrementModifier(StatType::SPEED_M, 1);
    test.assert((sm.getModifier(StatType::SPEED_M)), 3, "Attack modifier clamped positive correctly");

    sm.setModifier(StatType::SPEED_M, -3);
    test.assert((sm.getModifier(StatType::SPEED_M)), -3, "Attack modifier set neg correctly");

    sm.incrementModifier(StatType::SPEED_M, -1);
    test.assert((sm.getModifier(StatType::SPEED_M)), -3, "Attack modifier clamped neg correctly");
    t.addTest(test);
}

void ModifierSuite(TestRunner &r) {
    TestSuite t = TestSuite("Modifier Suite");
    SetModifierTest(t);
    SetModifierNegativeTest(t);
    NoModifierTest(t);
    ClampModifer(t);
    r.addTestSuite(t);
}