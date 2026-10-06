#pragma once

#include "fxtest.hpp"
#include "battlepresentation_fixture.hpp"
#include "src/engine/draw.h"
#include "src/lib/FxReadCounter.hpp"

namespace battle_presentation_test_detail {
extern "C" uint8_t __bss_end;

inline bool pixel(uint8_t x, uint8_t y) {
    return (arduboy.getBuffer()[x + static_cast<uint16_t>(y / 8) * 128]
            & (1u << (y % 8))) != 0;
}

inline uint16_t signature(uint8_t x, uint8_t firstPage, uint8_t width, uint8_t pages) {
    uint16_t hash = 1;
    for (uint8_t page = firstPage; page < firstPage + pages; ++page)
        for (uint8_t col = x; col < x + width; ++col)
            hash = static_cast<uint16_t>(hash * 33u + arduboy.getBuffer()[page * 128u + col]);
    return hash;
}

inline bool hasInk(uint8_t x, uint8_t y, uint8_t width, uint8_t height, bool ink) {
    for (uint8_t row = y; row < y + height; ++row)
        for (uint8_t col = x; col < x + width; ++col)
            if (pixel(col, row) == ink) return true;
    return false;
}

inline bool barEquals(uint8_t hp, int8_t shakeX = 0, int8_t shakeY = 0) {
    const uint8_t width = static_cast<uint16_t>(hp) * 30 / 100;
    for (uint8_t col = 0; col < 30; ++col)
        if (pixel(8 + shakeX + col, 36 + shakeY) != (col < width) ||
            pixel(8 + shakeX + col, 37 + shakeY) != (col < width))
            return false;
    return true;
}

inline void render(const battle::BattlePresenter &presenter,
                   const battle::BattleView &baseView) {
    battle::BattleView view = baseView;
    presenter.overlay(view);
    arduboy.clear();
    int8_t shakeX, shakeY;
    presenter.sceneOffset(shakeX, shakeY);
    drawScene(view, shakeX, shakeY);
    presenter.draw();
}

inline void switchedSprites(FxTest &test) {
    const uint8_t playerBefore = pgm_read_byte(&creatureFixtures->id);
    const uint8_t opponentBefore = pgm_read_byte(
        &(creatureFixtures[creatureFixtureCount - 1].id));
    test.expectEq(playerBefore != opponentBefore, true,
                  F("switch fixture uses distinct creatures"));

    for (uint8_t side = 0; side < 2; ++side) {
        battle::BattleView view{};
        view.active[0] = {playerBefore, 80, 100};
        view.active[1] = {opponentBefore, 80, 100};
        const uint8_t x = side == static_cast<uint8_t>(battle::Side::Player) ? 96 : 0;
        const uint8_t incoming = side == static_cast<uint8_t>(battle::Side::Player)
            ? opponentBefore : playerBefore;
        arduboy.clear();
        drawScene(view);
        const uint16_t outgoing = signature(x, 0, 32, 4);

        // Model the normal transition redraw: only the active view changes;
        // the framebuffer still contains the outgoing masked sprite.
        view.active[side].id = incoming;
        drawScene(view);
        const uint16_t switched = signature(x, 0, 32, 4);
        arduboy.clear();
        drawScene(view);
        test.expectEq(switched, signature(x, 0, 32, 4),
                      F("switch redraw matches incoming sprite on clean canvas"));
        test.expectEq(switched != outgoing, true,
                      F("switch replaces outgoing sprite pixels"));
    }
}

inline bool whiteFrom(uint8_t x, uint8_t y, uint8_t endX) {
    for (; x < endX; ++x)
        for (uint8_t row = y; row < y + 8; ++row)
            if (!pixel(x, row)) return false;
    return true;
}

inline void effectivenessFeedback(FxTest &test, Modifier modifier,
                                 uint8_t expectedWidth, const __FlashStringHelper *label) {
    battle::ActionResult result{};
    battle_presentation_fixture::fill(result, false);
    result.effectiveness = modifier;
    battle::BattlePresenter presenter;
    presenter.begin(result);
    for (uint8_t tick = 0; tick < battle::ANNOUNCE_TICKS; ++tick)
        presenter.update(false);
    test.expectEq(static_cast<uint8_t>(presenter.stage()),
                  static_cast<uint8_t>(battle::PresenterStage::Impact),
                  F("effectiveness enters impact feedback"));
    render(presenter, battle_presentation_fixture::afterView(result));
    test.expectEq(hasInk(3, 56, expectedWidth, 8, false), true, label);
    test.expectEq(hasInk(3, 56, expectedWidth, 8, true), true,
                  F("effectiveness text retains white background"));
    test.expectEq(whiteFrom(static_cast<uint8_t>(3 + expectedWidth), 56, 128), true,
                  F("effectiveness blit stops at generated asset width"));
    test.expectEq(hasInk(16, 48, 35, 8, false), true,
                  F("post-damage label renders complete damage bitmap"));
    test.expectEq(whiteFrom(51, 48, 86), true,
                  F("post-damage blit does not read adjacent packed assets"));
}

__attribute__((noinline)) inline void playback(FxTest &test, bool knockout) {
    using namespace battle;
    ActionResult result{};
    BattlePresenter presenter;
    battle_presentation_fixture::fill(result, knockout);
    const BattleView baseView = battle_presentation_fixture::afterView(result);
    FxReadCounter::resetFrame();
    presenter.begin(result);
    test.expectEq(FxReadCounter::count(), 2,
                  F("begin resolves creature and move names at the transition"));
    test.expectEq(FxReadCounter::markUpdate(), true, F("begin metadata is transition-only"));
    render(presenter, baseView);
    test.expectEq(FxReadCounter::count(), 2, F("draw streams pixels without metadata reads"));
    test.expectEq(barEquals(60), true, F("render starts with before HP bar"));
    test.expectEq(hasInk(3, 40, 75, 8, false), true, F("real FX creature name has ink"));
    test.expectEq(hasInk(3, 56, 45, 8, false), true, F("real FX move name has ink"));
    test.expectEq(hasInk(32, 0, 32, 32, true), true, F("real beam animation is visible"));
    const uint16_t ordinaryFrame = signature(0, 0, 128, 8);
    const uint16_t sprite = signature(0, 0, 32, 4);
    const uint16_t animation = signature(32, 0, 32, 4);
    render(presenter, baseView); render(presenter, baseView);
    test.expectEq(signature(0, 0, 128, 8), ordinaryFrame, F("repeated draw reproduces identical pixels"));
    test.expectEq(static_cast<uint8_t>(presenter.stage()), static_cast<uint8_t>(PresenterStage::Announce),
                  F("draw never advances stage"));
    for (uint8_t tick = 0; tick < ANNOUNCE_TICKS - 1; ++tick) presenter.update(false);
    render(presenter, baseView);
    test.expectEq(signature(32, 0, 32, 4) != animation, true, F("announce timer advances actual beam frames"));
    test.expectEq(barEquals(60), true, F("last announce frame retains old HP"));
    FxReadCounter::resetFrame();
    presenter.update(false);
    test.expectEq(FxReadCounter::markUpdate(), true, F("impact update has no metadata reads"));
    render(presenter, baseView);
    int8_t shakeX, shakeY;
    presenter.sceneOffset(shakeX, shakeY);
    test.expectEq(barEquals(knockout ? 0 : 35, shakeX, shakeY), true,
                  F("actual bar changes at shaken impact"));
    test.expectEq(shakeX, -2, F("impact starts with bounded scene offset"));
    test.expectEq(shakeY, 0, F("impact shake offset is deterministic"));
    test.expectEq(signature(0, 0, 128, 8) != ordinaryFrame, true, F("impact has distinct visible output"));
    for (uint8_t tick = 0; tick < 2; ++tick) {
        FxReadCounter::resetFrame();
        presenter.update(false);
        test.expectEq(FxReadCounter::markUpdate(), true, F("shake updates have no FX reads"));
    }
    render(presenter, baseView);
    test.expectEq(hasInk(3, 40, 75, 8, false), true,
                  F("post-damage name retains black glyph pixels during shake"));
    test.expectEq(hasInk(3, 40, 75, 8, true), true,
                  F("post-damage name retains white background during shake"));
    for (uint8_t tick = 2; tick < IMPACT_TICKS; ++tick) {
        FxReadCounter::resetFrame();
        presenter.update(false);
        test.expectEq(FxReadCounter::markUpdate(), true, F("playback reads only at prepared transitions"));
    }
    presenter.sceneOffset(shakeX, shakeY);
    test.expectEq(shakeX, 0, F("impact offset settles by end of effect"));
    test.expectEq(shakeY, 0, F("vertical offset settles by end of effect"));
    render(presenter, baseView);
    test.expectEq(signature(0, 0, 32, 4), sprite, F("impact settles back to creature sprite position"));
    if (knockout) {
        test.expectEq(static_cast<uint8_t>(presenter.stage()), static_cast<uint8_t>(PresenterStage::Faint),
                      F("KO has separate faint presentation"));
        render(presenter, baseView);
        test.expectEq(signature(0, 0, 32, 4), sprite, F("faint message retains sprite"));
        test.expectEq(hasInk(3, 48, 45, 8, false), true, F("real faint bitmap has ink"));
        for (uint8_t tick = 0; tick < FAINT_TICKS - 1; ++tick) presenter.update(false);
        render(presenter, baseView);
        test.expectEq(signature(0, 0, 32, 4), sprite, F("sprite remains until last faint frame"));
        presenter.update(false); render(presenter, baseView);
        test.expectEq(hasInk(0, 0, 32, 32, true), false, F("completed faint erases sprite"));
    }
    test.expectEq(presenter.done(), true, F("fixture playback completes with no input"));
    render(presenter, baseView);
    test.expectEq(barEquals(knockout ? 0 : 35), true, F("natural playback final HP exact"));

    // A presses accelerate display only; final state and sprite rules survive.
    FxReadCounter::resetFrame();
    presenter.begin(result);
    for (uint8_t stages = 0; stages < 3 && !presenter.done(); ++stages) {
        presenter.update(true);
        for (uint8_t tick = 1; tick < MIN_DWELL_TICKS; ++tick) presenter.update(false);
    }
    render(presenter, baseView);
    test.expectEq(presenter.done(), true, F("fresh A acceleration completes naturally"));
    test.expectEq(barEquals(knockout ? 0 : 35), true, F("accelerated final HP exact"));
}

inline uint16_t paint() {
    const uint16_t top = static_cast<uint16_t>(SP) - 64;
    for (volatile uint8_t *p = &__bss_end; reinterpret_cast<uint16_t>(p) < top; ++p) *p = 0xC5;
    return top;
}

inline uint16_t lowWater(uint16_t top) {
    for (volatile uint8_t *p = &__bss_end; reinterpret_cast<uint16_t>(p) < top; ++p)
        if (*p != 0xC5) return reinterpret_cast<uint16_t>(p);
    return top;
}

__attribute__((noinline)) inline void measuredChain() {
    battle_presentation_fixture::shippingSpike();
}

__attribute__((noinline)) inline void measuredCaptionChain() {
    battle_presentation_fixture::captionSpike();
}
} // namespace battle_presentation_test_detail

