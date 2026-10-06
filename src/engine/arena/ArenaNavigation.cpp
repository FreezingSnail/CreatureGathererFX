#include "ArenaNavigation.hpp"

#include "../menu/MenuV2.hpp"

namespace arena {
namespace {

uint8_t countFor(ArenaScreen screen, uint8_t playerCount,
                 uint8_t opponentCount)
{
    switch (screen) {
    case ArenaScreen::PlayerTeam: return playerCount;
    case ArenaScreen::OpponentTeam: return opponentCount;
    case ArenaScreen::Result: return 3;
    }
    return 0;
}

uint8_t clampIndex(uint8_t index, uint8_t count)
{
    return count == 0 || index >= count ? 0 : index;
}

void setScreen(ArenaUiState &ui, ArenaScreen screen, uint8_t cursor,
               uint8_t playerCount, uint8_t opponentCount)
{
    ui.screen = screen;
    const uint8_t count = countFor(screen, playerCount, opponentCount);
    ui.list.itemCount = count;
    ui.list.rows = screen == ArenaScreen::Result ? 3 : 1;
    ui.list.cursor = clampIndex(cursor, count);
    ui.list.windowStart = screen == ArenaScreen::Result ? 0 : ui.list.cursor;
}

} // namespace

ArenaIntent navigate(ArenaUiState &ui, ArenaContext &context,
                     uint8_t edgeButtons, uint8_t playerCount,
                     uint8_t opponentCount)
{
    // B > A > Up > Down. Left and Right have no arena navigation action.
    if (edgeButtons & MENU_EDGE_B) {
        if (ui.screen == ArenaScreen::OpponentTeam) {
            setScreen(ui, ArenaScreen::PlayerTeam, context.playerTeam,
                      playerCount, opponentCount);
            return playerCount == 0 ? ArenaIntent::None
                                    : ArenaIntent::PreviewChanged;
        }
        if (ui.screen == ArenaScreen::Result) {
            if (opponentCount == 0) return ArenaIntent::None;
            setScreen(ui, ArenaScreen::OpponentTeam, context.opponentTeam,
                      playerCount, opponentCount);
            return ArenaIntent::PreviewChanged;
        }
        return ArenaIntent::None;
    }

    if (edgeButtons & MENU_EDGE_A) {
        if (ui.screen == ArenaScreen::PlayerTeam) {
            if (playerCount == 0 || opponentCount == 0) {
                setScreen(ui, ArenaScreen::PlayerTeam, 0, playerCount,
                          opponentCount);
                return ArenaIntent::None;
            }
            const uint8_t selectedTeam = clampIndex(ui.list.cursor, playerCount);
            setScreen(ui, ArenaScreen::OpponentTeam, context.opponentTeam,
                      playerCount, opponentCount);
            context.playerTeam = selectedTeam;
            return ArenaIntent::PreviewChanged;
        }
        if (ui.screen == ArenaScreen::OpponentTeam) {
            if (opponentCount == 0 || playerCount == 0) {
                setScreen(ui, ArenaScreen::OpponentTeam, 0, playerCount,
                          opponentCount);
                return ArenaIntent::None;
            }
            context.opponentTeam = clampIndex(ui.list.cursor, opponentCount);
            return ArenaIntent::StartBattle;
        }

        switch (ui.list.cursor) {
        case 0:
            if (playerCount == 0 || opponentCount == 0)
                return ArenaIntent::None;
            return ArenaIntent::StartBattle;
        case 1:
            if (opponentCount == 0) return ArenaIntent::None;
            setScreen(ui, ArenaScreen::OpponentTeam, context.opponentTeam,
                      playerCount, opponentCount);
            return ArenaIntent::PreviewChanged;
        default:
            if (playerCount == 0) return ArenaIntent::None;
            setScreen(ui, ArenaScreen::PlayerTeam, context.playerTeam,
                      playerCount, opponentCount);
            return ArenaIntent::PreviewChanged;
        }
    }

    const uint8_t direction = (edgeButtons & MENU_NAV_UP) ? 0
                              : (edgeButtons & MENU_NAV_DOWN) ? 1
                              : 0xff;
    if (direction == 0xff) return ArenaIntent::None;

    const uint8_t count = countFor(ui.screen, playerCount, opponentCount);
    if (count == 0) {
        ui.list = {0, static_cast<uint8_t>(ui.screen == ArenaScreen::Result ? 3 : 1), 0, 0};
        return ArenaIntent::None;
    }

    ui.list.itemCount = count;
    ui.list.rows = ui.screen == ArenaScreen::Result ? 3 : 1;
    ui.list.cursor = clampIndex(ui.list.cursor, count);
    if (ui.list.rows >= count) ui.list.windowStart = 0;
    else if (ui.list.windowStart > ui.list.cursor ||
             ui.list.windowStart + ui.list.rows <= ui.list.cursor)
        ui.list.windowStart = ui.list.cursor;
    const uint8_t oldCursor = ui.list.cursor;
    listViewMove(ui.list, direction == 0 ? -1 : 1);
    if (ui.list.cursor == oldCursor) return ArenaIntent::None;
    return ui.screen == ArenaScreen::Result ? ArenaIntent::None
                                            : ArenaIntent::PreviewChanged;
}

} // namespace arena
