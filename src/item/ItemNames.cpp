#include "ItemNames.hpp"

#ifdef TEST
#include "../../tst/src/FXDataFake.hpp"
#else
#include <ArduboyFX.h>
#endif
#include "../fxdata.h"

namespace item {

ItemName itemNameAddr(ItemKind kind, uint8_t id)
{
    ItemName name = {};
    if (kind == ItemKind::Lure && id < LURE_COUNT) {
        name.part[0] = FX::readIndexedUInt24(LureTierNames::LureTierNames, lureTierOf(id));
        name.part[1] = FX::readIndexedUInt24(LureTypeNames::LureTypeNames, lureTypeOf(id));
        name.parts = 2;
    } else if (kind == ItemKind::Consumable && id < CONSUMABLE_COUNT) {
        name.part[0] = FX::readIndexedUInt24(ConsumableNames::ConsumableNames, id);
        name.parts = 1;
    }
    return name;
}

} // namespace item
