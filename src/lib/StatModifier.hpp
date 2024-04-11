#pragma once
#include "Move.hpp"
#include "Stats.hpp"

class StatModifer {
  public:
    // 1 bit wasted (most significant bit is always 0)
    uint16_t modifiers;

    void setModifier(StatType stat, int8_t amount) {
        uint8_t modifier = amount;
        if (amount < 0) {
            modifier = 0b00000100 | ((~amount+1) & 0b11);
        }
        switch (stat) {
        case StatType::ATTACK_M:
            modifiers = (modifiers & 0xFFF8) | modifier;
            break;
        case StatType::DEFENSE_M:
            modifiers = (modifiers & 0xFFC7) | (modifier << 3);
            break;
        case StatType::SPEED_M:
            modifiers = (modifiers & 0x7E3F) | (modifier << 6);
            break;
        case StatType::SPECIAL_ATTACK_M:
            modifiers = (modifiers & 0x71FF) | (modifier << 9);
            break;
        case StatType::SPECIAL_DEFENSE_M:
            modifiers = (modifiers & 0xFFF) | (modifier << 12);
            break;
        }
    }

    int8_t getModifier(StatType stat) {
        int8_t ret = 0;
        switch (stat) {
        case StatType::ATTACK_M:
            ret = modifiers & 0b111;
            break;
        case StatType::DEFENSE_M:
            ret = (modifiers >> 3) & 0b111;
            break;
        case StatType::SPEED_M:
            ret = (modifiers >> 6) & 0b111;
            break;
        case StatType::SPECIAL_ATTACK_M:
            ret = (modifiers >> 9) & 0b111;
            break;
        case StatType::SPECIAL_DEFENSE_M:
            ret = (modifiers >> 12) & 0b111;
            break;
        }
        if (ret & 0b00000100) {
            return ~((ret & 0b11) -1);
        }
        return ret;
    }
};