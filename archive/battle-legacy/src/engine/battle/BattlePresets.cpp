#include "BattlePresets.hpp"

#include "../../lib/FxReadCounter.hpp"
#include "../../lib/ReadData.hpp"
#include "../../player/Player.hpp"

namespace battle {

void applyPlayerPreset(Player &player, const BattlePresets::Preset &stored)
{
    const BattlePresets::Preset preset = BattlePresets::copyPreset(stored);
    uint8_t reads = 0;
    for (uint8_t slot = 0; slot < 3; ++slot) {
        const BattlePresets::Member &member = preset.player[slot];
        uint8_t species = member.species;
#ifdef CGFX_TRAINER_DEMO_EXPANSION
        static const uint8_t expansionSpecies[3] = {32, 40, 52};
        species = expansionSpecies[slot];
#endif
        const CreatureData_t seed = getCreatureFromStore(species);
#ifdef __AVR__
        ++reads;
#endif
        Creature &creature = player.party[slot];
        creature.id = seed.id;
        creature.level = member.level;
        creature.loadTypes(seed);
        creature.setStats(seed);
        for (uint8_t moveSlot = 0; moveSlot < 4; ++moveSlot) {
            uint8_t id = member.moveIds[moveSlot];
#ifdef CGFX_TRAINER_DEMO_EXPANSION
            id = moveSlot == 0 ? seed.move1 : moveSlot == 1 ? seed.move2 :
                 moveSlot == 2 ? seed.move3 : seed.move4;
#endif
            if (id == BattlePresets::emptyMoveId) {
                // 32 is the historical player empty-slot encoding. The
                // source move list also has a move numbered 32, so do not
                // accidentally load its descriptor for an empty slot.
                creature.moves[moveSlot] = BattlePresets::emptyMoveId;
                creature.moveList[moveSlot] = Move();
            } else {
                creature.setMove(id, moveSlot);
                ++reads;
            }
        }
        creature.status.clearEffects();
        creature.statMods.clearModifiers();
        player.creatureHPs[slot] = creature.statlist.hp;
    }
    FxReadCounter::transitionExact(reads);
}

} // namespace battle
