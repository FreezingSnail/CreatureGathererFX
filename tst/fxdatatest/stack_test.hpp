#pragma once

#include "fxtest.hpp"
#include "src/save/Compaction.hpp"
#include "src/save/SaveFile.hpp"
#include "src/vm/opcodes.hpp"

namespace stack_fx_test_detail {
extern "C" uint8_t __bss_end;

constexpr uint8_t PAINT_BYTE = 0xC5;
constexpr uint8_t PAINT_MARGIN = 64;
// ATmega32u4 has 2560 B SRAM. The qu9.11 save-chain run measured 421 B
// above the device-test globals; pin 400 B, including the 69 B USB ISR reserve.
constexpr uint16_t MIN_HEADROOM = 400;

inline uint16_t paintStack()
{
    const uint16_t base = reinterpret_cast<uint16_t>(&__bss_end);
    const uint16_t top = static_cast<uint16_t>(SP) - PAINT_MARGIN;
    for (volatile uint8_t *cursor = reinterpret_cast<volatile uint8_t *>(base);
         reinterpret_cast<uint16_t>(cursor) < top; ++cursor) {
        *cursor = PAINT_BYTE;
    }
    return top;
}

inline uint16_t lowWater(uint16_t base, uint16_t top)
{
    if (top <= base) {
        return base;
    }
    for (volatile uint8_t *cursor = reinterpret_cast<volatile uint8_t *>(base);
         reinterpret_cast<uint16_t>(cursor) < top; ++cursor) {
        if (*cursor != PAINT_BYTE) {
            return reinterpret_cast<uint16_t>(cursor);
        }
    }
    return top;
}

inline void printAddress(const __FlashStringHelper *name, uint16_t value)
{
    Serial.print(name);
    Serial.print(F("=0x"));
    Serial.print(value, HEX);
    Serial.print(F(" ("));
    Serial.print(value);
    Serial.println(F(")"));
}

// Keep the large save snapshot off test_stack's entry frame so the paint starts
// above the live stack. The snapshot is allocated only for the final chain.
__attribute__((noinline)) inline SaveStep runSave()
{
    SaveFile state = {};
    saveBegin();
    SaveStep step = SaveStep::Idle;
    for (uint8_t attempts = 0; attempts < 64 && saveInProgress(); ++attempts) {
        step = saveStepAdvance(state);
        FX::waitWhileBusy();
    }
    return step;
}
} // namespace stack_fx_test_detail

inline void test_stack(FxTest &test)
{
    using namespace stack_fx_test_detail;
    const uint16_t base = reinterpret_cast<uint16_t>(&__bss_end);
    const uint16_t top = paintStack();

    vm.initVM();
    uint8_t *script = vm.ptr;
    script[0] = static_cast<uint8_t>(VmOpcode::Tp);
    script[1] = 0;
    script[2] = 4;
    script[3] = 0;
    script[4] = 4;
    script[5] = static_cast<uint8_t>(VmOpcode::End);
    vm.run();

    player.basic();
    engine.startFight(0);
    menu.printMenu(engine);
    engine.opponentCur->level = 31;
    engine.opponentHealths[0] = 1000;
    engine.queueAction(ActionType::ATTACK, 0);
    engine.turnState = BattleState::TURN_INPUT;
    engine.turnTick();
    engine.turnState = BattleState::PLAYER_ATTACK;
    engine.turnTick();
    engine.turnState = BattleState::OPPONENT_RECEIVE_DAMAGE;
    engine.turnTick();
    engine.turnState = BattleState::OPPONENT_RECEIVE_EFFECT_APPLICATION;
    engine.turnTick();
    engine.turnState = BattleState::OPPONENT_ATTACK;
    engine.turnTick();
    engine.turnState = BattleState::PLAYER_RECEIVE_DAMAGE;
    engine.turnTick();
    engine.turnState = BattleState::PLAYER_RECEIVE_EFFECT_APPLICATION;
    engine.turnTick();
    engine.turnState = BattleState::END_TURN;
    engine.turnTick();
    engine.endEncounter();

    const SaveStep step = runSave();

    const uint16_t low = lowWater(base, top);
    const uint16_t headroom = low - base;
    printAddress(F("stack base"), base);
    printAddress(F("stack top"), top);
    printAddress(F("stack low"), low);
    printAddress(F("stack headroom"), headroom);
    test.expectEq(static_cast<uint8_t>(step), static_cast<uint8_t>(SaveStep::Done),
                  F("save compaction completes"));
    test.expectEq(low > base, true, F("stack does not collide with globals"));
    test.expectEq(headroom >= MIN_HEADROOM, true, F("stack headroom >= 400 B"));
}
