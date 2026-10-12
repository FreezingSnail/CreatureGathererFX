#include "BattleFeedback.hpp"
#include "BattleNameCache.hpp"

#include "../../lib/Blit.hpp"
#include "../../lib/ReadData.hpp"

#include <Arduboy2.h>
#ifdef __AVR__
#include "../../fxdata.h"
#else
#include "../../../tst/src/FXDataFake.hpp"
#include "../../fxdata.h"
#endif

namespace battle {
namespace {

constexpr uint8_t captionWidth(uint8_t characters) {
    // The string renderer retains one leading blank cell in each bitmap.
    return static_cast<uint8_t>((characters + 1u) * 5u);
}

void cacheAddress(uint24_t &address, uint8_t &width, uint24_t source,
                  uint8_t sourceWidth) {
    address = 0;
    width = 0;
    if (source == 0 || sourceWidth == 0) return;
    address = source;
    width = sourceWidth;
}

void cacheCaption(uint24_t &address, uint8_t &width, uint24_t source,
                  uint8_t characters) {
    cacheAddress(address, width, source, captionWidth(characters));
}

void cacheMove(uint24_t &address, uint8_t &width, uint8_t move) {
    const uint8_t sourceWidth = readMoveNameWidth(move);
    const uint24_t source = sourceWidth == 0 ? 0 : readMoveNameAddress(move);
    cacheAddress(address, width, source, sourceWidth);
}

void cacheActor(uint24_t &address, uint8_t &width, const ActionResult &result) {
    const uint8_t side = static_cast<uint8_t>(result.actor);
    if (side < 2) cacheCreatureName(address, width, result.speciesBefore[side]);
}

void drawLine(uint24_t address, uint8_t width, uint8_t y) {
    if (address != 0 && width != 0)
        Blit::draw(2, y, width, 8, address, 0, Blit::OVERWRITE);
}

} // namespace

void BattleFeedback::begin(const ActionResult &result, const BattleState &state) {
    reset();

    // A terminal result owns both rows before action-specific captions.
    if (result.outcome != Outcome::None) {
        switch (result.outcome) {
        case Outcome::Win: cacheCaption(lines_[0], widths_[0], battleWin, 3); break;
        case Outcome::Lose: cacheCaption(lines_[0], widths_[0], battleDefeat, 6); break;
        case Outcome::Escaped: cacheCaption(lines_[0], widths_[0], battleEscaped, 7); break;
        case Outcome::Gathered: cacheCaption(lines_[0], widths_[0], battleGathered, 8); break;
        case Outcome::Fled: cacheCaption(lines_[0], widths_[0], battleFled, 4); break;
        case Outcome::None: break;
        }
        return;
    }

    switch (result.kind) {
    case ResultKind::Attack:
        cacheActor(lines_[0], widths_[0], result);
        if (result.flags & SELF_HIT) {
            cacheCaption(lines_[1], widths_[1], battleHurtItself, 11);
        } else if (result.flags & (REFUSED | STATUS_SKIPPED)) {
            cacheCaption(lines_[1], widths_[1], battleCannotAct, 10);
        } else {
            cacheMove(lines_[1], widths_[1], result.index);
        }
        break;
    case ResultKind::Switch: {
        const uint8_t side = static_cast<uint8_t>(result.actor);
        if (side < 2)
            cacheCreatureName(lines_[0], widths_[0], state.active[side].id);
        cacheCaption(lines_[1], widths_[1], battleEnters, 16);
        break;
    }
    case ResultKind::Gather:
        cacheActor(lines_[0], widths_[0], result);
        cacheCaption(lines_[1], widths_[1], gather, 6);
        break;
    case ResultKind::Escape:
        cacheActor(lines_[0], widths_[0], result);
        cacheCaption(lines_[1], widths_[1], battleCannotEscape, 13);
        break;
    case ResultKind::Skip:
        cacheActor(lines_[0], widths_[0], result);
        cacheCaption(lines_[1], widths_[1], battlePass, 4);
        break;
    case ResultKind::EndTurn:
        cacheCaption(lines_[0], widths_[0], battleTurnEnd, 8);
        break;
    case ResultKind::UseItem:
    case ResultKind::None:
    default:
        break;
    }
}

void BattleFeedback::update(bool freshA) {
    if (phase_ != 0) return;
    if (elapsed_ < 26) ++elapsed_;
    if (elapsed_ >= 26 || (freshA && elapsed_ >= 4)) phase_ = 1;
}

bool BattleFeedback::done() const { return phase_ != 0; }

void BattleFeedback::draw() const {
    if (lines_[0] == 0 && lines_[1] == 0) return;
    Blit::fillRect(0, 40, 128, 24, BLACK);
    drawLine(lines_[0], widths_[0], 48);
    drawLine(lines_[1], widths_[1], 56);
}

void BattleFeedback::reset() {
    lines_[0] = lines_[1] = 0;
    widths_[0] = widths_[1] = 0;
    elapsed_ = 0;
    phase_ = 0;
}

static_assert(captionWidth(16) <= 124, "caption fits the feedback row");
static_assert(captionWidth(13) <= 124, "caption fits the feedback row");
static_assert(captionWidth(11) <= 124, "caption fits the feedback row");
static_assert(captionWidth(10) <= 124, "caption fits the feedback row");

} // namespace battle
