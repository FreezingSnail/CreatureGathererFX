#pragma once

#include "ActionResult.hpp"
#include "BattleView.hpp"
#include "../../lib/uint24.h"

namespace battle {

// Enum constants cannot acquire SRAM storage when the AVR compiler retains
// unrelated read-only data from a sketch translation unit.
enum : uint8_t {
    ANNOUNCE_TICKS = 42, IMPACT_TICKS = 21, CONSEQUENCE_TICKS = 31,
    FAINT_TICKS = 31, TERMINAL_TICKS = 52, MIN_DWELL_TICKS = 10
};

enum class PresenterStage : uint8_t {
    Idle, Announce, Impact, Consequence, Faint, Terminal, Done
};

enum class AttackTreatment : uint8_t { Projectile, PhysicalWave };

constexpr AttackTreatment attackTreatment(bool physical) {
    return physical ? AttackTreatment::PhysicalWave : AttackTreatment::Projectile;
}

// Generated effectiveness labels use the 5x6 font with one leading blank
// glyph slot retained by the string packer.
constexpr uint8_t effectivenessTextWidth(Modifier modifier) {
    return modifier == Modifier::Quarter ? 90 :
           modifier == Modifier::Half ? 65 :
           modifier == Modifier::Double ? 70 :
           modifier == Modifier::Quadruple ? 90 : 0;
}

// Only the current display item is prepared. FX symbols point at raw pixels;
// dimensions are separate and the renderer compensates its +2-byte prefix.
struct PreparedBattleItem {
    uint24_t name, detail, animation;
    uint8_t nameWidth, nameHeight, detailWidth, detailHeight;
    uint8_t animationWidth, animationHeight, frames;
};

class BattlePresenter {
public:
    void begin(const ActionResult &result);
    void reset();
    void update(bool freshAEdge);
    void draw() const;
    bool done() const { return stage_ == PresenterStage::Done; }
    PresenterStage stage() const { return stage_; }
    void sceneOffset(int8_t &x, int8_t &y) const;
    void overlay(BattleView &view) const;

private:
    const ActionResult *result_ = nullptr;
    PresenterStage stage_ = PresenterStage::Idle;
    uint8_t elapsed_ = 0, fact_ = 0, flags_ = 0;
    uint8_t displayHp_[2] = {};
    PreparedBattleItem item_ = {};

    void prepare();
    void next();
    bool nextFact();
    bool nextFaint();
    void nextFaintOrTerminal();
};

#ifdef __AVR__
static_assert(sizeof(PreparedBattleItem) == 16, "one prepared battle item");
static_assert(sizeof(BattlePresenter) == 24, "presenter fits frozen AVR budget");
#endif

} // namespace battle
