#include "BattleScene.hpp"

#include "BattleLayout.hpp"
#include "../PpGlyph.hpp"
#include "../../lib/Blit.hpp"
#include "../../lib/Text.hpp"
#include "../../fxdata.h"
#ifdef CGFX_DUEL_BATTLE
#include "DuelView.hpp"
#endif

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
#ifdef CGFX_DUEL_BATTLE
    // The duel HUD shows numerals on both sides; the foe panel drops the FOE
    // label and hugs its numerals to the top so they stay disjoint from the
    // y=10 bar. The legacy path keeps the original label + y+7 numerals.
    const bool showFoeNumbers = showValues && y == 0;
#else
    const bool showFoeNumbers = false;
#endif
    if (!showFoeNumbers) {
        const uint8_t labelY = y == 0 ? 2 : y;
        const uint8_t offset = y == 0 ? 0 : 3;
        for (uint8_t i = 0; i < 3; ++i)
            PpGlyph::draw(50 + i * 4, labelY, pgm_read_word(labels + offset + i));
    }
    if (showValues) {
#ifdef CGFX_DUEL_BATTLE
        const uint8_t numberY = y == 0 ? 1 : y + 7;
#else
        const uint8_t numberY = y + 7;
#endif
        const uint8_t end = PpGlyph::number(50, numberY, creature.hp);
        PpGlyph::draw(end, numberY, 0b001001010100100);
        PpGlyph::number(end + 4, numberY, creature.maxHp);
    }

    const uint8_t barY = y == 0 ? 10 : 35;
    Blit::fillRect(49, barY, 30, 3, WHITE);
    Blit::fillRect(50, barY + 1, 28, 1, BLACK);
    Blit::fillRect(50, barY + 1, hpWidth(creature.hp, creature.maxHp), 1, WHITE);
}

#ifdef CGFX_DUEL_BATTLE
// Duel team pips: one cell per original member, alive = 2x3, active underline
// 2x1 at y+4. Positions match mock/duel-bonus.html (x = 72 + slot*3; foe y=2,
// you y=27). Uses only resident state, so it is metadata-read free.
void drawPips(const BattleState &state, uint8_t side, uint8_t y) {
    const uint8_t count = state.partyCount[side] > PARTY_SIZE
        ? PARTY_SIZE : state.partyCount[side];
    const uint8_t active = state.activeSlot[side];
    uint8_t bench = 0;
    for (uint8_t original = 0; original < count; ++original) {
        const uint8_t hp = original == active
            ? state.active[side].hp : state.bench[side][bench].hp;
        if (original != active) ++bench;
        const uint8_t x = static_cast<uint8_t>(72 + original * 3);
        if (hp != 0) Blit::fillRect(x, y, 2, 3, WHITE);
        if (original == active) Blit::fillRect(x, y + 4, 2, 1, WHITE);
    }
}

// Icon sheet frames (tools/export-duel-icons.mjs): white-ink 0..6, black-ink
// 7..13. A white-ink PLUSMASK stencil paints white; a black-ink frame clears to
// black, so the selected (white) cell shows the mock's inverted black glyph.
constexpr uint8_t DUEL_ICON_DARK = 7;
constexpr uint8_t DUEL_MARK_UP_WHITE = 5;
constexpr uint8_t DUEL_MARK_DOWN_WHITE = 6;
constexpr uint8_t DUEL_MARK_UP_DARK = 12;
constexpr uint8_t DUEL_MARK_DOWN_DARK = 13;

// Type badges pinned to each creature art: white 12x12 frame, black 10x10 inner,
// 8x8 type icon. The player creature sits at the right (Side::Player == 0), so
// player chips read bottom-right and foe chips bottom-left (mock).
void drawBadges(const BattleState &state) {
    for (uint8_t side = 0; side < 2; ++side) {
        // An empty active slot (level 0) has no real types; skip its badge so a
        // drained side never renders a fold-of-NONE (type 255) chip.
        if (state.active[side].level == 0) continue;
        const bool isPlayer = side == static_cast<uint8_t>(Side::Player);
        uint8_t types[2];
        const uint8_t count = duel::activeTypes(state, side, types);
        for (uint8_t i = 0; i < count && i < 2; ++i) {
            const uint8_t x = isPlayer ? static_cast<uint8_t>(113 - i * 12)
                                       : static_cast<uint8_t>(1 + i * 12);
            Blit::fillRect(static_cast<int16_t>(x) - 1, 36, 12, 12, WHITE);
            Blit::fillRect(x, 37, 10, 10, BLACK);
            Blit::draw(x + 1, 38, 8, 8, duelIcons + 4, types[i], Blit::PLUSMASK);
        }
    }
}

