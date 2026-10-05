#pragma once

#include "ArenaTypes.hpp"

#if defined(CGFX_ARENA_DEMO) || defined(TEST) || defined(FX_READ_COUNTER)
namespace arena {

extern ArenaContext arenaContext;

void boot();
void update(uint8_t edgeButtons);
void draw();
void finishBattle(battle::Outcome outcome);
bool startMatch();

} // namespace arena
#endif
