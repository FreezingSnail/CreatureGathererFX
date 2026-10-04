#pragma once

#include "src/engine/battle/BattlePresenter.hpp"
#include "generated/creature_data.hpp"

namespace battle_presentation_fixture {
// Read generated authored fields rather than relying on numeric table indices.
inline void fill(battle::ActionResult &result, bool knockout) {
    battle::resetActionResult(result);
    const uint8_t playerSpecies = pgm_read_byte(&creatureFixtures->id);
    const CreatureData_t *opponent = creatureFixtures + creatureFixtureCount - 1;
    const uint8_t opponentSpecies = pgm_read_byte(&opponent->id);
    result.kind = battle::ResultKind::Attack;
    result.index = pgm_read_byte(&creatureFixtures->move1);
    result.speciesBefore[0] = playerSpecies;
    result.speciesBefore[1] = opponentSpecies;
    result.maxHpBefore[0] = result.maxHpBefore[1] = 100;
    result.hpBefore[0] = result.hpAfter[0] = 80;
    result.hpBefore[1] = 60;
    result.hpAfter[1] = knockout ? 0 : 35;
    result.flags = knockout ? battle::OPPONENT_FAINTED : 0;
}

inline battle::BattleView afterView(const battle::ActionResult &result) {
    battle::BattleView view{};
    for (uint8_t side = 0; side < 2; ++side)
        view.active[side] = {result.speciesBefore[side], result.hpAfter[side],
                             result.maxHpBefore[side]};
    view.gatherProgress = result.progressAfter;
    return view;
}

inline void drawView(const battle::BattleView &view) {
    for (uint8_t side = 0; side < 2; ++side) {
        const battle::ActiveView &active = view.active[side];
        const uint8_t x = side == static_cast<uint8_t>(battle::Side::Player) ? 96 : 0;
        if (active.id < 32) {
            const uint8_t frame = static_cast<uint8_t>(active.id * 2 + (side == 0 ? 1 : 0));
            Blit::draw(x, 0, 32, 32, NewecreatureSprites, FRAME(frame), Blit::PLUSMASK);
        }
        const uint8_t barX = side == 0 ? 90 : 8;
        const uint8_t bgX = side == 0 ? 88 : 6;
        const uint8_t hp = active.maxHp == 0 ? 0
            : (active.hp > active.maxHp ? active.maxHp : active.hp);
        const uint8_t width = active.maxHp == 0 ? 0
            : static_cast<uint8_t>(static_cast<uint16_t>(hp) * 30 / active.maxHp);
        Blit::fillRect(bgX, 34, 34, 6, BLACK);
        Blit::fillRect(barX, 36, width, 2, WHITE);
    }
}

// Optional shipping-build spike exercises the actual FX renderer under LTO.
// It adds no globals. It is deliberately fixture-driven, not combat/session proof.
__attribute__((noinline)) inline void shippingSpike() {
    battle::ActionResult result{};
    battle::BattlePresenter presenter;
    for (uint8_t knockout = 0; knockout < 2; ++knockout) {
        fill(result, knockout != 0);
        const battle::BattleView baseView = afterView(result);
        presenter.begin(result);
        while (!presenter.done()) {
            battle::BattleView view = baseView;
            presenter.overlay(view);
            arduboy.clear();
            drawView(view);
            presenter.draw();
            presenter.update(false);
        }
    }
}

// Isolate the internal PSTR/fontTrimmed caption call chain from optional
// result-kind test locals while retaining the same result/presenter/view shape.
__attribute__((noinline)) inline void captionSpike() {
    battle::ActionResult result{};
    battle::resetActionResult(result);
    result.kind = battle::ResultKind::Skip;
    result.speciesBefore[0] = pgm_read_byte(&creatureFixtures->id);
    result.maxHpBefore[0] = result.hpBefore[0] = result.hpAfter[0] = 100;
    battle::BattlePresenter presenter;
    presenter.begin(result);
    battle::BattleView view = afterView(result);
    presenter.overlay(view);
    arduboy.clear();
    drawView(view);
    presenter.draw();
    presenter.draw();
}
} // namespace battle_presentation_fixture