inline void test_battlepresentation(FxTest &test) {
    using namespace battle_presentation_test_detail;
    playback(test, false);
    playback(test, true);
    switchedSprites(test);
    effectivenessFeedback(test, Modifier::Quarter, 90,
                          F("barely damages bitmap renders"));
    effectivenessFeedback(test, Modifier::Half, 65,
                          F("does some bitmap renders"));
    effectivenessFeedback(test, Modifier::Double, 70,
                          F("does great bitmap renders"));
    effectivenessFeedback(test, Modifier::Quadruple, 90,
                          F("devastating bitmap renders"));
    effectivenessFeedback(test, Modifier::None, 90,
                          F("no-effect caption renders"));

    // Empty and absent move IDs never index the FX name table. Species zero
    // is the generated first fixture and must still render; absent255 does not.
    {
        battle::ActionResult result{};
        battle::BattlePresenter presenter;
        battle_presentation_fixture::fill(result, false);
        for (uint8_t edge = 0; edge < 2; ++edge) {
            result.index = edge == 0 ? 32 : 255;
            FxReadCounter::resetFrame();
            presenter.begin(result);
            test.expectEq(FxReadCounter::count(), 1, F("empty/sentinel move omits table lookup"));
            const battle::BattleView baseView = battle_presentation_fixture::afterView(result);
            render(presenter, baseView);
            test.expectEq(hasInk(3, 56, 75, 8, false), false, F("empty/sentinel move row stays blank"));
            test.expectEq(hasInk(96, 0, 32, 32, true), true, F("generated species zero remains visible"));
        }
        result.speciesBefore[0] = 255;
        FxReadCounter::resetFrame();
        presenter.begin(result);
        test.expectEq(FxReadCounter::count(), 0, F("absent species never indexes name table"));
        const battle::BattleView baseView = battle_presentation_fixture::afterView(result);
        render(presenter, baseView);
        test.expectEq(hasInk(96, 0, 32, 32, true), false, F("absent species does not draw sprite"));
    }

    // Paint before entering the local result+presenter chain, so both borrowed
    // result storage and the real FX renderer call depth count in this proof.
    const uint16_t top = paint();
    measuredChain();
    const uint16_t headroom = lowWater(top) - reinterpret_cast<uint16_t>(&__bss_end);
    Serial.print(F("presenter chain painted headroom=")); Serial.println(headroom);
    Serial.print(F("presenter chain effective headroom=")); Serial.println(headroom >= 69 ? headroom - 69 : 0);
    Serial.print(F("ActionResult bytes=")); Serial.println(sizeof(battle::ActionResult));
    Serial.print(F("BattlePresenter bytes=")); Serial.println(sizeof(battle::BattlePresenter));
    test.expectEq(headroom >= 219, true, F("presenter chain preserves 150 B after 69 B USB ISR"));

    FxReadCounter::resetFrame();
    const uint16_t captionTop = paint();
    measuredCaptionChain();
    const uint16_t captionHeadroom = lowWater(captionTop) - reinterpret_cast<uint16_t>(&__bss_end);
    Serial.print(F("caption chain painted headroom=")); Serial.println(captionHeadroom);
    Serial.print(F("caption chain effective headroom="));
    Serial.println(captionHeadroom >= 69 ? captionHeadroom - 69 : 0);
    test.expectEq(captionHeadroom >= 219, true, F("PSTR caption chain preserves 150 B after ISR"));
    test.expectEq(FxReadCounter::count(), 1, F("repeated PSTR caption draws add no metadata reads"));
    test.expectEq(hasInk(3, 48, 72, 8, false), true, F("PSTR caption renders black font glyphs"));
}
