#pragma once
#include <stdint.h>
#include "Move.hpp"

class StatusEffect {
  public:
    Effect effects[2];
    StatusEffect() {
        effects[0] = Effect::NONE;
        effects[1] = Effect::NONE;
    }

    bool applyEffect(Effect effect) {
        if (effects[0] == Effect::NONE) {
            effects[0] = effect;
            return true;
        } else if (effects[1] == Effect::NONE) {
            effects[1] = effect;
            return true;
        }
        return false;
    }

    Effect removeEffect(Effect effect) {
        if (effects[0] == effect) {
            effects[0] = Effect::NONE;
            return effect;
        } else if (effects[1] == effect) {
            effects[1] = Effect::NONE;
            return effect;
        }
        return Effect::NONE;
    }
};