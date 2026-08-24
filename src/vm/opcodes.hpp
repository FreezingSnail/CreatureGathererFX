#pragma once
#include <stdint.h>
// Auto-generated – DO NOT EDIT

enum class VmOpcode : uint8_t {
    Msg = 0,
    TMsg = 1,
    SMsg = 2,
    Tp = 3,
    TpIf = 4,
    If = 5,
    SetFlag = 6,
    UnsetFlag = 7,
    ReadFlag = 8,
    End = 255,
};
