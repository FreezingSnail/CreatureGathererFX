#pragma once

#include "fxtest.hpp"
#include "harness/fx_globals.hpp"
#include "src/engine/battle/BattleSession.hpp"
#include "src/lib/FxReadCounter.hpp"

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
}
