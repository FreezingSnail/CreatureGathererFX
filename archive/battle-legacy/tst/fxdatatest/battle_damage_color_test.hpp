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

    // Independent raw font reference for this fixture's 60-to-35 HP delta.
    uint8_t expectedLine[90];
    arduboy.clear();
    Blit::fillRect(0, 40, 128, 24, BLACK);
    const char *text = PSTR("25 damage dealt");
    uint8_t x = 3;
    for (uint8_t character; (character = pgm_read_byte(text++)) != 0; x += 6)
        if (character != ' ')
            Blit::draw(x, 48, 5, 6, ArduFontTrimmed,
                       FRAME(character - '0'), Blit::OVERWRITE);
    for (uint8_t column = 0; column < sizeof(expectedLine); ++column)
        expectedLine[column] = Arduboy2Base::sBuffer[6 * 128 + 3 + column];

    battle_presentation_fixture::drawView(baseView);
    presenter.draw();
    bool labelMatchesWhiteTreatment = true;
    for (uint8_t column = 18; column < sizeof(expectedLine); ++column) {
        if (Arduboy2Base::sBuffer[6 * 128 + 3 + column] != expectedLine[column]) {
            labelMatchesWhiteTreatment = false;
            break;
        }
    }
    test.expectEq(labelMatchesWhiteTreatment, true,
                  F("damage label is white on black"));

    bool noAdjacentPackedPixels = true;
    for (uint8_t column = 3 + sizeof(expectedLine); column < 128; ++column) {
        if (Arduboy2Base::sBuffer[6 * 128 + column] != 0) {
            noAdjacentPackedPixels = false;
            break;
        }
    }
    test.expectEq(noAdjacentPackedPixels, true,
                  F("damage label stops before adjacent packed asset data"));

    bool numberMatchesWhiteTreatment = true;
    for (uint8_t column = 0; column < 18; ++column) {
        if (Arduboy2Base::sBuffer[6 * 128 + 3 + column] != expectedLine[column]) {
            numberMatchesWhiteTreatment = false;
            break;
        }
    }
    test.expectEq(numberMatchesWhiteTreatment, true,
                  F("damage value uses white caption-font digits inline"));
}
