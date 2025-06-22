#include "ScriptVM.hpp"
#include "opcodes.hpp"
#include "../macros.hpp"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

void ScriptVm::run() {
    // get the current command
    VmOpcode byte = static_cast<VmOpcode>(*this->ptr);
    for (auto i = 0; i < 30; i++) {
        this->ptr++;
        printd("running command %d at pointer %d\n", byte, this->ptr - this->buffer);
        switch (byte) {
        case VmOpcode::Msg: {
            break;
        }
        case VmOpcode::TMsg: {
            break;
        }
        case VmOpcode::Tp: {
            uint16_t x = readUInt16();
            uint16_t y = readUInt16();
            uint16_t end = To1D(x, y);
            printd("tp from %hu to %hu, player loc %d\n", this->state->playerLocation, end, this->state->playerLocation);
            this->state->playerLocation = end;
            break;
        }
        case VmOpcode::TpIf: {
            uint16_t x = readUInt16();
            uint16_t y = readUInt16();
            uint16_t start = To1D(x, y);
            x = readUInt16();
            y = readUInt16();
            uint16_t end = To1D(x, y);
            printd("tp from %hu to %hu, player loc %d\n", start, end, this->state->playerLocation);

            if (this->state->playerLocation == start) {
                printd("teleportin\n");
                this->state->playerLocation = end;
            }
            break;
        }
        case VmOpcode::If: {
            printd("if\n");
            uint8_t condType = readUInt8();
            switch (condType) {
            case 0:
            case 1: {   // if then else
                uint16_t flag = readUInt16();
                uint8_t ifType = readUInt8();
                uint8_t jump = readUInt8();
                printd("if type: %d\n", ifType);
                printd("flag check, flag %d, jump %d\n", flag, jump);
                bool set = this->state->getFlag(flag);
                printd("flag set: %d\n", set);
                if ((condType == 0 && !set) || (condType == 1 && set)) {
                    printd("jumping %d\n", jump);
                    this->ptr += jump;
                    printd("pointer at %d\n", this->ptr - this->buffer);
                }
            }
            }
            break;
        }

        case VmOpcode::End: {
            // reset the pointer to the start of the buffer
            this->ptr = this->buffer;
            printd("\n");
            return;
        }
        }
        byte = static_cast<VmOpcode>(*this->ptr);
    }
}

uint16_t ScriptVm::readUInt16() {
    uint16_t val = (static_cast<uint16_t>(this->ptr[0]) << 8 | this->ptr[1]);
    printd("readInt16: %d: from values %hu, %hu\n", int(val), this->ptr[0], this->ptr[1]);
    this->ptr += 2;
    return val;
}

uint8_t ScriptVm::readUInt8() {
    uint8_t val = this->ptr[0];
    printd("readUInt8: %d: from value %hu\n", int(val), this->ptr[0]);
    this->ptr += 1;
    return val;
}