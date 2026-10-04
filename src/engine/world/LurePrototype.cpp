#include "LurePrototype.hpp"

namespace lure_prototype {

Prototype::Prototype()
{
    reset();
}

uint8_t Prototype::materialIndex(Material material) const
{
    const uint8_t index = static_cast<uint8_t>(material);
    return index < 2 ? index : 255;
}

void Prototype::reset()
{
    gather_ = {0, GATHER_NEED, FLEE_TURNS, 1};
    metrics_[0] = {0, 0, 0};
    metrics_[1] = {0, 0, 0};
    tile_ = GATHER_TILE;
    acquiredMask_ = 0;
    turn_ = 0;
    hp_ = 100;
    battleTurns_ = 0;
    encounterIndex_ = 0;
    fleeFires_ = 0;
}

bool Prototype::isGatherTile(uint16_t tile) const
{
    return tile == GATHER_TILE && tile_ == GATHER_TILE;
}

void Prototype::countTurn(uint8_t hpCost)
{
    if (turn_ != 255) ++turn_;
    const uint8_t actualCost = hpCost > hp_ ? hp_ : hpCost;
    hp_ = static_cast<uint8_t>(hp_ - actualCost);

    if (gather_.fleeTurns != 0) --gather_.fleeTurns;
    if (gather_.fleeTurns == 0) {
        if (fleeFires_ != 255) ++fleeFires_;
        gather_.fleeTurns = FLEE_TURNS;
    }
}

bool Prototype::take(Material material)
{
    const uint8_t index = materialIndex(material);
    if (index >= 2 || !isGatherTile(tile_) || acquired(material)) return false;

    if (material == Material::Plant) {
        countTurn(0);
        if (gather_.progress < gather_.need) ++gather_.progress;
        if (gather_.progress < gather_.need) return true;
        acquiredMask_ |= static_cast<uint8_t>(1u << index);
    } else {
        countTurn(BATTLE_HP_COST);
        if (battleTurns_ != 255) ++battleTurns_;
        encounterIndex_ = static_cast<uint8_t>(battleTurns_ & 1u);
        if (battleTurns_ < BATTLE_TURNS_TO_DROP) return true;
        acquiredMask_ |= static_cast<uint8_t>(1u << index);
    }

    metrics_[index].turnsToAcquire = turn_;
    metrics_[index].fleeFires = fleeFires_;
    return true;
}

bool Prototype::gatherPlant()
{
    return take(Material::Plant);
}

bool Prototype::gatherBattleDrop()
{
    const uint8_t before = hp_;
    const bool result = take(Material::BattleDrop);
    if (result) {
        const uint8_t spent = static_cast<uint8_t>(before - hp_);
        metrics_[static_cast<uint8_t>(Material::BattleDrop)].hpCost =
            static_cast<uint8_t>(metrics_[static_cast<uint8_t>(Material::BattleDrop)].hpCost + spent);
    }
    return result;
}

void Prototype::waitTurn()
{
    countTurn(0);
}

bool Prototype::acquired(Material material) const
{
    const uint8_t index = materialIndex(material);
    return index < 2 && (acquiredMask_ & static_cast<uint8_t>(1u << index)) != 0;
}

Metrics Prototype::metrics(Material material) const
{
    const uint8_t index = materialIndex(material);
    return index < 2 ? metrics_[index] : Metrics{};
}

const battle::GatherState &Prototype::gatherState() const
{
    return gather_;
}

uint8_t Prototype::hp() const
{
    return hp_;
}

uint8_t Prototype::encounterCreature() const
{
    return ENCOUNTER_TABLE[encounterIndex_];
}

uint8_t Prototype::fleeFireCount() const
{
    return fleeFires_;
}

} // namespace lure_prototype
