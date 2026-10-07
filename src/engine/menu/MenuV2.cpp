#include "MenuV2.hpp"

#include "MenuNav.hpp"
#include "PackedMoveInfo.hpp"
#include "../../lib/MoveIds.hpp"
#include "DialogMenu.hpp"
#include "../battle/BattleSession.hpp"
#include "../../lib/FxReadCounter.hpp"
#include "../../lib/ReadData.hpp"

extern DialogMenu dialogMenu;

namespace {
constexpr uint8_t MENU_PARTY_SLOT_COUNT = 3;
constexpr uint8_t MENU_PARTY_CHOICE_COUNT = 2;
constexpr uint8_t MENU_MOVE_COUNT = 4;
constexpr uint8_t MOVE_INFO_BYTES = PackedMoveInfo::BYTE_COUNT;
constexpr uint8_t MOVE_ID_EMPTY = 32;
constexpr uint8_t MOVE_ID_ABSENT = 255;
#ifdef CGFX_TRAINER_DEMO_EXPANSION
constexpr uint8_t CREATURE_ID_COUNT = 64;
#else
constexpr uint8_t CREATURE_ID_COUNT = 32;
#endif

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
    // Species zero is valid. IDs outside the creature table, HP zero, and
    // slots outside the original party are dead/invalid sentinels.
    return choice.id < CREATURE_ID_COUNT && choice.hp != 0 &&
           choice.slot < MENU_PARTY_SLOT_COUNT;
}

uint16_t compactMoveInfo(const Move &move) {
    return static_cast<uint16_t>(
        ((static_cast<uint16_t>(move.getMoveType()) & 0x0f) << 6) |
        ((static_cast<uint16_t>(move.getMovePower()) & 0x1f) << 1) |
        (move.isPhysical() ? 1u : 0u));
}

void storeMoveInfo(uint8_t *packed, uint8_t slot, uint16_t info) {
    PackedMoveInfo::write(packed, slot, info);
}
} // namespace

MenuV2::MenuV2() {
    menuPointer = -1;
    cursorIndex = 0;
}

void MenuV2::openMenu(MenuEnum menu, const battle::BattleView &view) {
    if (menuPointer < 0 || stack[menuPointer] != menu) {
        if (menuPointer >= 5) return;
        push(menu);
    }

    if (menu == BATTLE_MOVE_SELECT) {
        moveSnapshot = {};
        for (uint8_t slot = 0; slot < MENU_MOVE_COUNT; ++slot) {
            moveSnapshot.moveIds[slot] = view.moveIds[slot];
            moveSnapshot.remainingUses[slot] = view.remainingUses[slot];
            moveNameAddresses[slot] = 0;
        }
        for (uint8_t byte = 0; byte < MOVE_INFO_BYTES; ++byte) {
            moveInfoPacked[byte] = 0;
        }
        moveSnapshot.useLimitsPacked = view.useLimitsPacked;
        uint8_t reads = 0;
        for (uint8_t slot = 0; slot < MENU_MOVE_COUNT; ++slot) {
            const uint8_t id = moveSnapshot.moveIds[slot];
            if (!validMoveId(id)) {
                continue;
            }
            moveNameAddresses[slot] = readMoveNameAddress(id);
            storeMoveInfo(moveInfoPacked, slot, compactMoveInfo(readMoveFX(id)));
            reads = static_cast<uint8_t>(reads + 2);
        }
        FxReadCounter::transitionExact(reads);
        return;
    }

    if (menu == BATTLE_CREATURE_SELECT) {
        partyChoicesSnapshot = {};
        creatureNameAddresses[0] = 0;
        creatureNameAddresses[1] = 0;
        const uint8_t player = static_cast<uint8_t>(battle::Side::Player);
        const uint8_t activeSlot = view.activeSlot[player];
        const uint8_t count = view.partyCount[player] > MENU_PARTY_SLOT_COUNT
                                  ? MENU_PARTY_SLOT_COUNT
                                  : view.partyCount[player];
        uint8_t reads = 0;
        for (uint8_t slot = 0; slot < count; ++slot) {
            if (slot == activeSlot ||
                partyChoicesSnapshot.count >= MENU_PARTY_CHOICE_COUNT) {
                continue;
            }
            const battle::PartySummary &summary = view.party[player][slot];
            const uint8_t choice = partyChoicesSnapshot.count++;
            // PartySummary::alive is authoritative at the view boundary. Keep
            // the original HP only for live choices; update() rejects zero.
            const uint8_t hp = summary.alive ? summary.hp : 0;
            partyChoicesSnapshot.choices[choice] = {summary.id, slot, hp};
            creatureNameAddresses[choice] = 0;
            if (summary.alive && summary.hp != 0 &&
                summary.id < CREATURE_ID_COUNT) {
                creatureNameAddresses[choice] =
                    readCreatureNameAddress(summary.id);
                ++reads;
            }
        }
        FxReadCounter::transitionExact(reads);
    }
}

const battle::MoveSnapshot &MenuV2::movesSnapshot() const {
    return moveSnapshot;
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

const uint8_t *MenuV2::moveInfo() const {
    return moveInfoPacked;
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
    moveSnapshot = {};
    for (uint8_t slot = 0; slot < MENU_MOVE_COUNT; ++slot) {
        moveNameAddresses[slot] = 0;
    }
    for (uint8_t byte = 0; byte < MOVE_INFO_BYTES; ++byte) {
        moveInfoPacked[byte] = 0;
    }
    partyChoicesSnapshot = {};
    creatureNameAddresses[0] = 0;
    creatureNameAddresses[1] = 0;
}

MenuIntent MenuV2::run(battle::BattleSession &session, uint8_t edgeButtons) {
    if (menuPointer < 0 && !dialogMenu.peek()) return noIntent();
    if (dialogMenu.peek()) {
        if ((edgeButtons & MENU_EDGE_A) != 0) dialogMenu.popMenu();
        return noIntent();
    }

    const int8_t previousPointer = menuPointer;
    const MenuIntent result = update(edgeButtons);
    if (menuPointer > previousPointer) {
        const MenuEnum current = stack[menuPointer];
        if (current == BATTLE_MOVE_SELECT || current == BATTLE_CREATURE_SELECT) {
            openMenu(current, session.view());
        }
    }
    return result;
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
        {
            bool available = false;
            for (uint8_t slot = 0; slot < 4; ++slot)
                if (moveSnapshot.remainingUses[slot]) available = true;
            if (!available) { pop(); return intent(MenuIntentKind::Pass); }
        }
        if (cursor >= 4 || !validMoveId(moveSnapshot.moveIds[cursor]) ||
            moveSnapshot.remainingUses[cursor] == 0) return noIntent();
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
    default:
        return noIntent();
    }
}
