#pragma once
#include <cstdint>
// Auto-generated – DO NOT EDIT

enum class VmOpcode : uint8_t {
    Msg = 0,
    TMsg = 1,
    Tp = 2,
    TpIf = 3,
    If = 4,
    SetFlag = 5,
    UnsetFlag = 6,
    ReadFlag = 7,
    End = 255,
};
