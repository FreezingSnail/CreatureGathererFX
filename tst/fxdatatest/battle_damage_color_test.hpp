#pragma once

#include "fxtest.hpp"
#include "battlepresentation_fixture.hpp"

inline void test_battle_damage_color(FxTest &test) {
    using namespace battle;
    ActionResult result{};
    BattlePresenter presenter;
    battle_presentation_fixture::fill(result, false);
    const BattleView baseView = battle_presentation_fixture::afterView(result);
    presenter.begin(result);
    for (uint8_t tick = 0; tick < ANNOUNCE_TICKS; ++tick) presenter.update(false);
    test.expectEq(static_cast<uint8_t>(presenter.stage()),
                  static_cast<uint8_t>(PresenterStage::Impact), F("damage enters impact"));

    // Full "Damage delt!" bitmap: twelve glyphs plus the generated leading blank.
    uint8_t expectedLabel[damageTextWidth];
    uint8_t expectedNumber[11];
    arduboy.clear();
    Blit::fillRect(0, 40, 128, 24, WHITE);
    Blit::draw(16, 48, sizeof(expectedLabel), 8, damageText, FRAME(0), Blit::OVERWRITE);
    for (uint8_t column = 0; column < sizeof(expectedLabel); ++column)
        expectedLabel[column] = static_cast<uint8_t>(~Arduboy2Base::sBuffer[6 * 128 + 16 + column]);

    arduboy.clear();
    Blit::fillRect(0, 40, 128, 24, WHITE);
    const uint8_t damage = result.hpBefore[1] - result.hpAfter[1];
    drawNumbersBlack(3, 48, damage);
    for (uint8_t column = 0; column < sizeof(expectedNumber); ++column)
        expectedNumber[column] = Arduboy2Base::sBuffer[6 * 128 + 3 + column];

    battle_presentation_fixture::drawView(baseView);
    presenter.draw();
    bool labelMatchesBlackTreatment = true;
    for (uint8_t column = 0; column < sizeof(expectedLabel); ++column) {
        if (Arduboy2Base::sBuffer[6 * 128 + 16 + column] != expectedLabel[column]) {
            labelMatchesBlackTreatment = false;
            break;
        }
    }
    test.expectEq(labelMatchesBlackTreatment, true,
                  F("damage label is inverted to black text on white panel"));

    bool noAdjacentPackedPixels = true;
    for (uint8_t column = 16 + sizeof(expectedLabel); column < 112; ++column) {
        if (Arduboy2Base::sBuffer[6 * 128 + column] != 0xFF) {
            noAdjacentPackedPixels = false;
            break;
        }
    }
    test.expectEq(noAdjacentPackedPixels, true,
                  F("damage label stops before adjacent packed asset data"));

    bool numberMatchesBlackTreatment = true;
    for (uint8_t column = 0; column < sizeof(expectedNumber); ++column) {
        if (Arduboy2Base::sBuffer[6 * 128 + 3 + column] != expectedNumber[column]) {
            numberMatchesBlackTreatment = false;
            break;
        }
    }
    test.expectEq(numberMatchesBlackTreatment, true,
                  F("damage value retains black number sprite"));
}
