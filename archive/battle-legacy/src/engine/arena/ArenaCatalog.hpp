#pragma once

#include "ArenaTypes.hpp"

namespace arena {

struct Member {
    uint8_t species;
    uint8_t level;
    uint8_t moveIds[4];
};

static_assert(sizeof(Member) == 6, "arena catalog member must remain six bytes");

bool readPlayerMember(uint8_t team, uint8_t slot, Member &out);
bool readOpponentId(uint8_t opponent, uint8_t &out);
bool loadPreview(ArenaScreen screen, uint8_t index, ArenaPreview &out);

} // namespace arena
