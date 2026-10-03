#include "MenuV2.hpp"

#include "MenuNav.hpp"

namespace {
constexpr uint8_t MENU_PARTY_SLOT_COUNT = 3;
constexpr uint8_t MENU_PARTY_CHOICE_COUNT = 2;

MenuIntent noIntent() {
    return {MenuIntentKind::None, 0};
}

MenuIntent intent(MenuIntentKind kind, uint8_t index = 0) {
    return {kind, index};
}

bool isPartyMenu(MenuEnum menu) {
    return menu == BATTLE_CREATURE_SELECT;
}

bool isVoluntarySubmenu(MenuEnum menu) {
    return menu == BATTLE_MOVE_SELECT || menu == BATTLE_CREATURE_SELECT;
}

uint8_t partyCursor(uint8_t cursor, uint8_t count, uint8_t buttons) {
    if (count == 0) return 0;
    if (cursor >= count) cursor = static_cast<uint8_t>(count - 1);
    if ((buttons & MENU_NAV_UP) && cursor > 0) --cursor;
    if ((buttons & MENU_NAV_DOWN) && cursor + 1 < count) ++cursor;
    return cursor;
}

bool validPartyChoice(const battle::PartySnapshot &snapshot, uint8_t cursor) {
    if (snapshot.count > MENU_PARTY_CHOICE_COUNT || cursor >= snapshot.count) {
        return false;
    }
    const battle::PartyChoice &choice = snapshot.choices[cursor];
    // Species zero is valid. HP zero is the dead sentinel; slots outside the
    // three original party positions cannot be dispatched to BattleSession.
    return choice.hp != 0 && choice.slot < MENU_PARTY_SLOT_COUNT;
}
} // namespace

MenuV2::MenuV2() {
    menuPointer = -1;
    cursorIndex = 0;
}

void MenuV2::setPartySnapshot(const battle::PartySnapshot &snapshot) {
    partyChoicesSnapshot.choices[0] = snapshot.choices[0];
    partyChoicesSnapshot.choices[1] = snapshot.choices[1];
    partyChoicesSnapshot.count = snapshot.count > MENU_PARTY_CHOICE_COUNT
                                     ? MENU_PARTY_CHOICE_COUNT
                                     : snapshot.count;
    partyChoicesSnapshot.forced = snapshot.forced;
}

const battle::PartySnapshot &MenuV2::partySnapshot() const {
    return partyChoicesSnapshot;
}

void MenuV2::push(MenuEnum type) {
    if (menuPointer >= 5) return;
    ++menuPointer;
    stack[menuPointer] = type;
    cursorIndex = 0;
}

void MenuV2::pop() {
    if (menuPointer < 0) return;
    --menuPointer;
    cursorIndex = 0;
}

void MenuV2::clear() {
    menuPointer = -1;
    cursorIndex = 0;
    cachedNameMode = 255;
    partyChoicesSnapshot = {};
}

MenuIntent MenuV2::update(uint8_t edgeButtons) {
    if (menuPointer < 0) return noIntent();

    const MenuEnum current = stack[menuPointer];
    const uint8_t navigation = static_cast<uint8_t>(
        edgeButtons & (MENU_NAV_LEFT | MENU_NAV_RIGHT | MENU_NAV_UP |
                       MENU_NAV_DOWN));
    uint8_t cursor = cursorIndex < 0 ? 0 : static_cast<uint8_t>(cursorIndex);

    if (isPartyMenu(current)) {
        if (navigation != 0) {
            cursor = partyCursor(cursor, partyChoicesSnapshot.count, navigation);
        }
    } else {
        cursor = menuNavMove(menuDescFor(current), cursor, navigation);
    }
    cursorIndex = static_cast<int8_t>(cursor);

    if ((edgeButtons & MENU_EDGE_B) != 0) {
        if (current == BATTLE_CREATURE_SELECT && partyChoicesSnapshot.forced) {
            return noIntent();
        }
        if (isVoluntarySubmenu(current)) {
            pop();
            return intent(MenuIntentKind::Back);
        }
        return noIntent();
    }

    if ((edgeButtons & MENU_EDGE_A) == 0) return noIntent();

    switch (current) {
    case BATTLE_OPTIONS:
        switch (cursor) {
        case 0:
            push(BATTLE_MOVE_SELECT);
            return noIntent();
        case 1:
            return intent(MenuIntentKind::Gather);
        case 2:
            push(BATTLE_CREATURE_SELECT);
            return noIntent();
        case 3:
            return intent(MenuIntentKind::Escape);
        default:
            return noIntent();
        }

    case BATTLE_MOVE_SELECT:
        if (cursor >= 4) return noIntent();
        pop();
        return intent(MenuIntentKind::SelectMove, cursor);

    case BATTLE_CREATURE_SELECT:
        if (!validPartyChoice(partyChoicesSnapshot, cursor)) {
            return noIntent();
        }
        {
            const uint8_t originalSlot =
                partyChoicesSnapshot.choices[cursor].slot;
            pop();
            return intent(MenuIntentKind::SelectParty, originalSlot);
        }

    case WORLD_OPTIONS:
    case ARENA_MENU:
    default:
        return noIntent();
    }
}
