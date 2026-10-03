#pragma once

#include <stdint.h>

#include "TileProps.hpp"

// The rendered map cache stores six property bits and one GID-present bit for
// each cell. It never stores sprite IDs or graphics. The extra bit is needed
// because an empty GID and a non-walkable wall both have zero properties.
class TilePropertyWindow {
  public:
    static constexpr uint8_t WINDOW_WIDTH = 9;
    static constexpr uint8_t WINDOW_HEIGHT = 5;
    static constexpr uint8_t PROPERTY_BITS = 6;
    static constexpr uint8_t CELL_BITS = PROPERTY_BITS + 1;
    static constexpr uint8_t CELL_COUNT = WINDOW_WIDTH * WINDOW_HEIGHT;
    static constexpr uint8_t DATA_BYTES = (CELL_COUNT * CELL_BITS + 7) / 8;
    static constexpr uint8_t ORIGIN_X_OFFSET = DATA_BYTES;
    static constexpr uint8_t ORIGIN_Y_OFFSET = DATA_BYTES + 1;
    static constexpr uint8_t VALID_ROWS_OFFSET = DATA_BYTES + 2;
    static constexpr uint8_t STORAGE_BYTES = DATA_BYTES + 3;
    static constexpr uint8_t OCCUPIED_BIT = 1u << PROPERTY_BITS;
    static constexpr uint8_t ORIGIN_VALID_BIT = 1u << 7;

    explicit TilePropertyWindow(uint8_t *storage)
        : storage_(storage), mutableStorage_(storage) {}
    explicit TilePropertyWindow(const uint8_t *storage)
        : storage_(storage), mutableStorage_(nullptr) {}

    static void invalidate(uint8_t *storage) {
        storage[VALID_ROWS_OFFSET] = 0;
    }

    // Origin coordinates cover the drawable window at every map edge:
    // X ranges from -3 through 252 and Y from -2 through 253.
    void begin(int16_t originX, int16_t originY) {
        if (mutableStorage_ == nullptr) return;
        mutableStorage_[VALID_ROWS_OFFSET] = 0;
        if (originX < -3 || originX > 252 || originY < -2 || originY > 253) {
            return;
        }
        mutableStorage_[ORIGIN_X_OFFSET] = static_cast<uint8_t>(originX + 3);
        mutableStorage_[ORIGIN_Y_OFFSET] = static_cast<uint8_t>(originY + 2);
        mutableStorage_[VALID_ROWS_OFFSET] = ORIGIN_VALID_BIT;
    }

    void writeRow(uint8_t row, const uint16_t words[WINDOW_WIDTH]) {
        if (mutableStorage_ == nullptr) return;
        if (row >= WINDOW_HEIGHT ||
            (mutableStorage_[VALID_ROWS_OFFSET] & ORIGIN_VALID_BIT) == 0) return;
        for (uint8_t col = 0; col < WINDOW_WIDTH; ++col) {
            const uint16_t word = words[col];
            const uint8_t value = static_cast<uint8_t>(
                TileProps::tileProps(word) |
                (TileProps::tileId(word) != 0 ? OCCUPIED_BIT : 0));
            writeCell(static_cast<uint8_t>(row * WINDOW_WIDTH + col), value);
        }
        mutableStorage_[VALID_ROWS_OFFSET] |= static_cast<uint8_t>(1u << row);
    }

    bool lookup(int16_t worldX, int16_t worldY, uint8_t &properties,
                bool &occupied) const {
        const int16_t originX = static_cast<int16_t>(storage_[ORIGIN_X_OFFSET]) - 3;
        const int16_t originY = static_cast<int16_t>(storage_[ORIGIN_Y_OFFSET]) - 2;
        const int16_t col = worldX - originX;
        const int16_t row = worldY - originY;
        if ((storage_[VALID_ROWS_OFFSET] & ORIGIN_VALID_BIT) == 0 ||
            col < 0 || col >= WINDOW_WIDTH || row < 0 || row >= WINDOW_HEIGHT ||
            (storage_[VALID_ROWS_OFFSET] & static_cast<uint8_t>(1u << row)) == 0) {
            return false;
        }

        const uint8_t value =
            readCell(static_cast<uint8_t>(row * WINDOW_WIDTH + col));
        properties = value & TileProps::TILE_PROP_BITS;
        occupied = (value & OCCUPIED_BIT) != 0;
        return true;
    }

  private:
    const uint8_t *storage_;
    uint8_t *mutableStorage_;

    void writeCell(uint8_t cell, uint8_t value) const {
        const uint16_t bitOffset = static_cast<uint16_t>(cell) * CELL_BITS;
        const uint8_t byteOffset = static_cast<uint8_t>(bitOffset >> 3);
        const uint8_t shift = static_cast<uint8_t>(bitOffset & 7);
        uint16_t pair = static_cast<uint16_t>(mutableStorage_[byteOffset]) |
                        (static_cast<uint16_t>(mutableStorage_[byteOffset + 1]) << 8);
        const uint16_t mask = static_cast<uint16_t>((1u << CELL_BITS) - 1u) << shift;
        pair = static_cast<uint16_t>((pair & ~mask) |
                                     ((static_cast<uint16_t>(value) << shift) & mask));
        mutableStorage_[byteOffset] = static_cast<uint8_t>(pair & 0xFF);
        mutableStorage_[byteOffset + 1] = static_cast<uint8_t>(pair >> 8);
    }

    uint8_t readCell(uint8_t cell) const {
        const uint16_t bitOffset = static_cast<uint16_t>(cell) * CELL_BITS;
        const uint8_t byteOffset = static_cast<uint8_t>(bitOffset >> 3);
        const uint8_t shift = static_cast<uint8_t>(bitOffset & 7);
        const uint16_t pair = static_cast<uint16_t>(storage_[byteOffset]) |
                              (static_cast<uint16_t>(storage_[byteOffset + 1]) << 8);
        return static_cast<uint8_t>((pair >> shift) & ((1u << CELL_BITS) - 1u));
    }
};

static_assert(TilePropertyWindow::DATA_BYTES == 40,
              "tile properties and GID presence need 40 packed bytes");
static_assert(TilePropertyWindow::STORAGE_BYTES == 43,
              "tile property window includes two origin bytes and a row mask");
