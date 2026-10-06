#include "ArenaView.hpp"

#include "../../common.hpp"
#include "../../lib/Text.hpp"

namespace {

void drawCounter(uint8_t current, uint8_t count)
{
    // The single digit asset is the same renderer used by the battle HUD.
    Blit::draw(96, 10, 3, 8, singlenumberswhite, FRAME(current), Blit::PLUSMASK);
    arduboy.drawPixel(100, 12, WHITE);
    arduboy.drawPixel(101, 14, WHITE);
    arduboy.drawPixel(102, 16, WHITE);
    Blit::draw(105, 10, 3, 8, singlenumberswhite, FRAME(count), Blit::PLUSMASK);
}

void drawControls()
{
    drawText(0, 56, arenaControls, 65, FRAME(0));
}

void drawSelection(const arena::ArenaUiState &ui, bool players)
{
    drawText(0, 0, players ? arenaChooseTeam : arenaChooseOpponent,
             players ? 55 : 75, FRAME(0));
    drawText(0, 10, ui.preview.labelAddress, ui.preview.labelWidth, FRAME(0));
    for (uint8_t slot = 0; slot < 3; ++slot)
        drawText(0, static_cast<int16_t>(22 + slot * 10),
                 ui.preview.nameAddress[slot], ui.preview.nameWidth[slot],
                 FRAME(0));
    drawCounter(static_cast<uint8_t>(ui.list.cursor + 1), ui.list.itemCount);
    drawControls();
}

} // namespace

namespace arena {

void draw(const ArenaUiState &ui)
{
    arduboy.clear();
    if (ui.screen == ArenaScreen::PlayerTeam)
        drawSelection(ui, true);
    else if (ui.screen == ArenaScreen::OpponentTeam)
        drawSelection(ui, false);
    else
        drawSelection(ui, false);
}

} // namespace arena
