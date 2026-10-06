#include "ArenaDemo.hpp"

#if defined(CGFX_ARENA_DEMO) || defined(TEST) || defined(FX_READ_COUNTER)

#include "../ModeState.hpp"
#include "../game/Gamestate.hpp"
#include "../menu/MenuV2.hpp"
#include "../../globals.hpp"
#include "../../player/Player.hpp"
#include "../menu/MenuNav.hpp"
#include "../battle/BattleSetup.hpp"
#include "../../lib/FxReadCounter.hpp"
#include "../../lib/MoveIds.hpp"
#include "../../lib/ReadData.hpp"
#include "../../../fxdata/generated/arena_demo_ids.hpp"
#include "ArenaCatalog.hpp"
#include "ArenaNavigation.hpp"
#include "ArenaView.hpp"

namespace arena {

ArenaContext arenaContext = {};

namespace {

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

bool applyPlayerTeam(uint8_t team, Player &target)
{
    if (team >= ArenaDemoIds::playerCount) return false;

    for (uint8_t slot = 0; slot < 3; ++slot) {
        Member member{};
        if (!readPlayerMember(team, slot, member)) return false;

        const CreatureData_t seed = getCreatureFromStore(member.species);
        uint8_t reads = 0;
#ifdef __AVR__
        ++reads; // The canonical creature record is one FX lookup.
#endif

        Creature &creature = target.party[slot];
        creature.id = seed.id;
        creature.level = member.level;
        creature.loadTypes(seed);
        creature.setStats(seed);
        for (uint8_t moveSlot = 0; moveSlot < 4; ++moveSlot) {
            const uint8_t moveId = member.moveIds[moveSlot];
            if (moveId == LEGACY_EMPTY_MOVE_ID) {
                creature.moves[moveSlot] = LEGACY_EMPTY_MOVE_ID;
                creature.moveList[moveSlot] = Move();
            } else {
                creature.setMove(moveId, moveSlot);
                ++reads;
            }
        }
        creature.status.clearEffects();
        creature.statMods.clearModifiers();
        target.creatureHPs[slot] = creature.statlist.hp;

        // The member reader owns its six-byte FX read. Approve the canonical
        // creature and authored move records at this per-member boundary.
        FxReadCounter::transitionExact(reads);
    }
    return true;
}

void boot()
{
    arenaContext = {0, 0, battle::Outcome::None};
    enterArena();
    ArenaUiState &ui = modeState.arena.ui;
    ui = {};
    ui.screen = ArenaScreen::PlayerTeam;
    ui.list = {ArenaDemoIds::playerCount, 1, 0, 0};
    loadPreview(ArenaScreen::PlayerTeam, 0, ui.preview);
    gameState.state = GameState_t::ARENA;
}

bool startMatch()
{
    if (arenaContext.playerTeam >= ArenaDemoIds::playerCount ||
        arenaContext.opponentTeam >= ArenaDemoIds::opponentCount) return false;

    uint8_t trainerId;
    if (!readOpponentId(arenaContext.opponentTeam, trainerId)) return false;
    if (!applyPlayerTeam(arenaContext.playerTeam, player)) return false;

    menu.clear();
    dialogMenu.clear();
    enterBattle();
    battleSession().beginTrainer(trainerId);
    if (!battleSession().isActive() || battleSession().state().over) {
        enterArena();
        ArenaUiState &ui = modeState.arena.ui;
        ui.screen = ArenaScreen::PlayerTeam;
        ui.list = {ArenaDemoIds::playerCount, 1, 0, arenaContext.playerTeam};
        loadPreview(ArenaScreen::PlayerTeam, arenaContext.playerTeam, ui.preview);
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
    const ArenaIntent intent = navigate(ui, arenaContext, edgeButtons,
                                        ArenaDemoIds::playerCount,
                                        ArenaDemoIds::opponentCount);
    if (intent == ArenaIntent::PreviewChanged) {
        ArenaPreview preview = {};
        if (loadPreview(ui.screen, ui.list.cursor, preview))
            ui.preview = preview;
        return;
    }
    if (intent == ArenaIntent::StartBattle) {
        startMatch();
        return;
    }
}

void draw()
{
    draw(modeState.arena.ui, arenaContext);
}

} // namespace arena

#endif
