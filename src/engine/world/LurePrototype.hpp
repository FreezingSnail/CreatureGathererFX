#pragma once

#include <stdint.h>

#include "../battle/BattleState.hpp"

namespace lure_prototype {

enum class Material : uint8_t {
    Plant = 0,
    BattleDrop = 1,
};

struct Metrics {
    uint8_t turnsToAcquire;
    uint8_t hpCost;
    uint8_t fleeFires;
};

// M0.5 throwaway wedge: one zone, one gather tile, and one two-entry encounter
// table. It deliberately owns no SaveFile, Inventory, FX data, or UI state.
class Prototype {
  public:
    static constexpr uint16_t ZONE_ID = 0;
    static constexpr uint16_t GATHER_TILE = 0x0203;
    static constexpr uint8_t GATHER_NEED = 3;
    static constexpr uint8_t FLEE_TURNS = 6;
    static constexpr uint8_t BATTLE_TURNS_TO_DROP = 2;
    static constexpr uint8_t BATTLE_HP_COST = 4;
    static constexpr uint8_t ENCOUNTER_TABLE[2] = {4, 7};

    Prototype();

    void reset();
    bool isGatherTile(uint16_t tile) const;
    bool gatherPlant();
    bool gatherBattleDrop();
    void waitTurn();

    bool acquired(Material material) const;
    Metrics metrics(Material material) const;
    const battle::GatherState &gatherState() const;
    uint8_t hp() const;
    uint8_t encounterCreature() const;
    uint8_t fleeFireCount() const;

  private:
    battle::GatherState gather_;
    Metrics metrics_[2];
    uint16_t tile_;
    uint8_t acquiredMask_;
    uint8_t turn_;
    uint8_t hp_;
    uint8_t battleTurns_;
    uint8_t encounterIndex_;
    uint8_t fleeFires_;

    bool take(Material material);
    void countTurn(uint8_t hpCost);
    uint8_t materialIndex(Material material) const;
};

} // namespace lure_prototype
