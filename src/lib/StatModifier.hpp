#pragma once
#include "Move.hpp"
#include "Stats.hpp"

class StatModifer {
  public:
    // 1 bit wasted (most significant bit is always 0)
    uint16_t modifiers = 0;

    // Three sign-magnitude bits per stat; StatType fixes their packed order.
    void setModifier(StatType stat, int8_t amount) {
        uint8_t modifier = amount;
        if (amount < 0) {
            modifier = 0b00000100 | ((~amount + 1) & 0b11);
        }
        const uint8_t index = static_cast<uint8_t>(stat);
        if (index >= 5) return;
        const uint8_t shift = index * 3;
        modifiers = (modifiers & ~(static_cast<uint16_t>(7u) << shift)) |
                    (static_cast<uint16_t>(modifier) << shift);
    }

    int8_t getModifier(StatType stat) const {
        const uint8_t index = static_cast<uint8_t>(stat);
        if (index >= 5) return 0;
        const uint8_t bits = (modifiers >> (index * 3)) & 7u;
        return bits & 4u ? -static_cast<int8_t>(bits & 3u)
                         : static_cast<int8_t>(bits);
    }

    void incrementModifier(StatType stat, int8_t amount) {
        int8_t toSet = getModifier(stat) + amount;
        if (toSet > 2) {
            toSet = 2;
        } else if (toSet < -2) {
            toSet = -2;
        }

        setModifier(stat, toSet);
    }

    void clearModifiers() {
        modifiers = 0;
    }
};
