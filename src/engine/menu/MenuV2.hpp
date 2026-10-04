#pragma once

#include <stdint.h>

#include "../../lib/MenuStack.hpp"
#include "../../lib/uint24.h"
#include "../battle/BattleView.hpp"
#include "MenuIntent.hpp"

namespace battle {
class BattleSession;
}

enum MenuEdgeButton : uint8_t {
    MENU_EDGE_A = static_cast<uint8_t>(1u << 3),
    MENU_EDGE_B = static_cast<uint8_t>(1u << 2),
};

class MenuV2 {
  public:
    MenuEnum stack[6] = {};
    int8_t menuPointer = -1;
    int8_t cursorIndex = 0;
    bool drawMenu = true;
    battle::MoveSnapshot moveSnapshot = {};
    uint24_t moveNameAddresses[4] = {};
    uint24_t creatureNameAddresses[2] = {};
    uint8_t cachedRentalId = 255;
    // Battle move metadata and arena rental name never coexist. Five bytes
    // hold four ten-bit (type, power, physical) move descriptors.
    union {
        uint24_t rentalNameAddress = 0;
        uint8_t moveInfoPacked[5];
    };
    struct RentalStats {
        uint8_t type1, type2;
        uint8_t hpSeed, atkSeed, defSeed;
        uint8_t spcAtkSeed, spcDefSeed, spdSeed;
    } rentalSeed = {};

    MenuV2();

    // Consume one already-edge-qualified button mask. No engine, dialog, or
    // FX state is consulted; the caller owns routing the resulting intent.
    MenuIntent update(uint8_t edgeButtons);

    // Copy the data needed by a battle submenu at its transition boundary.
    // Rendering and subsequent updates consume only these copied values.
    void openMenu(MenuEnum menu, const battle::BattleView &view);
    const battle::MoveSnapshot &movesSnapshot() const;
    void setPartySnapshot(const battle::PartySnapshot &snapshot);
    const battle::PartySnapshot &partySnapshot() const;
    const uint8_t *moveInfo() const;

    void push(MenuEnum type);
    void pop();
    void clear();

    // run keeps the legacy button bridge for existing callers; rendering and
    // snapshot transitions consume only BattleView/snapshot data.
    MenuIntent run(battle::BattleSession &session);
    void printMenu(const battle::BattleView &view);
    void prepareCreatureRental();
    void creatureRental();

  private:
    battle::PartySnapshot partyChoicesSnapshot = {};
};
