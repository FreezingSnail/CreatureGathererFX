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
#include "../../lib/ReadData.hpp"
#include "../../../fxdata/generated/arena_demo_ids.hpp"
#include "ArenaCatalog.hpp"
#include "ArenaNavigation.hpp"
#include "ArenaView.hpp"
#include "../../lib/random.hpp"

namespace arena {

ArenaContext arenaContext = {};

namespace {

// The demo boots straight into a random 3v3. Both draws come from the shared
// game RNG (seeded once in setup from the hardware entropy source), so
// successive boots and successive rounds vary. Tests inject the sequence with
// rngSeed()/randomRoll().
void rollRandomPair()
{
    arenaContext.playerTeam = randomRoll(
        0, static_cast<uint8_t>(ArenaDemoIds::playerCount - 1));
    arenaContext.opponentTeam = randomRoll(
        0, static_cast<uint8_t>(ArenaDemoIds::opponentCount - 1));
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
        creature.attackMask = member.attackMask;
        creature.status.clearEffects();
        creature.statMods.clearModifiers();
        target.creatureHPs[slot] = creature.hp;

        // The member reader owns its three-byte FX read. Approve the canonical
        // creature record at this per-member boundary.
        FxReadCounter::transitionExact(reads);
    }
    return true;
}

void boot()
{
    arenaContext = {0, 0};
    enterArena();
    modeState.arena.ui = {};
    // Roll a random team and opponent, then start immediately: the demo has no
    // interactive picker on its normal path. The picker screens remain only as
    // the startMatch() fallback for an empty/invalid authored catalog.
    rollRandomPair();
    startMatch();
}

bool startMatch()
{
    if (arenaContext.playerTeam >= ArenaDemoIds::playerCount ||
        arenaContext.opponentTeam >= ArenaDemoIds::opponentCount) return false;

    uint8_t trainerId;
    if (!readOpponentId(arenaContext.opponentTeam, trainerId)) return false;
    if (!applyPlayerTeam(arenaContext.playerTeam, player)) return false;

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

void finishBattle(battle::Outcome)
{
    // Continuous random 3v3: every round re-rolls both sides and restarts.
    rollRandomPair();
    startMatch();
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
    draw(modeState.arena.ui);
}

} // namespace arena

#endif
