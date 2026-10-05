#include "ArenaDemo.hpp"

#if defined(CGFX_ARENA_DEMO) || defined(TEST) || defined(FX_READ_COUNTER)

#include "../ModeState.hpp"
#include "../game/Gamestate.hpp"
#include "../menu/MenuV2.hpp"
#include "../../globals.hpp"
#include "../../player/Player.hpp"
#include "../menu/MenuNav.hpp"
#include "../battle/BattlePresets.hpp"
#include "../battle/BattleSetup.hpp"
#include "ArenaView.hpp"

namespace arena {

ArenaContext arenaContext = {};

namespace {

const BattlePresets::Preset &playerPreset(uint8_t index)
{
    return index == 0 ? BattlePresets::opening : BattlePresets::switch_drill;
}

const BattlePresets::Preset &opponentPreset(uint8_t index)
{
    return index == 0 ? BattlePresets::opening : BattlePresets::switch_drill;
}

uint8_t edgeDirection(uint8_t buttons)
{
    if (buttons & MENU_NAV_UP) return 0;
    if (buttons & MENU_NAV_DOWN) return 1;
    return 0xff;
}

void moveCursor(uint8_t direction, uint8_t count)
{
    if (direction == 0xff || count == 0) return;
    ListView &list = modeState.arena.ui.list;
    listViewMove(list, direction == 0 ? -1 : 1);
}

} // namespace

void boot()
{
    arenaContext = {0, 0, battle::Outcome::None};
    enterArena();
    ArenaUiState &ui = modeState.arena.ui;
    ui = {};
    ui.screen = ArenaScreen::PlayerTeam;
    ui.list = {2, 1, 0, 0};
    gameState.state = GameState_t::ARENA;
}

bool startMatch()
{
    if (arenaContext.playerTeam >= 2 || arenaContext.opponentTeam >= 2) return false;

    const BattlePresets::Preset &storedPlayer = playerPreset(arenaContext.playerTeam);
    const BattlePresets::Preset &storedOpponent = opponentPreset(arenaContext.opponentTeam);
    const BattlePresets::Preset opponent = BattlePresets::copyPreset(storedOpponent);

    menu.clear();
    dialogMenu.clear();
    battle::applyPlayerPreset(player, storedPlayer);
    enterBattle();
    battleSession().beginTrainer(opponent.trainerId);
    if (!battleSession().isActive()) {
        enterArena();
        gameState.state = GameState_t::ARENA;
        return false;
    }
    gameState.state = GameState_t::BATTLE;
    return true;
}

void finishBattle(battle::Outcome outcome)
{
    arenaContext.outcome = outcome;
    enterArena();
    ArenaUiState &ui = modeState.arena.ui;
    ui = {};
    ui.screen = ArenaScreen::Result;
    ui.list = {3, 3, 0, 0};
    gameState.state = GameState_t::ARENA;
}

void update(uint8_t edgeButtons)
{
    ArenaUiState &ui = modeState.arena.ui;
    const uint8_t direction = edgeDirection(edgeButtons);
    if (direction != 0xff) {
        moveCursor(direction, ui.list.itemCount);
        return;
    }
    if ((edgeButtons & MENU_EDGE_A) == 0) return;

    if (ui.screen == ArenaScreen::PlayerTeam) {
        arenaContext.playerTeam = ui.list.cursor;
        ui.screen = ArenaScreen::OpponentTeam;
        ui.list = {2, 1, 0, arenaContext.opponentTeam};
        return;
    }
    if (ui.screen == ArenaScreen::OpponentTeam) {
        arenaContext.opponentTeam = ui.list.cursor;
        startMatch();
        return;
    }
    switch (ui.list.cursor) {
    case 0:
        startMatch();
        return;
    case 1:
        ui.screen = ArenaScreen::OpponentTeam;
        ui.list = {2, 1, 0, arenaContext.opponentTeam};
        return;
    default:
        ui.screen = ArenaScreen::PlayerTeam;
        ui.list = {2, 1, 0, arenaContext.playerTeam};
        return;
    }
}

void draw()
{
    draw(modeState.arena.ui, arenaContext);
}

} // namespace arena

#endif
