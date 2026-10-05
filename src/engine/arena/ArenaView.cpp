#include "ArenaView.hpp"

#include "../../common.hpp"

namespace arena {

void draw(const ArenaUiState &ui, const ArenaContext &context)
{
    arduboy.clear();
    // Lifecycle spike marker; Luna replaces this with the cached 1bpp view.
    arduboy.drawPixel(static_cast<int16_t>(ui.screen) * 4 + context.playerTeam,
                      context.opponentTeam, WHITE);
}

} // namespace arena
