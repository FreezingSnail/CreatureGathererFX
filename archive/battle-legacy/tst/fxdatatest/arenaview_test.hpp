#pragma once

#include "fxtest.hpp"
#include "arena_demo_ids.hpp"
#include "generated/arena_demo_data.hpp"
#include "src/engine/arena/ArenaCatalog.hpp"
#include "src/engine/arena/ArenaDemo.hpp"
#include "src/engine/arena/ArenaView.hpp"
#include "src/lib/FxReadCounter.hpp"
#include "src/globals.hpp"
#include <string.h>

inline void test_arenaview(FxTest &test)
{
    arena::ArenaUiState ui{};
    ui.screen = arena::ArenaScreen::PlayerTeam;
    ui.list = {ArenaDemoIds::playerCount, 1, 0, 0};
    for (uint8_t preview = 0; preview < 3; ++preview) {
        const uint8_t team = preview == 0 ? 0 :
            (preview == 1 ? ArenaDemoIds::playerCount - 1 : 2);
        FxReadCounter::resetFrame();
        test.expectEq(arena::loadPreview(arena::ArenaScreen::PlayerTeam,
                                         team, ui.preview), true, F("arena"));
        test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(8), F("arena"));
        test.expectEq(FxReadCounter::markUpdate(), true, F("arena"));
        test.expectEq(FxReadCounter::renderExact(0), true, F("arena"));
        ui.list.cursor = team;
        for (uint8_t frame = 0; frame < 3; ++frame) {
            const arena::ArenaUiState before = ui;
            FxReadCounter::resetFrame();
            test.expectEq(FxReadCounter::markUpdate(), true, F("arena"));
            arena::draw(ui);
            test.expectEq(FxReadCounter::renderExact(0), true, F("arena"));
            test.expectEq(memcmp(&ui, &before, sizeof(ui)), 0, F("arena"));
        }
    }

    ui.screen = arena::ArenaScreen::OpponentTeam;
    ui.list = {ArenaDemoIds::opponentCount, 1, 0, 0};
    bool sawSpeciesZero = false;
    for (uint8_t index = 0; index < ArenaDemoIds::opponentCount; ++index) {
        FxReadCounter::resetFrame();
        test.expectEq(arena::loadPreview(arena::ArenaScreen::OpponentTeam,
                                         index, ui.preview), true, F("arena"));
        test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(6), F("arena"));
        test.expectEq(FxReadCounter::markUpdate(), true, F("arena"));
        test.expectEq(FxReadCounter::renderExact(0), true, F("arena"));
        for (uint8_t slot = 0; slot < 3; ++slot)
            if (ui.preview.species[slot] == 0) sawSpeciesZero = true;
        ui.list.cursor = index;
        FxReadCounter::resetFrame();
        test.expectEq(FxReadCounter::markUpdate(), true, F("arena"));
        arena::draw(ui);
        test.expectEq(FxReadCounter::renderExact(0), true, F("arena"));
    }
    test.expectEq(sawSpeciesZero, true, F("arena"));

}
