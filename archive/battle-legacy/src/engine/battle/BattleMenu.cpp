#include "BattleMenu.hpp"
#include "BattleFeedback.hpp"
#include "BattleNameCache.hpp"

#include "../PpGlyph.hpp"
#include "../../lib/Blit.hpp"
#include "../../lib/FxReadCounter.hpp"
#include "../../lib/MoveIds.hpp"
#include "../../lib/ReadData.hpp"
#include "../../lib/Text.hpp"
#ifdef __AVR__
// Keep this identical to the AVR placement-new shim in ModeState.hpp.
inline void *operator new(__SIZE_TYPE__, void *storage) noexcept { return storage; }
inline void operator delete(void *, void *) noexcept {}
#else
#include <new>
#endif
#ifdef __AVR__
#include "../../fxdata.h"
#endif

namespace battle {
namespace {

constexpr uint8_t PAGE_MASK = 0x03;
constexpr uint8_t PAGE_OPTIONS = 0;
constexpr uint8_t PAGE_MOVES = 1;
constexpr uint8_t PAGE_PARTY = 2;
constexpr uint8_t PAGE_REPLACEMENT = 3;
constexpr uint8_t ALLOW_GATHER = 1u << 2;
constexpr uint8_t ALLOW_ESCAPE = 1u << 3;
constexpr uint8_t PASS_SENT = 1u << 4;
constexpr uint8_t PARTY_READY = 1u << 5;
constexpr uint8_t PARTY_FORCED = 1u << 6;
constexpr uint8_t INITIALIZED = 1u << 7;

constexpr uint8_t EDGE_B = 1u << 2;
constexpr uint8_t EDGE_A = 1u << 3;
constexpr uint8_t NAV_DOWN = 1u << 4;
constexpr uint8_t NAV_LEFT = 1u << 5;
constexpr uint8_t NAV_RIGHT = 1u << 6;
constexpr uint8_t NAV_UP = 1u << 7;

constexpr uint8_t MENU_PARTY_SLOT_COUNT = 3;
constexpr uint8_t PARTY_ROWS = 2;
#ifdef CGFX_TRAINER_DEMO_EXPANSION
constexpr uint8_t CREATURE_ID_COUNT = 64;
#else
constexpr uint8_t CREATURE_ID_COUNT = 32;
#endif

MenuIntent noIntent() { return {MenuIntentKind::None, 0}; }
MenuIntent intent(MenuIntentKind kind, uint8_t index = 0) { return {kind, index}; }

#ifdef __AVR__
uint24_t optionsAddress() { return battleOptions48 + 4; }
#endif

bool usableMove(const MoveSnapshot &moves, uint8_t slot) {
    return slot < 4 && validMoveId(moves.moveIds[slot]) &&
           moves.remainingUses[slot] != 0;
}

bool hasUsableMove(const MoveSnapshot &moves) {
    for (uint8_t slot = 0; slot < 4; ++slot)
        if (usableMove(moves, slot)) return true;
    return false;
}

uint8_t lowestUsableMove(const MoveSnapshot &moves) {
    for (uint8_t slot = 0; slot < 4; ++slot)
        if (usableMove(moves, slot)) return slot;
    return 0;
}

} // namespace

BattleMenu::BattleMenu() : payload_{}, cursor_(0), flags_(0) {}

bool BattleMenu::initialized() const {
    return (flags_ & INITIALIZED) != 0;
}

void BattleMenu::beginOptions(const MoveSnapshot &moves, bool allowGather,
                              bool allowEscape) {
    ::new (static_cast<void *>(&payload_.moves)) MovePayload{};
    payload_.moves.moves = moves;
    for (uint8_t slot = 0; slot < 4; ++slot) {
        payload_.moves.names[slot] = 0;
        payload_.moves.widths[slot] = 0;
        const uint8_t id = moves.moveIds[slot];
        if (!validMoveId(id)) continue;
        payload_.moves.widths[slot] = readMoveNameWidth(id);
        if (payload_.moves.widths[slot] == 0) continue;
        payload_.moves.names[slot] = readMoveNameAddress(id);
    }
    cursor_ = 0;
    flags_ = static_cast<uint8_t>(INITIALIZED | PAGE_OPTIONS |
                                  (allowGather ? ALLOW_GATHER : 0) |
                                  (allowEscape ? ALLOW_ESCAPE : 0));
}

void BattleMenu::beginParty(const battle::BattleView &view) {
    ::new (static_cast<void *>(&payload_.party)) PartyPayload{};
    const uint8_t player = static_cast<uint8_t>(battle::Side::Player);
    const uint8_t activeSlot = view.activeSlot[player];
    const uint8_t count = view.partyCount[player] > MENU_PARTY_SLOT_COUNT
                              ? MENU_PARTY_SLOT_COUNT
                              : view.partyCount[player];
    uint8_t reads = 0;
    for (uint8_t slot = 0; slot < count; ++slot) {
        if (slot == activeSlot || payload_.party.count >= PARTY_ROWS) continue;
        const battle::PartySummary &summary = view.party[player][slot];
        if (!summary.alive || summary.hp == 0) continue;
        const uint8_t row = payload_.party.count++;
        payload_.party.slots[row] = slot;
        if (summary.id >= CREATURE_ID_COUNT) continue;
        cacheCreatureName(payload_.party.names[row],
                          payload_.party.widths[row], summary.id);
        ++reads;
    }
    FxReadCounter::transitionExact(reads);
    cursor_ = 0;
    flags_ |= PARTY_READY;
}

bool BattleMenu::partyPending() const {
    return (flags_ & PAGE_MASK) == PAGE_PARTY && !(flags_ & PARTY_READY);
}

#if !CGFX_BATTLE_AUTO_REPLACE
void BattleMenu::beginReplacement(const ReplacementChoices &choices) {
    ::new (static_cast<void *>(&payload_.party)) PartyPayload{};
    payload_.party.count = choices.count > PARTY_ROWS
        ? PARTY_ROWS : choices.count;
    for (uint8_t row = 0; row < PARTY_ROWS; ++row) {
        if (row >= payload_.party.count) continue;
        const uint8_t id = choices.ids[row];
        payload_.party.slots[row] = choices.slots[row];
        if (id >= CREATURE_ID_COUNT) continue;
        cacheCreatureName(payload_.party.names[row], payload_.party.widths[row],
                          id);
    }
    cursor_ = 0;
    flags_ = static_cast<uint8_t>(INITIALIZED | PAGE_REPLACEMENT |
                                  PARTY_READY | PARTY_FORCED);
}
#endif

MenuIntent BattleMenu::update(uint8_t buttons) {
    const uint8_t page = flags_ & PAGE_MASK;

    if (page == PAGE_OPTIONS) {
        uint8_t cursor = cursor_ < 4 ? cursor_ : 0;
        if (buttons & (NAV_LEFT | NAV_RIGHT)) cursor ^= 1;
        if (buttons & (NAV_UP | NAV_DOWN)) cursor ^= 2;
        cursor_ = static_cast<uint8_t>(cursor & 3);
        if (!(buttons & EDGE_A)) return noIntent();
        switch (cursor_) {
        case 0:
            flags_ = static_cast<uint8_t>((flags_ & ~PAGE_MASK) | PAGE_MOVES);
            cursor_ = lowestUsableMove(payload_.moves.moves);
            flags_ &= static_cast<uint8_t>(~PASS_SENT);
            return noIntent();
        case 1:
            return (flags_ & ALLOW_GATHER) ? intent(MenuIntentKind::Gather)
                                           : noIntent();
        case 2:
            flags_ = static_cast<uint8_t>(
                (flags_ & ~(PAGE_MASK | PARTY_READY)) | PAGE_PARTY);
            cursor_ = 0;
            return noIntent();
        case 3:
            return (flags_ & ALLOW_ESCAPE) ? intent(MenuIntentKind::Escape)
                                           : noIntent();
        default:
            return noIntent();
        }
    }

    if (page == PAGE_MOVES) {
        uint8_t direction = 0;
        if (buttons & NAV_RIGHT) direction = NAV_RIGHT;
        else if (buttons & NAV_LEFT) direction = NAV_LEFT;
        else if (buttons & NAV_UP) direction = NAV_UP;
        else if (buttons & NAV_DOWN) direction = NAV_DOWN;
        if (direction) {
            uint8_t candidate = cursor_;
            for (uint8_t scan = 0; scan < 4; ++scan) {
                if (direction == NAV_RIGHT) candidate = (candidate + 1) & 3;
                else if (direction == NAV_LEFT) candidate = (candidate + 3) & 3;
                else candidate ^= 2;
                if (usableMove(payload_.moves.moves, candidate)) {
                    cursor_ = candidate;
                    break;
                }
            }
        }
        if (buttons & EDGE_B) {
            flags_ = static_cast<uint8_t>((flags_ & ~PAGE_MASK) | PAGE_OPTIONS);
            cursor_ = 0;
            flags_ &= static_cast<uint8_t>(~PASS_SENT);
            return noIntent();
        }
        if (!(buttons & EDGE_A)) return noIntent();
        if (!hasUsableMove(payload_.moves.moves)) {
            if (flags_ & PASS_SENT) return noIntent();
            flags_ |= PASS_SENT;
            return intent(MenuIntentKind::Pass);
        }
        if (!usableMove(payload_.moves.moves, cursor_)) return noIntent();
        return intent(MenuIntentKind::SelectMove, cursor_);
    }

    if (page == PAGE_PARTY || page == PAGE_REPLACEMENT) {
        const uint8_t count = payload_.party.count;
        if (count > 1 && (buttons & (NAV_UP | NAV_DOWN))) cursor_ ^= 1;
        if (buttons & EDGE_B) {
            // A forced replacement cannot be cancelled.
            if (page == PAGE_REPLACEMENT) return noIntent();
            flags_ = static_cast<uint8_t>(
                (flags_ & ~(PAGE_MASK | PARTY_READY)) | PAGE_OPTIONS);
            cursor_ = 0;
            return noIntent();
        }
        if (!(buttons & EDGE_A)) return noIntent();
        if (cursor_ < count && payload_.party.widths[cursor_] != 0) {
            const uint8_t originalSlot = payload_.party.slots[cursor_];
            if (page == PAGE_PARTY) {
                flags_ = static_cast<uint8_t>(
                    (flags_ & ~(PAGE_MASK | PARTY_READY)) | PAGE_OPTIONS);
                cursor_ = 0;
            }
            return intent(MenuIntentKind::SelectParty, originalSlot);
        }
        return noIntent();
    }
    return noIntent();
}

void BattleMenu::draw() const {
    constexpr uint16_t selectedFrame = 0;
    constexpr uint16_t normalFrame = 1;
    const uint8_t page = flags_ & PAGE_MASK;

    if (page == PAGE_OPTIONS) {
#ifdef __AVR__
        // Final 1bpp assets use one plane per frame, so the frame index is the
        // cursor value (FRAME(x) == x).
        Blit::draw(0, 48, 128, 16, optionsAddress(), cursor_, Blit::OVERWRITE);
#endif
        return;
    }

    Blit::fillRect(0, 48, 128, 16, WHITE);

    if (page == PAGE_MOVES) {
        const MoveSnapshot &moves = payload_.moves.moves;
        for (uint8_t slot = 0; slot < 4; ++slot) {
            if (!validMoveId(moves.moveIds[slot]) ||
                payload_.moves.widths[slot] == 0) continue;
            const uint8_t x = (slot & 1) ? 68 : 4;
            const uint8_t y = slot < 2 ? 48 : 56;
            const uint8_t cellX = (slot & 1) ? 64 : 0;
            if (slot == cursor_) Blit::fillRect(cellX, y, 64, 8, BLACK);
            drawText(x, y, payload_.moves.names[slot], payload_.moves.widths[slot],
                     slot == cursor_ ? selectedFrame : normalFrame);
        }
        if (!hasUsableMove(moves)) {
            Blit::fillRect(48, 40, 32, 8, WHITE);
            drawText(51, 41, BattleFeedbackText::passAddress(),
                     BattleFeedbackText::passWidth, normalFrame);
        } else if (cursor_ < 4) {
            Blit::fillRect(48, 40, 32, 8, WHITE);
            PpGlyph::draw(50, 41, 0b001001111101111);
            PpGlyph::draw(54, 41, 0b001001111101111);
            const uint8_t remaining = moves.remainingUses[cursor_];
            if (remaining == 255) {
                PpGlyph::draw(66, 42, 0b010111010);
            } else if (remaining < 10) {
                PpGlyph::digit(61, 41, remaining);
                PpGlyph::draw(65, 41, 0b001001010100100);
                PpGlyph::digit(69, 41,
                    (moves.useLimitsPacked >> (cursor_ * 2)) & 3);
            }
        }
        return;
    }

    if (page == PAGE_PARTY || page == PAGE_REPLACEMENT) {
        for (uint8_t row = 0; row < payload_.party.count; ++row) {
            if (payload_.party.widths[row] == 0) continue;
            const uint8_t y = row == 0 ? 48 : 56;
            if (cursor_ == row) Blit::fillRect(0, y, 128, 8, BLACK);
            drawText(4, y, payload_.party.names[row],
                     payload_.party.widths[row],
                     cursor_ == row ? selectedFrame : normalFrame);
        }
    }
}

} // namespace battle