void drawTypeIcon(uint8_t x, uint8_t y, uint8_t type, bool selected) {
    const uint8_t frame = selected
        ? static_cast<uint8_t>(DUEL_ICON_DARK + type) : type;
    Blit::draw(x, y, 8, 8, duelIcons + 4, frame, Blit::PLUSMASK);
}

// A text string raster retains one leading blank glyph slot (5 bytes for the
// 5x6 font), and each frame is (chars+1)*5 bytes wide. Draw only the authored
// glyphs so labels fit their cells: skip the leading blank (frame 0) or the
// leading blank of the inverted frame (frame 1), always compiled at frame 0
// with the trimmed visible width as the stride.
void drawDuelLabel(uint8_t x, uint8_t y, uint24_t address, uint8_t visibleWidth,
                   bool dark) {
    const uint24_t base = address + (dark ? visibleWidth + 10 : 5);
    Blit::draw(x, y, visibleWidth, 8, base, 0, Blit::OVERWRITE);
}

void drawMarker(uint8_t x, uint8_t y, uint8_t marker, bool selected) {
    const uint8_t frame = marker == 1
        ? (selected ? DUEL_MARK_UP_DARK : DUEL_MARK_UP_WHITE)
        : (selected ? DUEL_MARK_DOWN_DARK : DUEL_MARK_DOWN_WHITE);
    Blit::draw(x, y, 8, 8, duelIcons + 4, frame, Blit::PLUSMASK);
}

// Compact pick row + centre strip (mock drawDuelMoveRow / choice centre).
void drawDuelChoice(const BattleState &state, int8_t cursor) {
    duel::Cells cells;
    duel::buildCells(state, 0, cells);
    const uint8_t count = cells.count;
    const uint8_t cell = static_cast<uint8_t>(120 / count);
    for (uint8_t index = 0; index < count; ++index) {
        const uint8_t x = static_cast<uint8_t>(4 + index * cell);
        const bool selected = cursor >= 0 && static_cast<uint8_t>(cursor) == index;
        if (selected) Blit::fillRect(x, 48, static_cast<uint8_t>(cell - 2), 14, WHITE);
        if (index < cells.options) {
            drawTypeIcon(static_cast<uint8_t>(x + (cell - 2 - 8) / 2), 50,
                         cells.types[index], selected);
            if (cells.markers[index] != 0)
                drawMarker(static_cast<uint8_t>(x + cell - 10), 50,
                           cells.markers[index], selected);
        } else if (cells.swap && index == cells.options) {
            const uint8_t labelX = static_cast<uint8_t>(x + (cell - 2 - 20) / 2);
            drawDuelLabel(labelX, 53, duelSwap, 20, selected);
        } else {
            const uint8_t labelX = static_cast<uint8_t>(x + (cell - 2 - 20) / 2);
            drawDuelLabel(labelX, 53, duelWait, 20, selected);
        }
    }

    // Centre: R# then the selected cell's type icon (or SWAP label). The visible
    // glyphs keep the strip within x48..79 (R 5px, digits, then content).
    drawDuelLabel(48, 41, duelRound, 5, false);
    const uint8_t numberEnd = PpGlyph::number(53, 41, cells.round);
    const uint8_t contentX = numberEnd > 60 ? 60 : numberEnd;
    const bool onSwap = cells.swap && cursor >= 0 &&
                        static_cast<uint8_t>(cursor) == cells.options;
    if (onSwap) {
        drawDuelLabel(contentX, 41, duelSwap, 20, false);
    } else if (cursor >= 0 && static_cast<uint8_t>(cursor) < cells.options) {
        Blit::draw(contentX, 40, 8, 8, duelIcons + 4, cells.types[cursor],
                   Blit::PLUSMASK);
    }
}

void duelCaption(uint8_t feedback, uint24_t &address, uint8_t &width) {
    address = 0;
    width = 0;
    switch (static_cast<duel::Feedback>(feedback)) {
    case duel::Feedback::BonusWin: address = duelBonusWin; width = 50; break;
    case duel::Feedback::BonusLose: address = duelBonusLose; width = 55; break;
    case duel::Feedback::WeakWin: address = duelWeakWin; width = 45; break;
    case duel::Feedback::WeakLose: address = duelWeakLose; width = 50; break;
    case duel::Feedback::RpsWin: address = duelRpsWin; width = 40; break;
    case duel::Feedback::RpsLose: address = duelRpsLose; width = 45; break;
    case duel::Feedback::Trade: address = duelTrade; width = 30; break;
    case duel::Feedback::DefaultWin: address = duelDefaultWin; width = 60; break;
    case duel::Feedback::Swap: address = duelSwap; width = 25; break;
    case duel::Feedback::Wait: address = duelWait; width = 25; break;
    case duel::Feedback::Draw: address = duelDraw; width = 25; break;
    case duel::Feedback::Gather: address = gather; width = 35; break;
    case duel::Feedback::Gathered: address = battleGathered; width = 45; break;
    case duel::Feedback::Fled: address = battleFled; width = 25; break;
    case duel::Feedback::Win: address = battleWin; width = 20; break;
    case duel::Feedback::Lose: address = battleDefeat; width = 35; break;
    case duel::Feedback::Escape: address = battleEscaped; width = 40; break;
    case duel::Feedback::None: default: break;
    }
}

