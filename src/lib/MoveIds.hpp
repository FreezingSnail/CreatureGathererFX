#pragma once
#include <stdint.h>

// Keep historical saved/arena/preset empty 32 readable. Semantic Deluge 44
// retains its original packed row 32; the gap at authored 35 stays explicit.
constexpr uint8_t EMPTY_MOVE_ID = 255;
constexpr uint8_t LEGACY_EMPTY_MOVE_ID = 32;
constexpr uint8_t DELUGE_MOVE_ID = 44;

inline bool validMoveId(uint8_t id)
{
    return id < 45 && id != LEGACY_EMPTY_MOVE_ID && id != 35;
}

inline uint8_t battleMoveId(uint8_t id)
{
    return validMoveId(id) ? id : EMPTY_MOVE_ID;
}

inline uint8_t moveRecordIndex(uint8_t id)
{
    if (!validMoveId(id)) return EMPTY_MOVE_ID;
    if (id == DELUGE_MOVE_ID) return 32;
    return id >= 36 ? static_cast<uint8_t>(id - 1) : id;
}
