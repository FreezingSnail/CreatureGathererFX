#include "BattleMenu.hpp"
#include "BattleFeedback.hpp"
#include "BattleNameCache.hpp"

#include "../PpGlyph.hpp"
#include "../../lib/Blit.hpp"
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
constexpr uint8_t PAGE_MOVES = 0;
constexpr uint8_t PAGE_ACTIONS = 1;
#if !CGFX_BATTLE_AUTO_REPLACE
constexpr uint8_t PAGE_REPLACEMENT = 2;
#endif
constexpr uint8_t PAGE_UNINITIALIZED = 3;
constexpr uint8_t ALLOW_GATHER = 1u << 2;
constexpr uint8_t ALLOW_ESCAPE = 1u << 3;
constexpr uint8_t PASS_SENT = 1u << 4;

constexpr uint8_t EDGE_B = 1u << 2;
constexpr uint8_t EDGE_A = 1u << 3;
constexpr uint8_t NAV_DOWN = 1u << 4;
constexpr uint8_t NAV_LEFT = 1u << 5;
constexpr uint8_t NAV_RIGHT = 1u << 6;
constexpr uint8_t NAV_UP = 1u << 7;

#if !CGFX_BATTLE_AUTO_REPLACE
constexpr uint8_t PARTY_ROWS = 2;
#ifdef CGFX_TRAINER_DEMO_EXPANSION
constexpr uint8_t CREATURE_ID_COUNT = 64;
#else
constexpr uint8_t CREATURE_ID_COUNT = 32;
#endif
#endif

MenuIntent noIntent() { return {MenuIntentKind::None, 0}; }
MenuIntent intent(MenuIntentKind kind, uint8_t index = 0) { return {kind, index}; }

// Six-glyph generated labels carry one leading blank slot: 7 cells * 5 px.
constexpr uint8_t ACTION_LABEL_WIDTH = 35;

uint24_t gatherAddress() {
#ifdef __AVR__
    return gather;
#else
    return 0;
#endif
}

uint24_t escapeAddress() {
#ifdef __AVR__
    return escape;
#else
    return 0;
#endif
}

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

BattleMenu::BattleMenu() : payload_{}, cursor_(0), flags_(PAGE_UNINITIALIZED) {}

bool BattleMenu::initialized() const {
    return (flags_ & PAGE_MASK) != PAGE_UNINITIALIZED;
}

void BattleMenu::beginMoves(const MoveSnapshot &moves, bool allowGather,
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
    cursor_ = lowestUsableMove(moves);
    flags_ = PAGE_MOVES | (allowGather ? ALLOW_GATHER : 0) |
             (allowEscape ? ALLOW_ESCAPE : 0);
}

#if !CGFX_BATTLE_AUTO_REPLACE
void BattleMenu::beginReplacement(const ReplacementChoices &choices) {
    ::new (static_cast<void *>(&payload_.replacement)) ReplacementPayload{};
    payload_.replacement.count = choices.count > PARTY_ROWS
        ? PARTY_ROWS : choices.count;
    for (uint8_t row = 0; row < PARTY_ROWS; ++row) {
        if (row >= payload_.replacement.count) continue;
        const uint8_t id = choices.ids[row];
        payload_.replacement.slots[row] = choices.slots[row];
        if (id >= CREATURE_ID_COUNT) continue;
        cacheCreatureName(payload_.replacement.names[row],
                          payload_.replacement.widths[row], id);
    }
    cursor_ = 0;
    flags_ = PAGE_REPLACEMENT;
}
#endif

MenuIntent BattleMenu::update(uint8_t buttons) {
    const uint8_t page = flags_ & PAGE_MASK;
#if !CGFX_BATTLE_AUTO_REPLACE
    if (page == PAGE_REPLACEMENT) {
        const uint8_t count = payload_.replacement.count;
        if (count > 1 && (buttons & (NAV_UP | NAV_DOWN))) cursor_ ^= 1;
        // The session is paused while this filtered replacement page is open.
        if ((buttons & EDGE_A) && cursor_ < count &&
            payload_.replacement.widths[cursor_] != 0)
            return intent(MenuIntentKind::SelectParty,
                          payload_.replacement.slots[cursor_]);
        return noIntent();
    }
#endif

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
            flags_ = static_cast<uint8_t>((flags_ & ~PAGE_MASK) | PAGE_ACTIONS);
            cursor_ = (flags_ & ALLOW_GATHER) ? 0 : 1;
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

    if (page == PAGE_ACTIONS) {
        if (buttons & (NAV_LEFT | NAV_RIGHT)) cursor_ ^= 1;
        if (buttons & EDGE_B) {
            flags_ = static_cast<uint8_t>((flags_ & ~PAGE_MASK) | PAGE_MOVES);
            flags_ &= static_cast<uint8_t>(~PASS_SENT);
            return noIntent();
        }
        if (!(buttons & EDGE_A)) return noIntent();
        if (cursor_ == 0 && (flags_ & ALLOW_GATHER))
            return intent(MenuIntentKind::Gather);
        if (cursor_ == 1 && (flags_ & ALLOW_ESCAPE))
            return intent(MenuIntentKind::Escape);
    }
    return noIntent();
}

void BattleMenu::draw() const {
    constexpr uint16_t selectedFrame = 0;
    constexpr uint16_t normalFrame = 1;
    const uint8_t page = flags_ & PAGE_MASK;
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

    if (page == PAGE_ACTIONS) {
        const bool gather = (flags_ & ALLOW_GATHER) != 0;
        const bool escape = (flags_ & ALLOW_ESCAPE) != 0;
        if (cursor_ == 0 && gather) Blit::fillRect(0, 56, 64, 8, BLACK);
        if (cursor_ == 1 && escape) Blit::fillRect(64, 56, 64, 8, BLACK);
        drawText(4, 56, gatherAddress(), ACTION_LABEL_WIDTH,
                 cursor_ == 0 && gather ? selectedFrame : normalFrame);
        drawText(68, 56, escapeAddress(), ACTION_LABEL_WIDTH,
                 cursor_ == 1 && escape ? selectedFrame : normalFrame);
        return;
    }

#if !CGFX_BATTLE_AUTO_REPLACE
    if (page == PAGE_REPLACEMENT) {
        for (uint8_t row = 0; row < payload_.replacement.count; ++row) {
            if (payload_.replacement.widths[row] == 0) continue;
            const uint8_t y = row == 0 ? 48 : 56;
            if (cursor_ == row) Blit::fillRect(0, y, 128, 8, BLACK);
            drawText(4, y, payload_.replacement.names[row],
                     payload_.replacement.widths[row],
                     cursor_ == row ? selectedFrame : normalFrame);
        }
    }
#endif
}

} // namespace battle
