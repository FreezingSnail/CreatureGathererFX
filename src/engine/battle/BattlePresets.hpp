#pragma once

#if __has_include("../../../tst/fxdatatest/generated/battle_preset_data.hpp")
#include "../../../tst/fxdatatest/generated/battle_preset_data.hpp"
#else
// The device-test builder flattens generated fixtures beside the sketch.
#include "../../../battle_preset_data.hpp"
#endif

class Player;

namespace battle {

// Dev and device-test bootstrap only. `stored` must reference a generated
// PROGMEM preset on AVR, not a RAM copy. Reads each canonical species and
// authored non-empty move once, then initializes persistent party HP.
void applyPlayerPreset(Player &player, const BattlePresets::Preset &stored);

} // namespace battle
