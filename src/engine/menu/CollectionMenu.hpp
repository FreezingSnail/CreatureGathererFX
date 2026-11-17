#pragma once

#include <stdint.h>

#include "../../item/Inventory.hpp"
#include "../../lib/ListView.hpp"
#include "../../lib/uint24.h"

class Player;

// A four-row collection page over the stable 64-slot store: the ListView
// window plus the cached facts for exactly those rows. Occupied slots are
// discovered by one bounded store scan on open/scroll; draw and steady update
// read only this cache. The world menu instance lives in file statics.
struct CollectionMenu {
    // AVR address bytes first so the byte fields pack without padding.
    uint24_t rowName[4];
    ListView view;
    uint8_t rowSlot[4];
    uint8_t rowId[4];
    uint8_t rowLevel[4];
    uint8_t rowWidth[4];
    uint8_t rowParty;  // bit N marks visible row N as a party instance
    uint8_t page;
    uint8_t rootCursor;
    uint8_t slotCursor;
    uint8_t pendingSlot;
    uint8_t choiceSlot;
    uint8_t choiceParty;
    uint8_t feedback;
};

// Root: BAG, COLLECTION, SAVE. A on SAVE closes the menu and starts the
// explicit WORLD checkpoint; a failure stays on the save screen for a fresh
// A retry or B return.
enum class CollectionPage : uint8_t { Closed, Root, Bag, List, Party };

// A confirmed A press on the party-slot page. slot is the chosen instance's
// stable store slot (255 when nothing was confirmed); partySlot is 1..3.
// Reassignment and HP policy belong to jp8.11.9.
struct CollectionChoice {
    uint8_t slot;
    uint8_t partySlot;
};

// Result of consuming a confirmed choice. SaveNeeded is the explicit
// "save first" refusal: the outgoing HP mirror or the assignment could not be
// published, so the live party was left unchanged.
enum class CollectionFeedback : uint8_t { None, InParty, SaveNeeded };

#ifdef __AVR__
static_assert(sizeof(CollectionMenu) == 40, "collection menu cache must remain 40 bytes");
#endif

void collectionMenuOpen(CollectionMenu &m);
void collectionMenuMove(CollectionMenu &m, int8_t delta);
// Stable store slot of the selected visible row, or 255 while the collection
// is empty. Intents never carry a row number or species id.
uint8_t collectionMenuSelect(const CollectionMenu &m);
void drawCollectionMenu(const CollectionMenu &m);

// --- World root menu and collection page ----------------------------------
// Fresh-edge bits match the world picker's edge encoding (up, down, A, B).
bool collectionMenuWorldInput(uint8_t edges, bool worldIdle, const item::Inventory &inv);
bool collectionMenuWorldOpen();
void collectionMenuWorldDraw(const item::Inventory &inv);
CollectionPage collectionMenuWorldPage();
CollectionChoice collectionMenuWorldChoice();
// Consume a choice confirmed on the party-slot page. Runs only from the WORLD
// update: an already-assigned instance reports InParty without mutation;
// otherwise the outgoing member's current HP is mirrored to its own record
// first, then the selected record is assigned with its encoded HP, and only
// after the assignment is published is the one live party member rebuilt.
// A refusal leaves the live party/HP untouched and reports SaveNeeded.
bool collectionMenuWorldApplyChoice(Player &player);
CollectionFeedback collectionMenuWorldFeedback();
