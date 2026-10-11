#include "LureZone.hpp"

#include "../PpGlyph.hpp"
#include "../../lib/Blit.hpp"
#include "../../lib/Text.hpp"
#include "../../fxdata.h"

namespace {

// Same fixed glyph counts as InventoryMenu so the shared name sprites line up.
uint8_t lurePartWidth(uint8_t id, uint8_t part) {
    static const uint8_t tierChars[] = {5, 4, 4};
    static const uint8_t typeChars[] = {6, 5, 4, 4, 4, 9, 5, 5};
    if (id >= item::LURE_COUNT) return 0;
    const uint8_t chars = part == 0 ? tierChars[item::lureTierOf(id)]
                                    : typeChars[item::lureTypeOf(id)];
    return static_cast<uint8_t>((chars + 1) * 5);
}

void drawPicker() {
    Blit::fillRect(0, 0, 128, 64, BLACK);
    drawText(0, 0, LureZone::pickerFeedback() ? lureTextSet : lureTextPrompt, 45, 0);

    if (LureZone::pickerCount() == 0) {
        drawText(0, 24, lureTextEmpty, 45, 0);
    } else {
        const uint8_t cursor = LureZone::pickerCursor();
        const uint8_t window = LureZone::pickerWindow();
        for (uint8_t row = 0; row < LureZone::PICKER_ROWS; ++row) {
            uint8_t id;
            uint8_t parts;
            uint24_t name0;
            uint24_t name1;
            if (!LureZone::pickerRow(row, id, parts, name0, name1)) continue;
            const uint8_t y = static_cast<uint8_t>(12 + row * 9);
            const uint8_t frame = static_cast<uint8_t>(window + row) == cursor ? 0 : 1;
            PpGlyph::number(1, y + 1,
                            item::inventoryCount(LureZone::inventory(),
                                                 item::ItemKind::Lure, id));
            drawText(17, y, name0, lurePartWidth(id, 0), frame);
            if (parts > 1) drawText(66, y, name1, lurePartWidth(id, 1), frame);
        }
    }
    drawText(0, 56, lureTextFooter, 65, 0);
}

void drawMessage() {
    drawText(2, 56, LureZone::messageAddr(), LureZone::messageWidth(), 0);
}

} // namespace

namespace LureZone {

void drawOverlay() {
    if (pickerOpen()) {
        drawPicker();
        return;
    }
    if (messageActive()) drawMessage();
}

} // namespace LureZone
