#include "ListView.hpp"

void listViewMove(ListView &view, int8_t delta) {
    if (view.itemCount == 0 || delta == 0) return;

    int16_t next = static_cast<int16_t>(view.cursor) + delta;
    if (next < 0) next = 0;
    if (next >= view.itemCount) next = static_cast<int16_t>(view.itemCount) - 1;
    view.cursor = static_cast<uint8_t>(next);

    if (view.rows == 0) {
        view.windowStart = view.cursor;
        return;
    }

    if (view.rows >= view.itemCount) {
        view.windowStart = 0;
    } else if (view.cursor < view.windowStart) {
        view.windowStart = view.cursor;
    } else if (static_cast<uint16_t>(view.cursor) >=
               static_cast<uint16_t>(view.windowStart) + view.rows) {
        view.windowStart = static_cast<uint8_t>(view.cursor - view.rows + 1);
    }
}

uint8_t listViewItemAt(const ListView &view, uint8_t row) {
    if (row >= view.rows) return 0;
    const uint16_t item = static_cast<uint16_t>(view.windowStart) + row;
    return item < view.itemCount ? static_cast<uint8_t>(item) : 0;
}

bool listViewVisible(const ListView &view, uint8_t row) {
    return row < view.rows &&
           static_cast<uint16_t>(view.windowStart) + row < view.itemCount;
}
