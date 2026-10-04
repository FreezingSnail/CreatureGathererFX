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
    if (menuPointer < 0 && !dialogMenu.peek()) return {MenuIntentKind::None, 0};

    // World/script dialogs retain their separate A dismiss owner. Battle
    // playback does not enter this compatibility path or enqueue dialogs.
    if (dialogMenu.peek()) {
        if (arduboy.justPressed(A_BUTTON)) dialogMenu.popMenu();
        return {MenuIntentKind::None, 0};
    }

    const int8_t previousPointer = menuPointer;
    const MenuIntent result = update(menuEdgeButtons());
    // update() owns cursor/intent state. When it opens a battle submenu, this
    // compatibility boundary supplies the view and resolves transition data;
    // steady update calls never revisit battle memory or FX tables.
    if (menuPointer > previousPointer) {
        const MenuEnum current = stack[menuPointer];
        if (current == BATTLE_MOVE_SELECT || current == BATTLE_CREATURE_SELECT) {
            openMenu(current, session.view());
        }
    }
    return result;
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

void MenuV2::prepareCreatureRental() {
    if (cursorIndex > 30) {
        cursorIndex = 0;
    } else if (cursorIndex < 0) {
        cursorIndex = 30;
    }
    if (cachedRentalId == static_cast<uint8_t>(cursorIndex)) return;
    cachedRentalId = static_cast<uint8_t>(cursorIndex);
    rentalNameAddress = readCreatureNameAddress(cachedRentalId);
    const CreatureData_t seed = getCreatureFromStore(cachedRentalId);
    rentalSeed = {seed.type1, seed.type2, seed.hpSeed, seed.atkSeed,
                  seed.defSeed, seed.spcAtkSeed, seed.spcDefSeed, seed.spdSeed};
    FxReadCounter::transitionExact(2);
}

void MenuV2::creatureRental() {
    FX::setCursor(10, 55);
    if (cachedRentalId != static_cast<uint8_t>(cursorIndex)) return;
    drawText(0, 55, rentalNameAddress,
                     readCreatureNameWidth(cachedRentalId), FRAME(0));
    Blit::draw(0, 0, 32, 32, NewecreatureSprites,
                             FRAME(cursorIndex * 2), Blit::PLUSMASK);

    const RentalStats &seed = rentalSeed;
    drawText(35, 0, hpText, 20, FRAME(0));
    drawStatNumbers(60, 0, seed.hpSeed);
    drawText(35, 10, atkText, 25, FRAME(0));
    drawStatNumbers(60, 10, seed.atkSeed);
    drawText(35, 20, defText, 25, FRAME(0));
    drawStatNumbers(60, 20, seed.defSeed);
    drawText(72, 0, satkText, 30, FRAME(0));
    drawStatNumbers(103, 0, seed.spcAtkSeed);
    drawText(72, 10, sdefText, 30, FRAME(0));
    drawStatNumbers(103, 10, seed.spcDefSeed);
    drawText(72, 20, spdText, 25, FRAME(0));
    drawStatNumbers(103, 20, seed.spdSeed);
    printType(Type(seed.type1), 0, 35);
    printType(Type(seed.type2), 0, 45);
}
