#pragma once

#include <cstring>
#include <fstream>
#include "../test.hpp"
#include "../../src/vm/ScriptVM.hpp"
#include "../../src/vm/opcodes.hpp"
#include "../../src/engine/world/Chunk.hpp"
#include "../../src/globals.hpp"
#include "../../src/flags/flag_bit_array.hpp"

namespace warp_test_detail {
constexpr uint8_t op(VmOpcode value) { return static_cast<uint8_t>(value); }

struct Result { uint16_t location; uint8_t flags; bool valid, reset; };

inline Result execute(uint8_t *slot, bool compact, uint16_t location,
                      uint8_t flags, uint16_t runnerEnd = 0) {
    ScriptVm vm = {};
    gameState.playerLocation = location;
    FLAG_BIT_ARRAY[0] = flags;
    dialogMenu.clear();
    // Deliberately disagree with the live location: TpIf uses live state.
    if (compact) vm.initWarpVM(slot, 123, 456);
    else vm.initVM(slot, 123, 456);
    if (runnerEnd) {
        vm.base = vm.ptr = slot;
        vm.endPtr = slot + runnerEnd;
        vm.valid = true;
    }
    if (compact) vm.runWarpVM(); else vm.run();
    return {gameState.playerLocation, FLAG_BIT_ARRAY[0], vm.valid, vm.ptr == slot};
}

inline void compare(Test &test, uint8_t *slot, uint16_t location, uint8_t flags,
                    uint16_t expected, bool valid = true, uint16_t runnerEnd = 0) {
    const Result full = execute(slot, false, location, flags, runnerEnd);
    const Result compact = execute(slot, true, location, flags, runnerEnd);
    test.assert(compact.location, full.location, "compact/full location parity");
    test.assert(compact.location, expected, "expected location");
    test.assert(compact.flags, full.flags, "compact/full flag parity");
    test.assert(compact.flags, flags, "warp does not write flags");
    test.assert(compact.valid, full.valid, "compact/full scan parity");
    test.assert(compact.valid, valid, "expected scan validity");
    test.assert(compact.reset, true, "compact runner returns to base");
    test.assert(full.reset, true, "full runner returns to base");
}
}

