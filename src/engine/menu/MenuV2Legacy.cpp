#include "MenuV2.hpp"
#include "MenuNav.hpp"
#include "../battle/BattleSession.hpp"
#include "../draw.h"
#include "../../common.hpp"
#include "../../globals.hpp"

namespace {
uint8_t menuEdgeButtons() {
    uint8_t buttons = 0;
    if (arduboy.justPressed(LEFT_BUTTON)) buttons |= MENU_NAV_LEFT;
    if (arduboy.justPressed(RIGHT_BUTTON)) buttons |= MENU_NAV_RIGHT;
    if (arduboy.justPressed(DOWN_BUTTON)) buttons |= MENU_NAV_DOWN;
    if (arduboy.justPressed(UP_BUTTON)) buttons |= MENU_NAV_UP;
    if (arduboy.justPressed(A_BUTTON)) buttons |= MENU_EDGE_A;
    if (arduboy.justPressed(B_BUTTON)) buttons |= MENU_EDGE_B;
    return buttons;
}
} // namespace

MenuIntent MenuV2::run(battle::BattleSession &session) {
    return run(session, menuEdgeButtons());
}

void MenuV2::printMenu(const battle::BattleView &view) {
    (void)view;
    if (menuPointer < 0) return;
    if (!drawMenu) {
        Blit::draw(0, 40, 128, 24, battleMenu, FRAME(0), Blit::OVERWRITE);
        return;
    }

    switch (stack[menuPointer]) {
    case BATTLE_OPTIONS:
        Blit::draw(0, 40, 128, 24, fightMenu,
                                  FRAME(cursorIndex), Blit::OVERWRITE);
        break;

    case BATTLE_MOVE_SELECT:
        Blit::draw(0, 40, 128, 24, battleMenu, FRAME(0), Blit::OVERWRITE);
        printMoveMenu(cursorIndex, moveSnapshot, moveNameAddresses,
                      moveInfoPacked);
        break;

    case BATTLE_CREATURE_SELECT: {
        const battle::PartySnapshot &party = partySnapshot();
        printCreatureMenu(party, cursorIndex, creatureNameAddresses);
        if (party.count != 0) {
            const uint8_t selected =
                cursorIndex < 0 ? 0 : static_cast<uint8_t>(cursorIndex);
            if (selected < party.count && party.choices[selected].id < 32) {
                Blit::draw(
                    0, 0, 32, 32, NewecreatureSprites,
                    FRAME(static_cast<uint8_t>(party.choices[selected].id * 2)), Blit::PLUSMASK);
            }
        }
        break;
    }

    default:
        break;
    }
    printCursor(cursorIndex);
}
