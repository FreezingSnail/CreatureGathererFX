#include "BattleScene.hpp"

#include "BattleLayout.hpp"
#include "../PpGlyph.hpp"
#include "../../lib/Blit.hpp"
#include "../../fxdata.h"

#include <Arduboy2.h>
#include <avr/pgmspace.h>

namespace battle {
namespace BattleScene {
namespace {

constexpr uint8_t SPECIES_COUNT = 64;

uint8_t hpWidth(uint8_t hp, uint8_t maximum) {
    if (maximum == 0) return 0;
    if (hp > maximum) hp = maximum;
    return static_cast<uint8_t>(static_cast<uint16_t>(hp) * 28u / maximum);
}

void drawHp(const Combatant &creature, uint8_t y, bool showValues) {
    Blit::fillRect(48, y, 32, 20, BLACK);
    static const uint16_t labels[] PROGMEM = {
        4815, 31599, 29391, 9389, 31599, 31597
    };
    const uint8_t labelY = y == 0 ? 2 : y;
    const uint8_t offset = y == 0 ? 0 : 3;
    for (uint8_t i = 0; i < 3; ++i)
        PpGlyph::draw(50 + i * 4, labelY, pgm_read_word(labels + offset + i));

    if (showValues) {
        const uint8_t end = PpGlyph::number(50, y + 7, creature.hp);
        PpGlyph::draw(end, y + 7, 0b001001010100100);
        PpGlyph::number(end + 4, y + 7, creature.maxHp);
    }

    const uint8_t barY = y == 0 ? 10 : 35;
    Blit::fillRect(49, barY, 30, 3, WHITE);
    Blit::fillRect(50, barY + 1, 28, 1, BLACK);
    Blit::fillRect(50, barY + 1, hpWidth(creature.hp, creature.maxHp), 1, WHITE);
}

void drawCreature(const Combatant &creature, uint8_t side) {
    const uint8_t x = side == static_cast<uint8_t>(Side::Player)
        ? battle_layout::playerX : 0;
    Blit::fillRect(x, 0, battle_layout::spriteSize, battle_layout::spriteSize, BLACK);
    if (creature.id >= SPECIES_COUNT) return;

    const uint16_t frame = static_cast<uint16_t>(creature.id) * 2u +
        (side == static_cast<uint8_t>(Side::Player) ? 1u : 0u);
    Blit::draw(x, 0, battle_layout::spriteSize, battle_layout::spriteSize,
               battleSprites48 + 4, frame, Blit::PLUSMASK);
}

} // namespace

void draw(const BattleState &state) {
    // Index only the two active slots. This renderer does not construct a
    // BattleView or scan the party and move tables.
    drawCreature(state.active[static_cast<uint8_t>(Side::Opponent)],
                 static_cast<uint8_t>(Side::Opponent));
    drawCreature(state.active[static_cast<uint8_t>(Side::Player)],
                 static_cast<uint8_t>(Side::Player));
    drawHp(state.active[static_cast<uint8_t>(Side::Opponent)], 0, false);
    drawHp(state.active[static_cast<uint8_t>(Side::Player)], 20, true);
}

} // namespace BattleScene
} // namespace battle
