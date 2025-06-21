#include "ScriptVM.hpp"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

void ScriptVm::run() {
    // get the current command
    SciptCommands byte = static_cast<SciptCommands>(*this->ptr);
    this->ptr++;
    while (byte != SciptCommands::cmd_end) {
        printf("running command %d\n", byte);

        switch (byte) {
        case SciptCommands::cmd_msg: {
            break;
        }
        case SciptCommands::cmd_tmsg: {
            break;
        }
        case SciptCommands::cmd_tp: {
            uint16_t x = readInt16();
            uint16_t y = readInt16();
            uint16_t start = To1D(x, y);
            x = readInt16();
            y = readInt16();
            uint16_t end = To1D(x, y);
            printf("tp from %hu to %hu, player loc %d\n", start, end, this->state->playerLocation);

            if (this->state->playerLocation == start) {
                printf("teleportin");
                this->state->playerLocation = end;
            }
            break;
        }

        case SciptCommands::cmd_end: {
            break;
        }
        }
        byte = static_cast<SciptCommands>(*this->ptr);
    }
    // reset the pointer to the start of the buffer
    this->ptr = this->buffer;
}

uint16_t ScriptVm::readInt16() {
    uint16_t val = (static_cast<uint16_t>(this->ptr[0]) << 8 | this->ptr[1]);
    printf("readInt16: %d: from values %hu, %hu\n", int(val), this->ptr[0], this->ptr[1]);
    this->ptr += 2;
    return val;
}