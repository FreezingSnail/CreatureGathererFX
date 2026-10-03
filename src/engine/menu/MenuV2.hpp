#pragma once

#include <stdint.h>

#include "../../lib/MenuStack.hpp"
#include "../../lib/uint24.h"
#include "../battle/BattleView.hpp"
#include "MenuIntent.hpp"

class BattleEngine;

enum MenuEdgeButton : uint8_t {
    MENU_EDGE_A = static_cast<uint8_t>(1u << 3),
    MENU_EDGE_B = static_cast<uint8_t>(1u << 2),
};

class MenuV2 {
  public:
    MenuEnum stack[6] = {};
    int8_t menuPointer = -1;
    int8_t cursorIndex = 0;
    uint8_t *moveList = nullptr;
    uint8_t creatures[2] = {};
    bool drawMenu = true;
    uint24_t moveNameAddresses[4] = {};
    uint8_t moveNameIds[4] = {255, 255, 255, 255};
    uint8_t cachedNameMode = 255;
    uint24_t creatureNameAddresses[2] = {};
    uint8_t creatureNameIds[2] = {255, 255};
    uint8_t cachedRentalId = 255;
    uint24_t rentalNameAddress = 0;
    struct RentalStats {
        uint8_t type1, type2;
        uint8_t hpSeed, atkSeed, defSeed;
        uint8_t spcAtkSeed, spcDefSeed, spdSeed;
    } rentalSeed = {};

    MenuV2();

    // Consume one already-edge-qualified button mask. No engine, dialog, or
    // FX state is consulted; the caller owns routing the resulting intent.
    MenuIntent update(uint8_t edgeButtons);
    void setPartySnapshot(const battle::PartySnapshot &snapshot);
    const battle::PartySnapshot &partySnapshot() const;

    void push(MenuEnum type);
    void pop();
    void clear();

    // Legacy render/arena entry points remain until their owning menu beads
    // port them to snapshots. They do not participate in update().
    void run(BattleEngine &engine);
    void printMenu(BattleEngine &engine);
    void prepareCreatureRental();
    void creatureRental();

  private:
    battle::PartySnapshot partyChoicesSnapshot = {};
};
