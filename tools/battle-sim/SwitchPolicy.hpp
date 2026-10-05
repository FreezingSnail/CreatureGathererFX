#pragma once

#if defined(BATTLE_SIMULATOR) && !defined(__AVR__)

#include "ScenarioBuilder.hpp"
#include "../../src/engine/menu/MenuIntent.hpp"

namespace battle_sim {

enum class SwitchReason : uint8_t {
    None,
    ImprovedSurvival,
    ForcedReplacement,
};

struct SwitchDecision {
    MenuIntent intent;
    SwitchReason reason;
};

SwitchDecision chooseSwitchTacticalIntent(const Scenario &scenario,
                                          const battle::BattleState &state,
                                          bool switchLocked);

} // namespace battle_sim

#endif
