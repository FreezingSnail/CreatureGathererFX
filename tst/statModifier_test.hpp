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

void SetModifierNegaticeTest(TestSuite &t) {
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

void ModifierSuite(TestRunner &r) {
    TestSuite t = TestSuite("Modifier Suite");
    SetModifierTest(t);
    SetModifierNegaticeTest(t);
    r.addTestSuite(t);
}