#pragma once

#include "fxtest.hpp"
#include "harness/fx_globals.hpp"
#include "src/engine/battle/BattleSession.hpp"
#include "src/engine/menu/PackedMoveInfo.hpp"
#include "src/lib/FxReadCounter.hpp"
#include "src/lib/MoveIds.hpp"
#include "src/lib/ReadData.hpp"

inline void test_menurun(FxTest &test) {
    constexpr uint16_t iterations = 2048;
    enterBattle();
    player.basic();
    battleSession().beginTrainer(0);
    menu.clear();
    menu.push(BATTLE_OPTIONS);
    menu.cursorIndex = 0;
    arduboy.pollButtons();

    FxReadCounter::resetFrame();
    const uint32_t start = micros();
    for (uint16_t i = 0; i < iterations; ++i) {
        menu.run(battleSession());
    }
    const uint32_t elapsed = micros() - start;
    const uint32_t averageMicros = elapsed / iterations;

    test.expectEq(menu.menuPointer, 0, F("menu remains on options"));
    test.expectEq(menu.cursorIndex, 0, F("idle menu input preserves cursor"));
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(0),
                  F("steady menu update has no FX reads"));
    test.expectEq(averageMicros < 19230, true,
                  F("menu update stays within 52 Hz frame period"));
    Serial.print(F("menu_run_avg_us="));
    Serial.println(averageMicros);

    battle::BattleView view = {};
    view.moveIds[0] = 0;
    view.moveIds[1] = DELUGE_MOVE_ID;
    view.moveIds[2] = LEGACY_EMPTY_MOVE_ID;
    view.moveIds[3] = EMPTY_MOVE_ID;
    menu.clear();
    FxReadCounter::resetFrame();
    menu.openMenu(BATTLE_MOVE_SELECT, view);
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(4),
                  F("edge move metadata reads only at submenu open"));
    const uint8_t validIds[2] = {0, DELUGE_MOVE_ID};
    const uint8_t validSlots[2] = {0, 1};
    for (uint8_t i = 0; i < 2; ++i) {
        const Move move = readMoveFX(validIds[i]);
        const uint16_t expected = static_cast<uint16_t>(
            ((static_cast<uint16_t>(move.getMoveType()) & 0x0f) << 6) |
            ((static_cast<uint16_t>(move.getMovePower()) & 0x1f) << 1) |
            (move.isPhysical() ? 1u : 0u));
        test.expectEq(PackedMoveInfo::read(menu.moveInfo(), validSlots[i]), expected,
                      F("edge move type, power, and class display value"));
    }
    test.expectEq(PackedMoveInfo::read(menu.moveInfo(), 2), static_cast<uint16_t>(0),
                  F("legacy empty move displays zero metadata"));
    test.expectEq(PackedMoveInfo::read(menu.moveInfo(), 3), static_cast<uint16_t>(0),
                  F("absent move displays zero metadata"));
}
