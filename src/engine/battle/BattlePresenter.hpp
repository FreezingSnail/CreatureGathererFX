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

// Generated string rasters retain one leading blank glyph slot. Keep their
// pixel widths tied to authored character counts so blits stop at the asset.
constexpr uint8_t generatedTextWidth(uint8_t characters) {
    return static_cast<uint8_t>((characters + 1u) * 5u);
}

constexpr uint8_t attackTextWidth = generatedTextWidth(16);
constexpr uint8_t enemyAttackTextWidth = generatedTextWidth(12);
constexpr uint8_t damageTextWidth = generatedTextWidth(12);
constexpr uint8_t switchInTextWidth = generatedTextWidth(16);
constexpr uint8_t faintedTextWidth = generatedTextWidth(7);
constexpr uint8_t winTextWidth = generatedTextWidth(7);
constexpr uint8_t loseTextWidth = generatedTextWidth(10);
constexpr uint8_t escapedTextWidth = generatedTextWidth(11);
constexpr uint8_t gatherTextWidth = generatedTextWidth(6);

// Generated effectiveness labels use the 5x6 font with one leading blank
// glyph slot retained by the string packer.
constexpr uint8_t effectivenessTextWidth(Modifier modifier) {
    return modifier == Modifier::Quarter ? generatedTextWidth(17) :
           modifier == Modifier::Half ? generatedTextWidth(12) :
           modifier == Modifier::Double ? generatedTextWidth(13) :
           modifier == Modifier::Quadruple ? generatedTextWidth(17) : 0;
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
