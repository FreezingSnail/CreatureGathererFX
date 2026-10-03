#include "ScriptVM.hpp"
#include "opcodes.hpp"
#include "../macros.hpp"
#include "../globals.hpp"
#include "../flags/flag_bit_array.hpp"
#include <stdint.h>

#ifdef TEST
extern uint16_t readScriptTextCountForTest();
#else
#include "../fxdata.h"
// ReadFXu16 uses little endian framing for the raw map text table. Script
// operands are decoded separately as big endian by readUInt16().
uint16_t ReadFXu16(uint24_t addr);
#endif

void ScriptVm::initVM(uint8_t *script, uint16_t current, uint16_t target) {
    this->base = script;
    this->ptr = script;
    this->endPtr = script;
    this->currentTile = current;
    this->targetTile = target;
    this->valid = false;

    if (script == nullptr) return;

    // Find End only at command boundaries. Operand bytes may themselves be
    // 0xFF (for example, coordinate 255), so scanning for the byte value alone
    // would truncate a valid slot.
    uint8_t offset = 0;
    while (offset < 128) {
        const uint8_t commandOffset = offset;
        const VmOpcode opcode = static_cast<VmOpcode>(script[offset++]);
        uint8_t operandBytes = 0;
        switch (opcode) {
        case VmOpcode::End:
            this->endPtr = script + commandOffset;
            this->valid = true;
            return;
        case VmOpcode::Msg:
            operandBytes = 2;
            break;
        case VmOpcode::TMsg:
        case VmOpcode::SMsg:
            operandBytes = 6;
            break;
        case VmOpcode::Tp:
            operandBytes = 4;
            break;
        case VmOpcode::TpIf:
            operandBytes = 8;
            break;
        case VmOpcode::If:
            operandBytes = 5;
            break;
        case VmOpcode::SetFlag:
        case VmOpcode::UnsetFlag:
        case VmOpcode::ReadFlag:
            operandBytes = 2;
            break;
        default:
            return;
        }
        if (operandBytes > 128 - offset) return;
        offset += operandBytes;
    }
}

static PopUpDialog scriptTextDialog(uint16_t index) {
    PopUpDialog dialog = {};
    dialog.x = 0;
    dialog.y = 43;
    dialog.width = 128;
    dialog.height = 24;
    dialog.textAddress = index;
    dialog.type = SCRIPT_TEXT;
    return dialog;
}

static bool validScriptTextIndex(uint16_t index) {
#ifdef TEST
    return index < readScriptTextCountForTest();
#else
    const uint16_t count = ReadFXu16(raw_map_text);
    FxReadCounter::transitionExact(1);
    return index < count;
#endif
}

static void queueScriptText(uint16_t index) {
    if (validScriptTextIndex(index)) {
        dialogMenu.pushMenu(scriptTextDialog(index));
    }
}

static bool validFlagIndex(uint16_t index) {
    return index < static_cast<uint16_t>(sizeof(FLAG_BIT_ARRAY) * 8);
}

void ScriptVm::run() {
    if (!this->valid || this->base == nullptr) return;

    for (uint8_t commands = 0; commands < 30; ++commands) {
        if (this->ptr >= this->endPtr) {
            this->end();
            return;
        }
        VmOpcode byte = static_cast<VmOpcode>(*this->ptr++);
        switch (byte) {
        case VmOpcode::Msg: {
            const uint16_t textIndex = readUInt16();
            if (this->ptr == nullptr) {
                this->end();
                return;
            }
            queueScriptText(textIndex);
            break;
        }
        case VmOpcode::TMsg: {
            const uint16_t x = readUInt16();
            const uint16_t y = readUInt16();
            const uint16_t textIndex = readUInt16();
            if (this->ptr == nullptr) {
                this->end();
                return;
            }
            if (x < MAP_WIDTH && y < MAP_HEIGHT && To1D(x, y) == this->targetTile) {
                queueScriptText(textIndex);
            }
            break;
        }
        case VmOpcode::SMsg: {
            const uint16_t x = readUInt16();
            const uint16_t y = readUInt16();
            const uint16_t textIndex = readUInt16();
            if (this->ptr == nullptr) {
                this->end();
                return;
            }
            if (x < MAP_WIDTH && y < MAP_HEIGHT && To1D(x, y) == this->currentTile) {
                queueScriptText(textIndex);
            }
            break;
        }
        case VmOpcode::Tp: {
            uint16_t x = readUInt16();
            uint16_t y = readUInt16();
            if (this->ptr == nullptr) {
                this->end();
                return;
            }
            uint16_t end = To1D(x, y);
            gameState.playerLocation = end;
            this->end();
            return;
        }
        case VmOpcode::TpIf: {
            uint16_t x = readUInt16();
            uint16_t y = readUInt16();
            uint16_t start = To1D(x, y);
            x = readUInt16();
            y = readUInt16();
            if (this->ptr == nullptr) {
                this->end();
                return;
            }
            uint16_t end = To1D(x, y);

            if (gameState.playerLocation == start) {
                gameState.playerLocation = end;
                this->end();
                return;
            }
            break;
        }
        case VmOpcode::If: {
            uint8_t condType = readUInt8();
            if (this->ptr == nullptr) {
                this->end();
                return;
            }
            switch (condType) {
            case 0:
            case 1: {   // if then else
                uint16_t flag = readUInt16();
                uint8_t ifType = readUInt8();
                uint8_t jump = readUInt8();
                if (this->ptr == nullptr) {
                    this->end();
                    return;
                }
                bool set = validFlagIndex(flag) && gameState.getFlag(flag);
                if ((condType == 0 && !set) || (condType == 1 && set)) {
                    if (static_cast<uint16_t>(this->ptr - this->base) + jump >
                        static_cast<uint16_t>(this->endPtr - this->base)) {
                        this->end();
                        return;
                    }
                    this->ptr += jump;
                }
            }
            }
            break;
        }

        case VmOpcode::SetFlag: {
            const uint16_t flag = readUInt16();
            if (this->ptr == nullptr) {
                this->end();
                return;
            }
            if (validFlagIndex(flag)) gameState.setFlag(flag);
            break;
        }

        case VmOpcode::UnsetFlag: {
            const uint16_t flag = readUInt16();
            if (this->ptr == nullptr) {
                this->end();
                return;
            }
            if (validFlagIndex(flag)) gameState.clearFlag(flag);
            break;
        }

        case VmOpcode::ReadFlag: {
            // ReadFlag is a two-byte flag operand. Conditional branches use
            // their own encoded flag check; preserve this command's operand
            // consumption until the VM exposes a result register.
            (void)readUInt16();
            if (this->ptr == nullptr) {
                this->end();
                return;
            }
            break;
        }

        case VmOpcode::End: {
            end();
            return;
        }
        default:
            this->end();
            return;
        }
    }
    this->end();
}

uint16_t ScriptVm::readUInt16() {
    if (this->ptr == nullptr || this->ptr > this->endPtr ||
        static_cast<uint8_t>(this->endPtr - this->ptr) < 2) {
        this->ptr = nullptr;
        return 0;
    }
    uint16_t val = (static_cast<uint16_t>(this->ptr[0]) << 8) | this->ptr[1];
    this->ptr += 2;
    return val;
}

uint8_t ScriptVm::readUInt8() {
    if (this->ptr == nullptr || this->ptr > this->endPtr ||
        static_cast<uint8_t>(this->endPtr - this->ptr) < 1) {
        this->ptr = nullptr;
        return 0;
    }
    uint8_t val = this->ptr[0];
    this->ptr += 1;
    return val;
}

void ScriptVm::end() {
    this->ptr = this->base;
}
