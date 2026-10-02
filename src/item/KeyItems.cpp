#include "KeyItems.hpp"

namespace item {

void keyItemsClear(KeyItems &k) {
    for (uint8_t i = 0; i < sizeof(k.bits); ++i) k.bits[i] = 0;
}

bool keyItemUnlocked(const KeyItems &k, uint8_t id) {
    if (id >= KEY_ITEM_COUNT) return false;
    return (k.bits[id >> 3] & static_cast<uint8_t>(1U << (id & 7))) != 0;
}

void keyItemUnlock(KeyItems &k, uint8_t id) {
    if (id >= KEY_ITEM_COUNT) return;
    k.bits[id >> 3] |= static_cast<uint8_t>(1U << (id & 7));
}

}  // namespace item
