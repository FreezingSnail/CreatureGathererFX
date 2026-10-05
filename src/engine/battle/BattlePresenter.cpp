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
    switch (modifier) {
    case Modifier::Quarter: return {quarter, 95};
    case Modifier::Half: return {half, 70};
    case Modifier::Double: return {doubled, 75};
    case Modifier::Quadruple: return {quad, 95};
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
__attribute__((optimize("no-tree-switch-conversion")))
const char *captionText(uint8_t caption) {
    switch (caption) {
    case CAPTION_SELF_HIT: return PSTR("hits itself");
    case CAPTION_STATUS_SKIP: return PSTR("could not act");
    case CAPTION_REFUSED: return PSTR("cannot do that");
    case CAPTION_NO_ACTION: return PSTR("did nothing");
    case CAPTION_SWITCHING: return PSTR("switching");
    case CAPTION_STAT_UP: return PSTR("rose");
    case CAPTION_STAT_DOWN: return PSTR("fell");
    case CAPTION_TYPE_UP: return PSTR("strengthened");
    case CAPTION_TYPE_DOWN: return PSTR("weakened");
    case CAPTION_HP_LOST: return PSTR("lost hp");
    case CAPTION_HP_HEALED: return PSTR("healed hp");
    case CAPTION_GATHERED: return PSTR("gathered");
    case CAPTION_FLED: return PSTR("fled");
    case CAPTION_NO_EFFECT: return PSTR("no effect");
    case CAPTION_STATUS_APPLIED: return PSTR("status applied");
    default: return nullptr;
    }
}

uint8_t drawCaption(uint8_t caption, int16_t x, int16_t y) {
    const char *text = captionText(caption);
    if (text == nullptr) return 0;
    uint8_t count = 0;
    // Every authored caption is at most 14 characters and uses the supported
    // '0'..'z' glyph range in the existing 5x6 FX font.
    while (count < 20) {
        const uint8_t character = pgm_read_byte(text + count);
        if (character == 0) break;
        if (character >= '0' && character <= 'z') {
            Blit::draw(x + static_cast<int16_t>(count) * 6, y,
                                      5, 6, fontTrimmed,
                                      FRAME(character - '0'), Blit::OVERWRITE);
        }
        ++count;
    }
    return static_cast<uint8_t>(count * 6);
}

void drawBlackText(uint8_t x, uint8_t y, uint24_t address, uint8_t width) {
    // Authored battle labels are white glyphs on a black raster. The feedback
    // panel is white, so invert this opaque 8-pixel row after drawing it to
    // match the transparent black damage-number sprites.
    drawText(x, y, address, width, FRAME(0));
    const uint16_t row = static_cast<uint16_t>(y >> 3) * 128;
    for (uint8_t column = 0; column < width; ++column)
        Arduboy2Base::sBuffer[row + x + column] ^= 0xFF;
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
            item_.animation = actor == static_cast<uint8_t>(Side::Player)
                ? basicBeamL : basicBeamR;
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
            setDetail({gather, 30});
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
            setDetail({SwitchIn, 90});
        } else if (result_->kind == ResultKind::Gather) {
            loadName(static_cast<uint8_t>(result_->actor));
            setDetail({gather, 30});
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
        setDetail({Fainted, 45});
        break;
    }
    case PresenterStage::Terminal:
        switch (result_->outcome) {
        case Outcome::Win: setDetail({win, 45}); break;
        case Outcome::Lose: setDetail({lose, 60}); break;
        case Outcome::Escaped: setDetail({escaped, 65}); break;
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
}

void BattlePresenter::draw() const {
#ifndef TEST
    if (result_ == nullptr || stage_ == PresenterStage::Idle ||
        stage_ == PresenterStage::Done) return;

    // Scene sprites and HP bars are rendered once by drawScene(overlay(view)).
    // This layer owns only the bounded feedback panel and attack animation.
    Blit::fillRect(0, 40, 128, 24, WHITE);
    const uint8_t caption = static_cast<uint8_t>((flags_ & CAPTION_MASK) >> CAPTION_SHIFT);
    const int16_t yName = 40, yAction = 48, yDetail = 56;

    if (stage_ == PresenterStage::Announce && result_->kind == ResultKind::Attack) {
        drawText(3, yName, item_.name, item_.nameWidth, FRAME(0));
        if (result_->flags & SELF_HIT)
            drawCaption(CAPTION_SELF_HIT, 3, yAction);
        else if (result_->actor == Side::Player)
            drawText(3, yAction, attackText, 90, FRAME(0));
        else
            drawText(3, yAction, enemyAttackText, 70, FRAME(0));
        drawText(3, yDetail, item_.detail, item_.detailWidth, FRAME(0));
    } else if (stage_ == PresenterStage::Announce && result_->kind == ResultKind::Switch) {
        drawText(3, yName, item_.name, item_.nameWidth, FRAME(0));
        drawCaption(caption, 3, yAction);
        drawText(3, yDetail, item_.detail, item_.detailWidth, FRAME(0));
    } else if (stage_ == PresenterStage::Announce && result_->kind == ResultKind::Gather) {
        drawText(3, yName, item_.name, item_.nameWidth, FRAME(0));
        drawText(3, yAction, item_.detail, item_.detailWidth, FRAME(0));
        drawNumbersBlack(3, yDetail, result_->progressBefore);
    } else if (stage_ == PresenterStage::Impact && result_->kind == ResultKind::Attack) {
        const uint8_t target = (result_->flags & SELF_HIT)
            ? static_cast<uint8_t>(result_->actor)
            : static_cast<uint8_t>(result_->actor) ^ 1u;
        const uint8_t before = result_->hpBefore[target];
        const uint8_t after = result_->hpAfter[target];
        const uint8_t damage = before > after ? before - after : 0;
        drawText(3, yName, item_.name, item_.nameWidth, FRAME(0));
        drawBlackText(16, yAction, damageText, 70);
        drawNumbersBlack(3, yAction, damage);
        drawText(3, yDetail, item_.detail, item_.detailWidth, FRAME(0));
        drawCaption(caption, 3, yDetail);
    } else if (stage_ == PresenterStage::Impact && result_->kind == ResultKind::Switch) {
        drawText(3, yName, item_.name, item_.nameWidth, FRAME(0));
        drawText(3, yAction, item_.detail, item_.detailWidth, FRAME(0));
    } else if (stage_ == PresenterStage::Impact && result_->kind == ResultKind::Gather) {
        drawText(3, yName, item_.name, item_.nameWidth, FRAME(0));
        drawText(3, yAction, item_.detail, item_.detailWidth, FRAME(0));
        drawNumbersBlack(3, yDetail, result_->progressAfter);
    } else if (stage_ == PresenterStage::Impact && result_->kind == ResultKind::EndTurn) {
        drawText(3, yName, item_.name, item_.nameWidth, FRAME(0));
        drawNumbersBlack(3, yAction, item_.animationWidth);
        drawCaption(caption, 18, yAction);
    } else if (stage_ == PresenterStage::Consequence) {
        drawText(3, yName, item_.name, item_.nameWidth, FRAME(0));
        const uint8_t prefixWidth = item_.detailWidth;
        drawText(3, yAction, item_.detail, prefixWidth, FRAME(0));
        drawCaption(caption, static_cast<int16_t>(3 + prefixWidth + (prefixWidth ? 1 : 0)), yAction);
    } else if (stage_ == PresenterStage::Faint) {
        drawText(3, yName, item_.name, item_.nameWidth, FRAME(0));
        drawText(3, yAction, item_.detail, item_.detailWidth, FRAME(0));
    } else if (stage_ == PresenterStage::Terminal) {
        drawText(3, yAction, item_.detail, item_.detailWidth, FRAME(0));
        drawCaption(caption, 3, yAction);
        drawText(3, yName, item_.name, item_.nameWidth, FRAME(0));
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
