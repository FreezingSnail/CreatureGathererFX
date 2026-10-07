#include "../../lib/Text.hpp"
#include "BattlePresenter.hpp"
#include "../../lib/FxReadCounter.hpp"
#include "../../lib/uint24.h"
#ifdef TEST
#include "../../../tst/src/FXDataFake.hpp"
#include "../../fxdata.h"
#else
#include "../../common.hpp"
#endif

uint24_t readCreatureNameAddress(uint8_t id);
uint24_t readMoveNameAddress(uint16_t id);
uint8_t readCreatureNameWidth(uint8_t id);
uint8_t readMoveNameWidth(uint16_t id);

namespace battle {
namespace {
// The first eight impact ticks use a deterministic 2-pixel cross shake.
// Deriving offsets from stage time keeps state bounded and guarantees settling.
void impactOffset(const ActionResult *result, PresenterStage stage, uint8_t elapsed,
                  int8_t &x, int8_t &y) {
    x = y = 0;
    if (result == nullptr || stage != PresenterStage::Impact ||
        result->kind != ResultKind::Attack) return;
    const uint8_t target = (result->flags & SELF_HIT)
        ? static_cast<uint8_t>(result->actor)
        : static_cast<uint8_t>(result->actor) ^ 1u;
    if (result->hpAfter[target] >= result->hpBefore[target] || elapsed >= 8) return;
    switch (elapsed) {
    case 0: x = -2; break;
    case 1: x = 2; break;
    case 2: x = 1; y = -1; break;
    case 3: x = -1; y = 1; break;
    case 4: x = 1; y = 1; break;
    case 5: x = -1; y = -1; break;
    default: break;
    }
}

enum : uint8_t {
    HIDDEN_MASK = 0x03,
    CAPTION_SHIFT = 2,
    CAPTION_MASK = 0xFC,
    CAPTION_NONE = 0,
    CAPTION_SELF_HIT = 1,
    CAPTION_STATUS_SKIP = 2,
    CAPTION_REFUSED = 3,
    CAPTION_NO_ACTION = 4,
    CAPTION_SWITCHING = 5,
    CAPTION_STAT_UP = 6,
    CAPTION_STAT_DOWN = 7,
    CAPTION_TYPE_UP = 8,
    CAPTION_TYPE_DOWN = 9,
    CAPTION_HP_LOST = 10,
    CAPTION_HP_HEALED = 11,
    CAPTION_GATHERED = 12,
    CAPTION_FLED = 13,
    CAPTION_NO_EFFECT = 14,
    CAPTION_STATUS_APPLIED = 15
};

// Keep the timing vocabulary in code on AVR. A compiler switch table costs
// static SRAM in this sketch even though every duration is a compile-time byte.
uint8_t duration(PresenterStage stage) {
    if (stage == PresenterStage::Announce) return ANNOUNCE_TICKS;
    if (stage == PresenterStage::Impact) return IMPACT_TICKS;
    if (stage == PresenterStage::Consequence) return CONSEQUENCE_TICKS;
    if (stage == PresenterStage::Faint) return FAINT_TICKS;
    if (stage == PresenterStage::Terminal) return TERMINAL_TICKS;
    return 0;
}

constexpr uint8_t hiddenBit(uint8_t side) { return static_cast<uint8_t>(1u << side); }

struct TextSprite {
    uint24_t address;
    uint8_t width;
};

TextSprite typeSprite(Effect effect) {
    switch (effect) {
    case Effect::DPRSD: case Effect::ENLTND: return {spirit, 35};
    case Effect::SOAKED: case Effect::DRNCHD: return {water, 30};
    case Effect::BUFTD: case Effect::AIRSWPT: return {wind, 25};
    case Effect::SOILED: case Effect::GRNDED: return {earth, 30};
    case Effect::SCRCHD: case Effect::KINDLD: return {fire, 25};
    case Effect::ZAPPED: case Effect::CHRGD: return {lightning, 50};
    case Effect::TANGLD: case Effect::ENRCHD: return {plant, 30};
    case Effect::REDCD: case Effect::EVOLVD: return {elder, 30};
    default: return {0, 0};
    }
}

TextSprite statSprite(Effect effect) {
    switch (effect) {
    case Effect::ATKDWN: case Effect::ATKUP: return {atkText, 25};
    case Effect::DEFDWN: case Effect::DEFUP: return {defText, 25};
    case Effect::SPCADWN: case Effect::SPCAUP: return {satkText, 30};
    case Effect::SPCDDWN: case Effect::SPCDUP: return {sdefText, 30};
    case Effect::SPDDWN: case Effect::SPDUP: return {spdText, 25};
    default: return {0, 0};
    }
}

TextSprite effectivenessSprite(Modifier modifier) {
    // Match 5 * (character count + the legacy leading space) used by the
    // generated string bitmaps. A one-byte overread pulls unrelated packed
    // sprite data into the effectiveness label and makes it appear garbled.
    switch (modifier) {
    case Modifier::Quarter: return {quarter, effectivenessTextWidth(modifier)};
    case Modifier::Half: return {half, effectivenessTextWidth(modifier)};
    case Modifier::Double: return {doubled, effectivenessTextWidth(modifier)};
    case Modifier::Quadruple: return {quad, effectivenessTextWidth(modifier)};
    default: return {0, 0};
    }
}

bool statRaised(Effect effect) {
    return static_cast<uint8_t>(effect) >= static_cast<uint8_t>(Effect::ATKUP) &&
           static_cast<uint8_t>(effect) <= static_cast<uint8_t>(Effect::SPDUP);
}

bool typeRaised(Effect effect) {
    return static_cast<uint8_t>(effect) >= static_cast<uint8_t>(Effect::ENLTND) &&
           static_cast<uint8_t>(effect) <= static_cast<uint8_t>(Effect::EVOLVD);
}

void cacheCreatureName(PreparedBattleItem &item, uint8_t id, uint8_t &lookups) {
    item.nameWidth = readCreatureNameWidth(id);
    item.nameHeight = 8;
    if (item.nameWidth != 0 && id < 32) {
        item.name = readCreatureNameAddress(id);
        ++lookups;
    }
}

void cacheCreatureDetail(PreparedBattleItem &item, uint8_t id, uint8_t &lookups) {
    item.detailWidth = readCreatureNameWidth(id);
    item.detailHeight = 8;
    if (item.detailWidth != 0 && id < 32) {
        item.detail = readCreatureNameAddress(id);
        ++lookups;
    }
}

void cacheMoveName(PreparedBattleItem &item, uint8_t id, uint8_t &lookups) {
    item.detailWidth = readMoveNameWidth(id);
    item.detailHeight = 8;
    if (item.detailWidth != 0 && id < 33) {
        item.detail = readMoveNameAddress(id);
        ++lookups;
    }
}

#ifndef TEST
const char captionTextData[] PROGMEM =
    "\0hits itself\0could not act\0cannot do that\0did nothing\0switching\0"
    "rose\0fell\0strengthened\0weakened\0lost hp\0healed hp\0gathered\0"
    "fled\0no effect\0status applied";
// These byte offsets include every preceding NUL, including the initial one.
const uint8_t captionOffsets[] PROGMEM = {
    0, 1, 13, 27, 42, 54, 64, 69, 74, 87, 96, 104, 114, 123, 128, 138
};
const char *captionText(uint8_t caption) {
    if (caption > CAPTION_STATUS_APPLIED) caption = CAPTION_NONE;
    return captionTextData + pgm_read_byte(&captionOffsets[caption]);
}

void drawCaptionText(const char *text, uint8_t x, uint8_t y) {
    uint8_t glyphX = x;
    // Authored captions are NUL-terminated, at most 14 characters, and use
    // lowercase letters/spaces in the existing 5x6 FX font.
    for (;;) {
        const uint8_t character = pgm_read_byte(text++);
        if (character == 0) break;
        if (character != ' ') {
            drawGlyph(glyphX, y, character, Blit::NEGATIVE);
        }
        glyphX += 6;
    }
}

void drawCaption(uint8_t caption, uint8_t x, uint8_t y) {
    drawCaptionText(captionText(caption), x, y);
}

void drawDamageLine(uint8_t damage, uint8_t x, uint8_t y) {
    uint8_t divisor = damage >= 100 ? 100 : damage >= 10 ? 10 : 1;
    do {
        drawGlyph(x, y, static_cast<uint8_t>('0' + damage / divisor), Blit::NEGATIVE);
        x += 6;
        damage %= divisor;
        divisor /= 10;
    } while (divisor != 0);
    drawCaptionText(PSTR("damage dealt"), x + 6, y);
}

void drawBlackText(uint8_t x, uint8_t y, uint24_t address, uint8_t width) {
    // Negative blitting maps the white-on-black FX raster directly onto the
    // white panel, including rows that cross an 8-pixel page boundary.
    Blit::draw(x, y, width, 8, address, FRAME(0), Blit::NEGATIVE);
}

void drawBlackCaption(uint8_t caption, uint8_t x, uint8_t y) {
    drawCaption(caption, x, y);
}

#endif
} // namespace

void BattlePresenter::reset() {
    result_ = nullptr;
    stage_ = PresenterStage::Idle;
    elapsed_ = fact_ = flags_ = 0;
    displayHp_[0] = displayHp_[1] = 0;
    item_ = {};
}

void BattlePresenter::begin(const ActionResult &result) {
    result_ = &result;
    elapsed_ = fact_ = flags_ = 0;
    for (uint8_t side = 0; side < 2; ++side) {
        const uint8_t maxHp = result.maxHpBefore[side];
        displayHp_[side] = maxHp == 0 || result.hpBefore[side] > maxHp
            ? (maxHp == 0 ? 0 : maxHp) : result.hpBefore[side];
        if (displayHp_[side] == 0) flags_ |= hiddenBit(side);
    }

    switch (result.kind) {
    case ResultKind::Attack:
        stage_ = (result.flags & REFUSED) ? PresenterStage::Consequence
                                          : PresenterStage::Announce;
        break;
    case ResultKind::Switch:
        stage_ = (result.flags & REFUSED) ? PresenterStage::Consequence
                                          : PresenterStage::Announce;
        break;
    case ResultKind::Gather:
        stage_ = (result.flags & REFUSED) ? PresenterStage::Consequence
                                          : PresenterStage::Announce;
        break;
    case ResultKind::Escape:
        stage_ = (result.flags & REFUSED) ? PresenterStage::Consequence
            : (result.outcome != Outcome::None ? PresenterStage::Terminal
                                                : PresenterStage::Consequence);
        break;
    case ResultKind::Skip:
        stage_ = PresenterStage::Consequence;
        break;
    case ResultKind::EndTurn:
        stage_ = PresenterStage::Done;
        if (nextFact()) break;
        nextFaintOrTerminal();
        break;
    case ResultKind::None:
    default:
        stage_ = PresenterStage::Done;
        break;
    }
    prepare();
}

bool BattlePresenter::nextFact() {
    if (result_ == nullptr) return false;
    while (fact_ < 4) {
        const Consequence &fact = result_->consequences[fact_];
        if (fact.effect == Effect::NONE) return false;
        if (fact.side >= 2) {
            ++fact_;
            continue;
        }
        stage_ = result_->kind == ResultKind::EndTurn &&
                 (fact.effect == Effect::SAPPD || fact.effect == Effect::INFSED)
            ? PresenterStage::Impact : PresenterStage::Consequence;
        return true;
    }
    return false;
}

bool BattlePresenter::nextFaint() {
    while (fact_ < 2) {
        const uint8_t side = fact_++;
        const uint8_t faintFlag = side == 0 ? PLAYER_FAINTED : OPPONENT_FAINTED;
        if ((result_->flags & faintFlag) && result_->hpBefore[side] != 0 &&
            result_->hpAfter[side] == 0) {
            stage_ = PresenterStage::Faint;
            return true;
        }
    }
    return false;
}

void BattlePresenter::nextFaintOrTerminal() {
    fact_ = 0;
    if (nextFaint()) return;
    stage_ = result_->outcome == Outcome::None ? PresenterStage::Done
                                               : PresenterStage::Terminal;
}

void BattlePresenter::prepare() {
    item_ = {};
    flags_ = static_cast<uint8_t>(flags_ & HIDDEN_MASK);
    uint8_t lookups = 0;
    const auto setCaption = [this](uint8_t caption) {
        flags_ = static_cast<uint8_t>((flags_ & HIDDEN_MASK) |
                                     (caption << CAPTION_SHIFT));
    };
    const auto setDetail = [this](TextSprite sprite) {
        item_.detail = sprite.address;
        item_.detailWidth = sprite.width;
        item_.detailHeight = 8;
    };
    const auto loadName = [this, &lookups](uint8_t side) {
        if (side < 2) cacheCreatureName(item_, result_->speciesBefore[side], lookups);
    };

    if (result_ == nullptr) return;
    switch (stage_) {
    case PresenterStage::Announce:
        if (result_->kind == ResultKind::Attack) {
            const uint8_t actor = static_cast<uint8_t>(result_->actor);
            loadName(actor);
            cacheMoveName(item_, result_->index, lookups);
#ifndef TEST
            const bool physical = (result_->flags & PHYSICAL_MOVE) != 0;
            const bool player = actor == static_cast<uint8_t>(Side::Player);
            // Physical moves use the paired wave art; special moves use the
            // beam art. Each animation has a mirrored asset for the actor side.
            item_.animation = player
                ? (physical ? BasicWaveL : basicBeamL)
                : (physical ? BasicWaveR : basicBeamR);
#endif
            item_.animationWidth = item_.animationHeight = 32;
            item_.frames = 8;
        } else if (result_->kind == ResultKind::Switch) {
            const uint8_t actor = static_cast<uint8_t>(result_->actor);
            loadName(actor);
            cacheCreatureDetail(item_, result_->index, lookups);
            setCaption(CAPTION_SWITCHING);
        } else if (result_->kind == ResultKind::Gather) {
            loadName(static_cast<uint8_t>(result_->actor));
            setDetail({gather, gatherTextWidth});
        }
        break;
    case PresenterStage::Impact:
        if (result_->kind == ResultKind::Attack) {
            const uint8_t target = (result_->flags & SELF_HIT)
                ? static_cast<uint8_t>(result_->actor)
                : static_cast<uint8_t>(result_->actor) ^ 1u;
            loadName(target);
            if (result_->effectiveness == Modifier::None)
                setCaption(CAPTION_NO_EFFECT);
            else
                setDetail(effectivenessSprite(result_->effectiveness));
        } else if (result_->kind == ResultKind::Switch) {
            cacheCreatureName(item_, result_->index, lookups);
            setDetail({SwitchIn, switchInTextWidth});
        } else if (result_->kind == ResultKind::Gather) {
            loadName(static_cast<uint8_t>(result_->actor));
            setDetail({gather, gatherTextWidth});
        } else if (result_->kind == ResultKind::EndTurn && fact_ < 4) {
            const Consequence &fact = result_->consequences[fact_];
            const uint8_t side = fact.side;
            loadName(side);
            const uint8_t before = displayHp_[side];
            const uint8_t maximum = result_->maxHpBefore[side];
            const uint8_t after = maximum == 0 || fact.value > maximum
                ? (maximum == 0 ? 0 : maximum) : fact.value;
            item_.animationWidth = before > after ? before - after : after - before;
            displayHp_[side] = after;
            setCaption(fact.effect == Effect::INFSED ? CAPTION_HP_HEALED
                                                      : CAPTION_HP_LOST);
        }
        break;
    case PresenterStage::Consequence:
        if ((result_->flags & STATUS_SKIPPED) != 0) {
            loadName(static_cast<uint8_t>(result_->actor));
            setCaption(CAPTION_STATUS_SKIP);
        } else if ((result_->flags & REFUSED) != 0) {
            loadName(static_cast<uint8_t>(result_->actor));
            setCaption(CAPTION_REFUSED);
        } else if (result_->kind == ResultKind::Skip) {
            loadName(static_cast<uint8_t>(result_->actor));
            setCaption(CAPTION_NO_ACTION);
        } else if ((result_->kind == ResultKind::Attack ||
                    result_->kind == ResultKind::EndTurn) && fact_ < 4) {
            const Consequence &fact = result_->consequences[fact_];
            loadName(fact.side);
            TextSprite prefix = statSprite(fact.effect);
            if (prefix.width != 0) {
                setDetail(prefix);
                setCaption(statRaised(fact.effect) ? CAPTION_STAT_UP : CAPTION_STAT_DOWN);
            } else {
                prefix = typeSprite(fact.effect);
                if (prefix.width != 0) {
                    setDetail(prefix);
                    setCaption(typeRaised(fact.effect) ? CAPTION_TYPE_UP : CAPTION_TYPE_DOWN);
                } else if (result_->kind == ResultKind::Attack) {
                    setCaption(CAPTION_STATUS_APPLIED);
                } else if (fact.effect == Effect::SAPPD) {
                    setCaption(CAPTION_HP_LOST);
                } else if (fact.effect == Effect::INFSED) {
                    setCaption(CAPTION_HP_HEALED);
                } else if (fact.effect == Effect::PINNED) {
                    setCaption(CAPTION_STATUS_SKIP);
                } else if (fact.effect == Effect::CONCUSED) {
                    setCaption(CAPTION_SELF_HIT);
                } else {
                    setCaption(CAPTION_STATUS_APPLIED);
                }
            }
        }
        break;
    case PresenterStage::Faint: {
        const uint8_t side = fact_ == 0 ? 0 : static_cast<uint8_t>(fact_ - 1);
        loadName(side);
        setDetail({Fainted, faintedTextWidth});
        break;
    }
    case PresenterStage::Terminal:
        switch (result_->outcome) {
        case Outcome::Win: setDetail({win, winTextWidth}); break;
        case Outcome::Lose: setDetail({lose, loseTextWidth}); break;
        case Outcome::Escaped: setDetail({escaped, escapedTextWidth}); break;
        case Outcome::Gathered:
            loadName(static_cast<uint8_t>(result_->actor));
            setCaption(CAPTION_GATHERED);
            break;
        case Outcome::Fled:
            loadName(static_cast<uint8_t>(Side::Opponent));
            setCaption(CAPTION_FLED);
            break;
        default: break;
        }
        break;
    case PresenterStage::Idle:
    case PresenterStage::Done:
    default:
        break;
    }
#ifndef TEST
    FxReadCounter::transitionExact(lookups);
#else
    (void)lookups;
#endif
}

void BattlePresenter::next() {
    if (stage_ == PresenterStage::Announce) {
        stage_ = PresenterStage::Impact;
        if (result_->kind == ResultKind::Attack)
            for (uint8_t side = 0; side < 2; ++side)
                displayHp_[side] = result_->maxHpBefore[side] == 0 ? 0
                    : (result_->hpAfter[side] > result_->maxHpBefore[side]
                        ? result_->maxHpBefore[side] : result_->hpAfter[side]);
        if (result_->kind == ResultKind::Switch && !(result_->flags & REFUSED)) {
            const uint8_t side = static_cast<uint8_t>(result_->actor);
            flags_ &= static_cast<uint8_t>(~hiddenBit(side));
            displayHp_[side] = result_->hpAfter[side];
        }
    } else if (stage_ == PresenterStage::Impact || stage_ == PresenterStage::Consequence) {
        if (stage_ == PresenterStage::Impact && result_->kind == ResultKind::Gather) {
            // The progress value is selected from the borrowed result in draw/overlay.
            nextFaintOrTerminal();
        } else if (stage_ == PresenterStage::Impact && result_->kind == ResultKind::Attack) {
            fact_ = 0;
            if (nextFact()) {
                elapsed_ = 0;
                prepare();
                return;
            }
            nextFaintOrTerminal();
        } else if (stage_ == PresenterStage::Impact && result_->kind == ResultKind::EndTurn) {
            ++fact_;
            if (nextFact()) {
                elapsed_ = 0;
                prepare();
                return;
            }
            for (uint8_t side = 0; side < 2; ++side) {
                const uint8_t maximum = result_->maxHpBefore[side];
                displayHp_[side] = maximum == 0 ? 0
                    : (result_->hpAfter[side] > maximum ? maximum : result_->hpAfter[side]);
            }
            nextFaintOrTerminal();
        } else if (stage_ == PresenterStage::Consequence &&
                   (result_->kind == ResultKind::Attack ||
                    result_->kind == ResultKind::EndTurn) &&
                   !(result_->flags & (REFUSED | STATUS_SKIPPED))) {
            ++fact_;
            if (nextFact()) {
                elapsed_ = 0;
                prepare();
                return;
            }
            nextFaintOrTerminal();
        } else {
            nextFaintOrTerminal();
        }
    } else if (stage_ == PresenterStage::Faint) {
        const uint8_t side = fact_ == 0 ? 0 : static_cast<uint8_t>(fact_ - 1);
        flags_ |= hiddenBit(side);
        if (!nextFaint()) {
            stage_ = result_->outcome == Outcome::None ? PresenterStage::Done
                                                       : PresenterStage::Terminal;
        }
    } else if (stage_ == PresenterStage::Terminal) {
        stage_ = PresenterStage::Done;
    }
    elapsed_ = 0;
    prepare();
}

void BattlePresenter::update(bool freshAEdge) {
    const uint8_t ticks = duration(stage_);
    if (ticks == 0) return;
    if (freshAEdge && elapsed_ < ticks - MIN_DWELL_TICKS)
        elapsed_ = ticks - MIN_DWELL_TICKS;
    if (++elapsed_ >= ticks) next();
}

void BattlePresenter::overlay(BattleView &view) const {
    if (result_ == nullptr || result_->kind == ResultKind::None) return;
    const uint8_t actor = static_cast<uint8_t>(result_->actor);
    const bool switchApplied = result_->kind == ResultKind::Switch &&
        !(result_->flags & REFUSED) && stage_ != PresenterStage::Announce;
    for (uint8_t side = 0; side < 2; ++side) {
        if (switchApplied && side == actor) {
            // This is the caller's fresh post-resolution view. It is the only
            // frozen source for incoming maxHP, which ActionResult omits.
            const uint8_t incomingMaxHp = view.active[side].maxHp;
            view.active[side].id = (flags_ & hiddenBit(side)) ? 255
                : (result_->index < 32 ? result_->index : 255);
            view.active[side].maxHp = incomingMaxHp;
        } else {
            view.active[side].id = (flags_ & hiddenBit(side)) ? 255
                : result_->speciesBefore[side];
            view.active[side].maxHp = result_->maxHpBefore[side];
        }
        uint8_t shownHp = displayHp_[side];
        const uint8_t shownMax = view.active[side].maxHp;
        if (shownMax == 0) shownHp = 0;
        else if (shownHp > shownMax) shownHp = shownMax;
        view.active[side].hp = shownHp;
    }
    if (result_->kind == ResultKind::Gather) {
        view.gatherProgress = stage_ == PresenterStage::Announce ||
                              (result_->flags & REFUSED)
            ? result_->progressBefore : result_->progressAfter;
    }
#ifdef CGFX_BATTLE_HP_SPIKE
    overlayHpSpike(view);
#endif
}

void BattlePresenter::overlayHpSpike(BattleView &view) const {
    if (result_ == nullptr || stage_ != PresenterStage::Impact) return;
    uint8_t side, before, after;
    if (result_->kind == ResultKind::Attack) {
        side = static_cast<uint8_t>(result_->actor);
        if (!(result_->flags & SELF_HIT)) side ^= 1u;
        if (side >= 2) return;
        before = result_->hpBefore[side];
        after = result_->hpAfter[side];
    } else if (result_->kind == ResultKind::EndTurn && fact_ < 4) {
        const Consequence &fact = result_->consequences[fact_];
        if (fact.side >= 2 || (fact.effect != Effect::SAPPD &&
                             fact.effect != Effect::INFSED)) return;
        side = fact.side;
        before = result_->hpBefore[side];
        // Previous tick endpoints are resident facts, not a new replay queue.
        for (uint8_t i = 0; i < fact_; ++i) {
            const Consequence &prior = result_->consequences[i];
            if (prior.side == side && (prior.effect == Effect::SAPPD ||
                                      prior.effect == Effect::INFSED))
                before = prior.value;
        }
        after = fact.value;
    } else return;
    const uint8_t maximum = view.active[side].maxHp;
    if (before > maximum) before = maximum;
    if (after > maximum) after = maximum;
    // Hold through tick 6; settle at tick 20, before Impact can finish.
    const uint8_t step = elapsed_ <= 6 ? 0 : elapsed_ >= 20 ? 14 : elapsed_ - 6;
    const uint8_t delta = before > after ? before - after : after - before;
    const uint8_t moved = static_cast<uint16_t>(delta) * step / 14;
    view.active[side].hp = before > after ? before - moved : before + moved;
}

void BattlePresenter::sceneOffset(int8_t &x, int8_t &y) const {
    impactOffset(result_, stage_, elapsed_, x, y);
}

void BattlePresenter::draw() const {
#ifndef TEST
    if (result_ == nullptr || stage_ == PresenterStage::Idle ||
        stage_ == PresenterStage::Done) return;

    // Scene sprites and HP bars shake through drawScene; keep the feedback
    // panel and its text stationary so damage remains easy to read.
    Blit::fillRect(0, 40, 128, 24, WHITE);
    const uint8_t caption = static_cast<uint8_t>((flags_ & CAPTION_MASK) >> CAPTION_SHIFT);
    const int16_t yName = 40, yAction = 48, yDetail = 56;

    if (stage_ == PresenterStage::Announce && result_->kind == ResultKind::Attack) {
        drawBlackText(3, yName, item_.name, item_.nameWidth);
        if (result_->flags & SELF_HIT)
            drawBlackCaption(CAPTION_SELF_HIT, 3, yAction);
        else if (result_->actor == Side::Player)
            drawBlackText(3, yAction, attackText, attackTextWidth);
        else
            drawBlackText(3, yAction, enemyAttackText, enemyAttackTextWidth);
        drawBlackText(3, yDetail, item_.detail, item_.detailWidth);
    } else if (stage_ == PresenterStage::Announce && result_->kind == ResultKind::Switch) {
        drawBlackText(3, yName, item_.name, item_.nameWidth);
        drawBlackCaption(caption, 3, yAction);
        drawBlackText(3, yDetail, item_.detail, item_.detailWidth);
    } else if (stage_ == PresenterStage::Announce && result_->kind == ResultKind::Gather) {
        drawBlackText(3, yName, item_.name, item_.nameWidth);
        drawBlackText(3, yAction, item_.detail, item_.detailWidth);
        drawNumbersBlack(3, yDetail, result_->progressBefore);
    } else if (stage_ == PresenterStage::Impact && result_->kind == ResultKind::Attack) {
        const uint8_t target = (result_->flags & SELF_HIT)
            ? static_cast<uint8_t>(result_->actor)
            : static_cast<uint8_t>(result_->actor) ^ 1u;
        const uint8_t before = result_->hpBefore[target];
        const uint8_t after = result_->hpAfter[target];
        const uint8_t damage = before > after ? before - after : 0;
        drawBlackText(3, yName, item_.name, item_.nameWidth);
        drawDamageLine(damage, 3, yAction);
        drawBlackText(3, yDetail, item_.detail, item_.detailWidth);
        drawBlackCaption(caption, 3, yDetail);
    } else if (stage_ == PresenterStage::Impact && result_->kind == ResultKind::Switch) {
        drawBlackText(3, yName, item_.name, item_.nameWidth);
        drawBlackText(3, yAction, item_.detail, item_.detailWidth);
    } else if (stage_ == PresenterStage::Impact && result_->kind == ResultKind::Gather) {
        drawBlackText(3, yName, item_.name, item_.nameWidth);
        drawBlackText(3, yAction, item_.detail, item_.detailWidth);
        drawNumbersBlack(3, yDetail, result_->progressAfter);
    } else if (stage_ == PresenterStage::Impact && result_->kind == ResultKind::EndTurn) {
        drawBlackText(3, yName, item_.name, item_.nameWidth);
        drawNumbersBlack(3, yAction, item_.animationWidth);
        drawBlackCaption(caption, 18, yAction);
    } else if (stage_ == PresenterStage::Consequence) {
        drawBlackText(3, yName, item_.name, item_.nameWidth);
        const uint8_t prefixWidth = item_.detailWidth;
        drawBlackText(3, yAction, item_.detail, prefixWidth);
        const uint8_t captionX = static_cast<uint8_t>(3 + prefixWidth + (prefixWidth ? 1 : 0));
        // A long type name followed by "strengthened" exceeds one 128px row.
        // Put the caption on the last panel row instead of overrunning it.
        if (static_cast<uint16_t>(captionX) + 14u * 6u <= 128u)
            drawBlackCaption(caption, captionX, yAction);
        else
            drawBlackCaption(caption, 3, yDetail);
    } else if (stage_ == PresenterStage::Faint) {
        drawBlackText(3, yName, item_.name, item_.nameWidth);
        drawBlackText(3, yAction, item_.detail, item_.detailWidth);
    } else if (stage_ == PresenterStage::Terminal) {
        drawBlackText(3, yAction, item_.detail, item_.detailWidth);
        drawBlackCaption(caption, 3, yAction);
        drawBlackText(3, yName, item_.name, item_.nameWidth);
    }

    if (item_.animation != 0 && item_.frames != 0 && stage_ == PresenterStage::Announce) {
        const uint8_t frame = static_cast<uint8_t>(
            static_cast<uint16_t>(elapsed_) * (item_.frames - 1) / (ANNOUNCE_TICKS - 1));
        Blit::draw(result_->actor == Side::Player ? 32 : 64, 0,
                                item_.animationWidth, item_.animationHeight,
                                item_.animation, FRAME(frame), Blit::PLUSMASK);
    }
#endif
}

} // namespace battle
