#include "ConsumableDef.hpp"

#ifdef TEST
#include "../../tst/src/FXDataFake.hpp"
#else
#include <ArduboyFX.h>
#endif
#include "../fxdata.h"

namespace item {

ConsumableDef readConsumableDef(uint8_t id) {
    if (id >= CONSUMABLE_COUNT) {
        return {static_cast<uint8_t>(ConsumableKind::None), 0};
    }

    uint8_t bytes[sizeof(ConsumableDef)];
    const uint24_t address = consumable_table + static_cast<uint16_t>(id) * sizeof(ConsumableDef);
    FX::readDataBytes(address, bytes, sizeof(bytes));
    return {bytes[0], bytes[1]};
}

}  // namespace item
