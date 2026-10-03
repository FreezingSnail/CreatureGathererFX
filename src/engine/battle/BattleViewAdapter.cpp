#include "BattleViewAdapter.hpp"

#include "Battle.hpp"
#include "../../values.hpp"

extern Player player;

namespace battle {

namespace {

uint8_t sideIndex(Side side) {
    return static_cast<uint8_t>(side);
}

uint8_t boundedHp(uint16_t hp, uint8_t maxHp) {
    return hp > maxHp ? maxHp : static_cast<uint8_t>(hp);
}

void copyActive(BattleView &view, Side side, const Creature *creature,
                uint16_t hp, uint8_t slot) {
    if (creature == nullptr) {
        return;
    }

    const uint8_t index = sideIndex(side);
    const uint8_t maxHp = creature->statlist.hp;
    view.active[index] = {creature->id, boundedHp(hp, maxHp), maxHp};
    view.activeSlot[index] = slot;
}

void copyParty(BattleView &view, Side side, Creature *const *party,
               const uint8_t *healths) {
    const uint8_t index = sideIndex(side);
    for (uint8_t slot = 0; slot < PARTY_SIZE; ++slot) {
        const Creature *creature = party[slot];
        if (creature == nullptr || creature->id == 0) {
            break;
        }

        const uint8_t hp = boundedHp(healths[slot], creature->statlist.hp);
        view.party[index][slot] = {creature->id, hp, static_cast<uint8_t>(hp != 0)};
        view.partyCount[index] = static_cast<uint8_t>(slot + 1);
    }
}

void copyOpponentParty(BattleView &view, const BattleEngine &engine) {
    const uint8_t index = sideIndex(Side::Opponent);
    for (uint8_t slot = 0; slot < PARTY_SIZE; ++slot) {
        const Creature &creature = engine.opponent.party[slot];
        if (creature.id == 0) {
            break;
        }

        const uint8_t hp = boundedHp(engine.opponentHealths[slot], creature.statlist.hp);
        view.party[index][slot] = {creature.id, hp, static_cast<uint8_t>(hp != 0)};
        view.partyCount[index] = static_cast<uint8_t>(slot + 1);
    }
}

} // namespace

BattleView legacyBattleView(const BattleEngine &engine) {
    BattleView view = {};
    view.active[sideIndex(Side::Player)].id = 255;
    view.active[sideIndex(Side::Opponent)].id = 255;

    for (uint8_t slot = 0; slot < 4; ++slot) {
        view.moveIds[slot] = 255;
    }

    if (engine.playerIndex < PARTY_SIZE) {
        copyActive(view, Side::Player, engine.playerCur,
                   player.creatureHPs[engine.playerIndex], engine.playerIndex);
    }
    if (engine.opponentIndex < PARTY_SIZE) {
        copyActive(view, Side::Opponent, engine.opponentCur,
                   engine.opponentHealths[engine.opponentIndex], engine.opponentIndex);
    }

    copyParty(view, Side::Player, engine.playerParty, player.creatureHPs);
    copyOpponentParty(view, engine);

    if (engine.playerCur != nullptr) {
        for (uint8_t slot = 0; slot < 4; ++slot) {
            view.moveIds[slot] = engine.playerCur->moves[slot];
        }
    }

    // The legacy engine has no gather state; jp8.3.9 adds it to BattleSession.
    view.gatherProgress = 0;
    view.gatherNeed = 0;
    return view;
}

} // namespace battle
