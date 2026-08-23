#pragma once

#include <stdint.h>

struct FlashBackend {
    void (*eraseSector)(uint16_t page);
    void (*writePage)(uint16_t page, const uint8_t *data);
    void (*writeBytes)(uint32_t addr, const uint8_t *data, uint16_t len);
    void (*readBytes)(uint32_t addr, uint8_t *data, uint16_t len);
    bool (*busy)(void);
};

#ifdef TEST
namespace FX {
uint8_t loadGameState(uint8_t *gameState, uint16_t size);
void saveGameState(const uint8_t *gameState, uint16_t size);
}

extern FlashBackend flash;

void flashFakeReset();
const uint8_t *flashFakeData();
void flashFakeSetBytes(uint32_t addr, const uint8_t *data, uint16_t len);
void flashFakeSetWriteLimit(int32_t bytes);
void flashFakeSetBusy(bool value);
uint32_t flashFakeReadCount();
void flashFakeResetReadCount();
#else
#include <ArduboyFX.h>

namespace flash_backend_detail {
inline void eraseSector(uint16_t page)
{
    FX::eraseSaveBlock(page);
}

inline void writePage(uint16_t page, const uint8_t *data)
{
    FX::writeSavePage(page, const_cast<uint8_t *>(data));
}

inline void writeBytes(uint32_t addr, const uint8_t *data, uint16_t len)
{
    while (len != 0) {
        const uint16_t pageOffset = static_cast<uint16_t>(addr & 0xff);
        uint16_t chunk = static_cast<uint16_t>(256 - pageOffset);
        if (chunk > len) {
            chunk = len;
        }

        FX::writeEnable();
        FX::seekCommand(SFC_WRITE,
                        (static_cast<uint24_t>(FX::programSavePage) << 8) + addr);
        for (uint16_t i = 0; i < chunk; ++i) {
            FX::writeByte(data[i]);
        }
        FX::disable();
        FX::waitWhileBusy();

        addr += chunk;
        data += chunk;
        len -= chunk;
    }
}

inline void readBytes(uint32_t addr, uint8_t *data, uint16_t len)
{
    FX::readSaveBytes(addr, data, len);
}

inline bool busy()
{
    FX::enable();
    FX::writeByte(SFC_READSTATUS1);
    const bool result = (FX::readByte() & 1) != 0;
    FX::disable();
    return result;
}
} // namespace flash_backend_detail

inline FlashBackend flash = {
    flash_backend_detail::eraseSector,
    flash_backend_detail::writePage,
    flash_backend_detail::writeBytes,
    flash_backend_detail::readBytes,
    flash_backend_detail::busy,
};
#endif