// One signed damage figure: a dash then the number.
void drawDamage(uint8_t x, uint8_t y, uint8_t amount) {
    Blit::fillRect(x, y + 2, 3, 1, WHITE);
    PpGlyph::number(static_cast<uint8_t>(x + 4), y, amount);
}

// Result footer: both revealed move icons with 'vs', the caption and both
// damage figures. The mock prints the player's outgoing damage first, so
// damage[opponent] (dealt to the foe) precedes damage[player].
void drawDuelFeedback(const BattleState &state) {
    Blit::fillRect(0, 48, 128, 16, BLACK);
    if (state.reveal[0] != duel::NO_TYPE)
        Blit::draw(6, 50, 8, 8, duelIcons + 4, state.reveal[0], Blit::PLUSMASK);
    if (state.reveal[1] != duel::NO_TYPE)
        Blit::draw(30, 50, 8, 8, duelIcons + 4, state.reveal[1], Blit::PLUSMASK);
    if (state.reveal[0] != duel::NO_TYPE || state.reveal[1] != duel::NO_TYPE)
        drawDuelLabel(18, 51, duelVs, 10, false);

    uint24_t caption = 0;
    uint8_t width = 0;
    duelCaption(state.feedback, caption, width);
    if (caption != 0) drawText(42, 48, caption, width, 0);

    drawDamage(42, 56, state.damage[1]);
    drawDamage(66, 56, state.damage[0]);
}
#endif

void drawCreature(const Combatant &creature, uint8_t side) {
    const uint8_t x = side == static_cast<uint8_t>(Side::Player)
        ? battle_layout::playerX : 0;
    Blit::fillRect(x, 0, battle_layout::spriteSize, battle_layout::spriteSize, BLACK);
#ifdef CGFX_DUEL_BATTLE
    // level 0 is the empty-active sentinel (empty or exhausted party). Draw no
    // sprite, so a drained slot never renders species 0 as an occupied creature.
    // Species 0 at a real level is a normal creature and still draws.
    if (creature.level == 0) return;
#endif
    if (creature.id >= SPECIES_COUNT) return;

    const uint16_t frame = static_cast<uint16_t>(creature.id) * 2u +
        (side == static_cast<uint8_t>(Side::Player) ? 1u : 0u);
    Blit::draw(x, 0, battle_layout::spriteSize, battle_layout::spriteSize,
               battleSprites48 + 4, frame, Blit::PLUSMASK);
}

} // namespace

void draw(const BattleState &state, int8_t duelCursor) {
    // Index only the two active slots. This renderer does not construct a
    // BattleView or scan the party and move tables.
    drawCreature(state.active[static_cast<uint8_t>(Side::Opponent)],
                 static_cast<uint8_t>(Side::Opponent));
    drawCreature(state.active[static_cast<uint8_t>(Side::Player)],
                 static_cast<uint8_t>(Side::Player));
#ifdef CGFX_DUEL_BATTLE
    // The canonical duel HUD shows numeric HP and a bar on both sides.
    drawHp(state.active[static_cast<uint8_t>(Side::Opponent)], 0, true);
#else
    drawHp(state.active[static_cast<uint8_t>(Side::Opponent)], 0, false);
#endif
    drawHp(state.active[static_cast<uint8_t>(Side::Player)], 20, true);
#ifdef CGFX_DUEL_BATTLE
    drawBadges(state);
    // The native HUD keeps three-digit HP numerals (up to x77) and the pips both
    // visible: the pips move below the foe numerals (y13) and above the player
    // numerals, right of the YOU label (y21). Documented layout accommodation;
    // see tst/fxdatatest/duel_session_integration_test.hpp.
    drawPips(state, static_cast<uint8_t>(Side::Opponent), 13);
    drawPips(state, static_cast<uint8_t>(Side::Player), 21);
    if (state.feedback != 0) drawDuelFeedback(state);
    else if (duelCursor >= 0) drawDuelChoice(state, duelCursor);
#else
    (void)duelCursor;
#endif
}

} // namespace BattleScene
} // namespace battle