inline void WarpVmTest(TestRunner &runner) {
    using namespace warp_test_detail;
    TestSuite suite("Compact world VM");
    Test canonical("canonical fixture parity");
    const uint8_t fixtures[3][16] = {
        {op(VmOpcode::If),0,0,0,1,9,op(VmOpcode::TpIf),0,1,0,1,0,0,0,0,op(VmOpcode::End)},
        {op(VmOpcode::TpIf),0,4,0,4,0,12,0,7,op(VmOpcode::End)},
        {op(VmOpcode::TpIf),0,12,0,7,0,4,0,4,op(VmOpcode::End)}
    };
    const uint16_t sources[] = {To1D(1,1), To1D(4,4), To1D(12,7)};
    const uint16_t targets[] = {To1D(0,0), To1D(12,7), To1D(4,4)};
    for (uint8_t index=0; index<3; ++index) {
        uint8_t slot[128] = {};
        std::memcpy(slot, fixtures[index], 16);
        for (uint8_t flags=0; flags<2; ++flags) {
            compare(canonical, slot, sources[index], flags,
                    index==0 && !flags ? sources[index] : targets[index]);
            compare(canonical, slot, To1D(200,200), flags, To1D(200,200));
        }
    }
    suite.addTest(canonical);
#ifndef CGFX_FULL_WORLD_VM
    Test packed("packed warp parity");
    std::ifstream image("fxdata/generated/scripts.bin", std::ios::binary);
    packed.assert(image.good(), true, "packed scripts are available");
    for (uint8_t index = 0; index < 3 && image.good(); ++index) {
        uint8_t slot[128];
        const uint16_t chunk = Chunk::chunkOfLocation(sources[index]);
        image.seekg(static_cast<std::streamoff>(chunk) * sizeof(slot));
        image.read(reinterpret_cast<char *>(slot), sizeof(slot));
        packed.assert(image.gcount(), static_cast<std::streamsize>(sizeof(slot)), "packed slot length");
        for (uint8_t flags = 0; flags < 2; ++flags) {
            compare(packed, slot, sources[index], flags,
                    index == 0 && !flags ? sources[index] : targets[index]);
            compare(packed, slot, To1D(200,200), flags, To1D(200,200));
        }
    }
    suite.addTest(packed);
#endif

    Test bounds("warp scan and operand bounds");
    compare(bounds, nullptr, 7, 0, 7, false);
    uint8_t slot[128] = {};
    compare(bounds, slot, 7, 0, 7, false);
    for (uint8_t at = 0; at < 126; at += 6) {
        slot[at] = op(VmOpcode::If);
        slot[at+1] = 0;
        slot[at+2] = slot[at+3] = slot[at+4] = slot[at+5] = 0;
    }
    slot[126] = op(VmOpcode::TpIf); // Eight operands cannot fit before byte128.
    compare(bounds, slot, 7, 0, 7, false);
    slot[126] = op(VmOpcode::If);
    compare(bounds, slot, 7, 0, 7, false);
    slot[126] = op(VmOpcode::End);
    compare(bounds, slot, 7, 0, 7);
    const uint8_t edge[] = {op(VmOpcode::TpIf),0,255,0,255,0,0,0,2,op(VmOpcode::End)};
    std::memset(slot, 0, sizeof(slot)); std::memcpy(slot, edge, sizeof(edge));
    compare(bounds, slot, To1D(255,255), 0, To1D(0,2));
    const uint8_t jump[] = {op(VmOpcode::If),0,0,0,1,255,op(VmOpcode::TpIf),0,1,0,1,0,0,0,0,op(VmOpcode::End)};
    std::memcpy(slot, jump, sizeof(jump));
    compare(bounds, slot, To1D(1,1), 0, To1D(1,1));
    compare(bounds, slot, To1D(1,1), 1, To1D(0,0));
    slot[2] = slot[3] = 255; // Out-of-range flags are treated as clear.
    slot[5] = 9;
    compare(bounds, slot, To1D(1,1), 1, To1D(1,1));
    slot[1] = 1;
    compare(bounds, slot, To1D(1,1), 1, To1D(0,0));
    slot[0] = op(VmOpcode::TpIf);
    slot[1] = op(VmOpcode::End); // An operand sentinel never ends the scan.
    std::memset(slot+2, 0, 126);
    compare(bounds, slot, 7, 0, 7, false);
    std::memset(slot, 0, sizeof(slot));
    slot[0] = op(VmOpcode::Msg); slot[3] = op(VmOpcode::End);
    ScriptVm unsupported = {};
    unsupported.initWarpVM(slot, 0, 0);
    bounds.assert(unsupported.valid, false, "full-only opcode rejected before execution");
    const uint8_t shortRun[] = {op(VmOpcode::If),0,0,0,1,0,op(VmOpcode::End)};
    std::memcpy(slot, shortRun, sizeof(shortRun));
    compare(bounds, slot, 7, 0, 7, true, 3);
    suite.addTest(bounds);

    Test cap("warp runner command cap");
    // A normal128B warp stream cannot contain30 six-byte If commands. Widen
    // only the runner fixture to exercise the independent execution cap.
    uint8_t longSlot[190] = {};
    for (uint16_t at = 0; at < 180; at += 6) longSlot[at] = op(VmOpcode::If);
    const uint8_t teleport[] = {op(VmOpcode::TpIf),0,1,0,1,0,0,0,0,op(VmOpcode::End)};
    std::memcpy(longSlot+180, teleport, sizeof(teleport));
    compare(cap, longSlot, To1D(1,1), 0, To1D(1,1), true, 189);
    suite.addTest(cap);
    FLAG_BIT_ARRAY[0] = 0;
    runner.addTestSuite(suite);
}
