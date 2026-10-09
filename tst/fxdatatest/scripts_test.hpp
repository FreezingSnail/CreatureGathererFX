#pragma once

#include <string.h>

#include "fxtest.hpp"
#include "src/fxdata.h"
#include "src/engine/world/Chunk.hpp"
#include "src/engine/world/World.hpp"
#include "src/lib/FxReadCounter.hpp"
#include "src/engine/menu/DialogMenu.hpp"
#include "src/flags/flag_bit_array.hpp"
#include "src/vm/ScriptVM.hpp"
#include "src/vm/opcodes.hpp"

namespace scripts_test_detail {

inline uint16_t readScriptTextCount() {
    uint8_t bytes[2];
    FX::readDataBytes(raw_map_text, bytes, sizeof(bytes));
    return static_cast<uint16_t>(bytes[0]) |
           (static_cast<uint16_t>(bytes[1]) << 8);
}

inline void expectRealScriptSlot(FxTest &test, uint8_t *slot) {
    static const uint8_t expected[] PROGMEM = {
        5, 0, 0, 0, 1, 9, 4, 0, 1, 0, 1, 0, 0, 0, 0, 255,
    };
    const uint24_t address = Chunk::scriptSlotAddr(scripts, 0);
    uint8_t addressBytes[sizeof(address)];
    memcpy(addressBytes, &address, sizeof(address));

    test.expectEq(address == scripts, true, F("chunk zero uses scripts base"));
    test.expectEq(addressBytes[sizeof(address) - 1] != 0, true,
                  F("real script slot address is above 0x010000"));

    // This is one raw 128-byte FX read, the same slot-sized operation used by
    // interact(). The remaining bytes are padding; compare only the encoded
    // nonempty script and its End sentinel.
    FX::readDataBytes(address, slot, Chunk::SCRIPT_SLOT_BYTES);
    for (uint8_t index = 0; index < sizeof(expected); ++index) {
        test.expectEqIdx(slot[index], pgm_read_byte(expected + index),
                         F("chunk zero generated script byte"), index);
    }

    test.expectEq(slot[0], static_cast<uint8_t>(VmOpcode::If),
                  F("real script begins with If"));
    test.expectEq(slot[6], static_cast<uint8_t>(VmOpcode::TpIf),
                  F("real script contains TpIf"));
    test.expectEq(slot[7], 0, F("TpIf source x high byte"));
    test.expectEq(slot[8], 1, F("TpIf source x low byte"));
    test.expectEq(slot[9], 0, F("TpIf source y high byte"));
    test.expectEq(slot[10], 1, F("TpIf source y low byte"));
    test.expectEq(slot[11], 0, F("TpIf target x high byte"));
    test.expectEq(slot[12], 0, F("TpIf target x low byte"));
    test.expectEq(slot[13], 0, F("TpIf target y high byte"));
    test.expectEq(slot[14], 0, F("TpIf target y low byte"));
    test.expectEq(slot[15], static_cast<uint8_t>(VmOpcode::End),
                  F("real script terminates"));
}

inline void testRealScriptExecution(FxTest &test, ScriptVm &vm, uint8_t *slot) {
    FLAG_BIT_ARRAY[0] = 0;
    gameState.playerLocation = To1D(1, 1);
    vm.initVM(slot, To1D(1, 1), To1D(1, 2));
    vm.run();
    test.expectEq(gameState.playerLocation, To1D(1, 1),
                  F("real If skips TpIf while flag is clear"));

    FLAG_BIT_ARRAY[0] = 1;
    gameState.playerLocation = To1D(1, 1);
    vm.initVM(slot, To1D(1, 1), To1D(1, 2));
    vm.run();
    test.expectEq(gameState.playerLocation, To1D(0, 0),
                  F("real TpIf decodes BE coordinates and teleports"));
    FLAG_BIT_ARRAY[0] = 0;
}

inline void testScriptTextDispatch(FxTest &test, ScriptVm &vm, uint8_t *slot) {
    dialogMenu.clear();
    const uint16_t count = readScriptTextCount();

    // The current generated map has no script text. Host ScriptVM tests cover
    // valid Msg/TMsg/SMsg payloads and coordinate filtering; if later map data
    // adds text, this device suite also exercises a valid Msg and its renderer.
    if (count != 0) {
        slot[0] = static_cast<uint8_t>(VmOpcode::Msg);
        slot[1] = 0;
        slot[2] = 0;
        slot[3] = static_cast<uint8_t>(VmOpcode::End);
        vm.initVM(slot, 0, 0);
        vm.run();
        test.expectEq(dialogMenu.peek(), true, F("valid Msg queues a dialog"));
        if (dialogMenu.peek()) {
            test.expectEq(dialogMenu.head().type, SCRIPT_TEXT,
                          F("Msg uses typed script text dialog"));
            test.expectEq(dialogMenu.head().textAddress == static_cast<uint24_t>(0), true,
                          F("script text dialog stores its index"));
            dialogMenu.drawPopMenu();
        }
        dialogMenu.clear();
    }

    const uint16_t invalidIndex = count;
    slot[0] = static_cast<uint8_t>(VmOpcode::Msg);
    slot[1] = static_cast<uint8_t>(invalidIndex >> 8);
    slot[2] = static_cast<uint8_t>(invalidIndex);
    slot[3] = static_cast<uint8_t>(VmOpcode::End);
    vm.initVM(slot, 0, 0);
    vm.run();
    test.expectEq(dialogMenu.peek(), false,
                  F("Msg rejects an index past the generated text table"));
    dialogMenu.drawPopMenu();
    dialogMenu.clear();
}

inline void testCompactWarps(FxTest &test, ScriptVm &vm, uint8_t *slot) {
#ifndef CGFX_FULL_WORLD_VM
    const uint16_t sources[] = {To1D(1,1), To1D(4,4), To1D(12,7)};
    const uint16_t targets[] = {To1D(0,0), To1D(12,7), To1D(4,4)};
    for (uint8_t index=0; index<3; ++index) {
        for (uint8_t flag=0; flag<2; ++flag) {
            FX::readDataBytes(Chunk::scriptSlotAddr(scripts,
                Chunk::chunkOfLocation(sources[index])), slot, 128);
            FLAG_BIT_ARRAY[0]=flag;
            gameState.playerLocation=sources[index];
            vm.initWarpVM(slot, 123, 456);
            test.expectEq(vm.valid, true, F("packed warp profile valid"));
            vm.runWarpVM();
            const uint16_t expected=index==0 && !flag ? sources[index] : targets[index];
            test.expectEq(gameState.playerLocation, expected, F("packed compact warp target"));
            gameState.playerLocation=To1D(200,200);
            vm.initWarpVM(slot, sources[index], sources[index]);
            vm.runWarpVM();
            test.expectEq(gameState.playerLocation, static_cast<uint16_t>(To1D(200,200)),
                          F("warp compares live location"));

            // Exercise the actual nonTEST World A route, not just the VM API.
            gameState.playerLocation=sources[index];
            memset(slot,0,128);
            arduboy.currentButtonState=A_BUTTON;
            arduboy.previousButtonState=0;
            FxReadCounter::resetFrame();
            WorldEngine::interact();
            test.expectEq(gameState.playerLocation, expected, F("World A dispatches packed warp"));
            test.expectEq(FxReadCounter::count(), 1, F("World A reads one FX slot"));
        }
    }
#endif

    // Full world mode honors an active popup; compact mode has no popup route.
    PopUpDialog dialog={};
    dialog.type=SCRIPT_TEXT;
    dialogMenu.pushMenu(dialog);
    arduboy.clear();
    dialogMenu.drawPopMenu();
    bool backgroundDrawn=false;
    for(uint16_t at=0;at<1024;++at) if(arduboy.getBuffer()[at]) backgroundDrawn=true;
    test.expectEq(backgroundDrawn,true,F("retained popup renderer draws background"));
    FLAG_BIT_ARRAY[0]=1;
    gameState.playerLocation=To1D(1,1);
    memset(slot,0,128);
    WorldEngine::syncFromLocation(worldState());
    arduboy.currentButtonState=A_BUTTON;
    arduboy.previousButtonState=0;
    FxReadCounter::resetFrame();
    WorldEngine::runMap(worldState());
#if defined(CGFX_FULL_WORLD_VM) || defined(TEST)
    test.expectEq(gameState.playerLocation, To1D(1,1), F("full world popup blocks script"));
    test.expectEq(FxReadCounter::count(), 0, F("full popup avoids script read"));
    test.expectEq(dialogMenu.peek(),false,F("full world A dismisses popup"));
#else
    test.expectEq(gameState.playerLocation, To1D(0,0), F("compact world skips popup handling"));
    test.expectEq(FxReadCounter::count(), 1, F("compact popup still reads script"));
    test.expectEq(dialogMenu.peek(),true,F("compact world leaves popup queue alone"));
#endif
    dialogMenu.clear();
    arduboy.currentButtonState=arduboy.previousButtonState=0;
    FLAG_BIT_ARRAY[0]=0;
}

inline void testCompactBounds(FxTest &test, ScriptVm &vm, uint8_t *slot) {
    vm.initWarpVM(nullptr,0,0);
    test.expectEq(vm.valid,false,F("null compact slot rejected"));
    vm.runWarpVM();
    memset(slot,0,128);
    vm.initWarpVM(slot,0,0);
    test.expectEq(vm.valid,false,F("zero compact slot rejected"));
    slot[3]=static_cast<uint8_t>(VmOpcode::End);
    vm.initWarpVM(slot,0,0);
    test.expectEq(vm.valid,false,F("full-only Msg rejected"));
    memset(slot,0,128);
    for(uint8_t at=0;at<126;at+=6) slot[at]=static_cast<uint8_t>(VmOpcode::If);
    slot[126]=static_cast<uint8_t>(VmOpcode::TpIf);
    vm.initWarpVM(slot,0,0);
    test.expectEq(vm.valid,false,F("truncated warp rejected"));
    slot[126]=static_cast<uint8_t>(VmOpcode::If);
    vm.initWarpVM(slot,0,0);
    test.expectEq(vm.valid,false,F("missing boundary End rejected"));
    memset(slot,0,128);
    slot[0]=static_cast<uint8_t>(VmOpcode::TpIf);
    slot[2]=slot[4]=255;
    slot[8]=2;
    slot[9]=static_cast<uint8_t>(VmOpcode::End);
    gameState.playerLocation=To1D(255,255);
    vm.initWarpVM(slot,0,0);vm.runWarpVM();
    test.expectEq(gameState.playerLocation,To1D(0,2),F("FF operands are coordinates"));
    memset(slot,0,128);
    slot[0]=static_cast<uint8_t>(VmOpcode::If);
    slot[4]=1;slot[5]=255;
    slot[6]=static_cast<uint8_t>(VmOpcode::TpIf);
    slot[8]=slot[10]=1;
    slot[15]=static_cast<uint8_t>(VmOpcode::End);
    gameState.playerLocation=To1D(1,1);
    vm.initWarpVM(slot,0,0);vm.runWarpVM();
    test.expectEq(gameState.playerLocation,To1D(1,1),F("out of bounds jump ends safely"));
    slot[2]=slot[3]=255;slot[5]=9;
    FLAG_BIT_ARRAY[0]=1;
    vm.initWarpVM(slot,0,0);vm.runWarpVM();
    test.expectEq(gameState.playerLocation,To1D(1,1),F("invalid flag reads clear"));
    FLAG_BIT_ARRAY[0]=0;

    // Normal slots cannot hold30 If commands. Use framebuffer scratch solely
    // to test the runner cap independently of the128B initialization bound.
    uint8_t *longSlot=arduboy.getBuffer();
    memset(longSlot,0,190);
    for(uint16_t at=0;at<180;at+=6) longSlot[at]=static_cast<uint8_t>(VmOpcode::If);
    longSlot[180]=static_cast<uint8_t>(VmOpcode::TpIf);
    longSlot[182]=longSlot[184]=1;
    longSlot[189]=static_cast<uint8_t>(VmOpcode::End);
    vm.base=vm.ptr=longSlot;vm.endPtr=longSlot+189;vm.valid=true;
    vm.runWarpVM();
    test.expectEq(gameState.playerLocation,To1D(1,1),F("compact runner stops after30 commands"));
    test.expectEq(vm.ptr==longSlot,true,F("compact cap resets cursor"));
}

} // namespace scripts_test_detail

inline void test_scripts(FxTest &test, ScriptVm &vm, uint8_t *slot) {
#ifdef CGFX_FULL_WORLD_VM
    static const uint8_t canonical[] PROGMEM = {
        static_cast<uint8_t>(VmOpcode::If),0,0,0,1,9,
        static_cast<uint8_t>(VmOpcode::TpIf),0,1,0,1,0,0,0,0,
        static_cast<uint8_t>(VmOpcode::End)
    };
    memset(slot,0,128);
    memcpy_P(slot,canonical,sizeof(canonical));
#else
    scripts_test_detail::expectRealScriptSlot(test, slot);
#endif
    scripts_test_detail::testRealScriptExecution(test, vm, slot);
    scripts_test_detail::testScriptTextDispatch(test, vm, slot);
    scripts_test_detail::testCompactWarps(test, vm, slot);
    scripts_test_detail::testCompactBounds(test, vm, slot);
}
