#pragma once
#include "test.hpp"
#include "../src/lib/StatModifier.hpp"

void SetModifierTest() {
    Test t;
    StatModifer sm;
    sm.setModifier(StatType::ATTACK_M, 1);
    sm.setModifier(StatType::DEFENSE_M, 2);
    sm.setModifier(StatType::SPEED_M, 3);
    sm.setModifier(StatType::SPECIAL_ATTACK_M, 1);
    sm.setModifier(StatType::SPECIAL_DEFENSE_M, 2);
    t.assert(int(sm.getModifier(StatType::ATTACK_M)), 1, "Attack modifier set correctly");
    t.assert(int(sm.getModifier(StatType::DEFENSE_M)), 2, "Defense modifier set correctly");
    t.assert(int(sm.getModifier(StatType::SPEED_M)), 3, "Speed modifier set correctly");
    t.assert(int(sm.getModifier(StatType::SPECIAL_ATTACK_M)), 1, "Special Attack modifier set correctly");
    t.assert(int(sm.getModifier(StatType::SPECIAL_DEFENSE_M)), 2, "Special Defense modifier set correctly");
    t.printSummary("SetModifierTest");
}

void SetModifierNegaticeTest() {
    Test t;
    StatModifer sm;
    sm.setModifier(StatType::ATTACK_M, -1);
    sm.setModifier(StatType::DEFENSE_M, -2);
    sm.setModifier(StatType::SPEED_M, -3);
    sm.setModifier(StatType::SPECIAL_ATTACK_M, -1);
    sm.setModifier(StatType::SPECIAL_DEFENSE_M, -2);
    t.assert(int(sm.getModifier(StatType::ATTACK_M)), -1, "Attack modifier set correctly");
    t.assert(int(sm.getModifier(StatType::DEFENSE_M)), -2, "Defense modifier set correctly");
    t.assert(int(sm.getModifier(StatType::SPEED_M)), -3, "Speed modifier set correctly");
    t.assert(int(sm.getModifier(StatType::SPECIAL_ATTACK_M)), -1, "Special Attack modifier set correctly");
    t.assert(int(sm.getModifier(StatType::SPECIAL_DEFENSE_M)), -2, "Special Defense modifier set correctly");
    t.printSummary("SetModifierTest");
}


void ModifierSuite() {
    printHeader("StatModifier");
    SetModifierTest();
    SetModifierNegaticeTest();
}