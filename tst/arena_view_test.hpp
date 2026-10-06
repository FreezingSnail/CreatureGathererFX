#pragma once

#include "test.hpp"
#include "src/engine/arena/ArenaView.hpp"
#include "src/lib/FxReadCounter.hpp"
#include "src/FXDataFake.hpp"
#include <Arduboy2.h>
#include <cstring>

inline void ArenaViewSuite(TestRunner &runner)
{
    TestSuite suite("Arena view");
    Test test("cached selection rendering is read-free and immutable");
    using namespace arena;
    ArenaUiState ui{};
    ui.screen = ArenaScreen::PlayerTeam;
    ui.list = {3, 1, 1, 0};
    ui.preview.labelAddress = 0x12345;
    ui.preview.labelWidth = 80;
    for (uint8_t i = 0; i < 3; ++i) {
        ui.preview.species[i] = i;
        ui.preview.nameAddress[i] = 0x12400 + i * 0x100;
        ui.preview.nameWidth[i] = static_cast<uint8_t>(24 + i * 5);
    }

    fxDataFake::readCount = 0;
    FxReadCounter::resetFrame();
    const ArenaUiState playerUi = ui;
    draw(ui);
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(0),
                "player selection draw performs no logical metadata reads");
    test.assert(std::memcmp(&ui, &playerUi, sizeof(ui)), 0,
                "player selection draw preserves UI and cached preview");

    ui.screen = ArenaScreen::OpponentTeam;
    ui.list = {5, 1, 4, 4};
    const ArenaUiState opponentUi = ui;
    FxReadCounter::resetFrame();
    draw(ui);
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(0),
                "opponent selection draw performs no logical metadata reads");
    test.assert(std::memcmp(&ui, &opponentUi, sizeof(ui)), 0,
                "opponent selection draw preserves UI");

    suite.addTest(test);
    runner.addTestSuite(suite);
}
