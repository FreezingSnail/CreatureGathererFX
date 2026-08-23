#include "../../src/save/FlashBackend.hpp"

#include <string.h>

namespace {
constexpr uint16_t FLASH_FAKE_BYTES = 8192;
constexpr uint16_t SAVE_SECTOR_BYTES = 4096;
uint8_t storage[FLASH_FAKE_BYTES];
int32_t writeLimit = -1;
bool busyFlag = false;
uint32_t readCount = 0;

bool rangeValid(uint32_t addr, uint16_t len)
{
    return addr <= FLASH_FAKE_BYTES && len <= FLASH_FAKE_BYTES - addr;
}

void eraseSector(uint16_t page)
{
    const uint32_t addr = static_cast<uint32_t>(page) * 256;
    if (!rangeValid(addr, SAVE_SECTOR_BYTES)) {
        return;
    }
    for (uint16_t i = 0; i < SAVE_SECTOR_BYTES; ++i) {
        if (writeLimit == 0) {
            return;
        }
        storage[addr + i] = 0xff;
        if (writeLimit > 0) {
            --writeLimit;
        }
    }
}

void writeBytes(uint32_t addr, const uint8_t *data, uint16_t len)
{
    if (!rangeValid(addr, len)) {
        return;
    }
    for (uint16_t i = 0; i < len; ++i) {
        if (writeLimit == 0) {
            return;
        }
        storage[addr + i] &= data[i];
        if (writeLimit > 0) {
            --writeLimit;
        }
    }
}

void writePage(uint16_t page, const uint8_t *data)
{
    writeBytes(static_cast<uint32_t>(page) * 256, data, 256);
}

void readBytes(uint32_t addr, uint8_t *data, uint16_t len)
{
    ++readCount;
    if (!rangeValid(addr, len)) {
        memset(data, 0xff, len);
        return;
    }
    memcpy(data, storage + addr, len);
}

bool busy()
{
    return busyFlag;
}

uint16_t read16(uint16_t addr)
{
    return static_cast<uint16_t>(storage[addr] << 8) |
           static_cast<uint16_t>(storage[addr + 1]);
}
} // namespace

FlashBackend flash = {eraseSector, writePage, writeBytes, readBytes, busy};

void flashFakeReset()
{
    memset(storage, 0xff, sizeof(storage));
    writeLimit = -1;
    busyFlag = false;
    readCount = 0;
}

const uint8_t *flashFakeData()
{
    return storage;
}

void flashFakeSetBytes(uint32_t addr, const uint8_t *data, uint16_t len)
{
    if (rangeValid(addr, len)) {
        memcpy(storage + addr, data, len);
    }
}

void flashFakeSetWriteLimit(int32_t bytes)
{
    writeLimit = bytes;
}

void flashFakeSetBusy(bool value)
{
    busyFlag = value;
}

uint32_t flashFakeReadCount()
{
    return readCount;
}

void flashFakeResetReadCount()
{
    readCount = 0;
}

namespace FX {
uint8_t loadGameState(uint8_t *gameState, uint16_t size)
{
    uint16_t addr = 0;
    uint8_t found = 0;
    while (addr + 2 <= SAVE_SECTOR_BYTES && read16(addr) == size) {
        if (addr + 2 + size > SAVE_SECTOR_BYTES) {
            break;
        }
        memcpy(gameState, storage + addr + 2, size);
        found = 1;
        addr = static_cast<uint16_t>(addr + 2 + size);
    }
    return found;
}

void saveGameState(const uint8_t *gameState, uint16_t size)
{
    uint16_t addr = 0;
    while (addr + 2 <= SAVE_SECTOR_BYTES && read16(addr) == size) {
        addr = static_cast<uint16_t>(addr + 2 + size);
    }
    if (addr + 2 + size > SAVE_SECTOR_BYTES - 2) {
        eraseSector(0);
        addr = 0;
    }

    const uint8_t header[2] = {
        static_cast<uint8_t>(size >> 8),
        static_cast<uint8_t>(size),
    };
    writeBytes(addr, header, sizeof(header));
    writeBytes(addr + 2, gameState, size);
}
} // namespace FX

namespace {
struct FlashFakeInitializer {
    FlashFakeInitializer() { flashFakeReset(); }
} flashFakeInitializer;
} // namespace
