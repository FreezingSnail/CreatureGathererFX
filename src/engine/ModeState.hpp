#pragma once

#include <stdint.h>
#ifdef __AVR__
// The Arduino AVR toolchain is built without the C++ standard headers. This
// is the standard no-allocation placement-new hook used to begin BattleSession's
// lifetime inside the union.
inline void *operator new(__SIZE_TYPE__, void *storage) noexcept { return storage; }
inline void operator delete(void *, void *) noexcept {}
#else
#include <new>
#endif

#include "battle/BattlePresenter.hpp"
#include "battle/BattleSession.hpp"
#include "world/TilePropertyWindow.hpp"
#if defined(CGFX_ARENA_DEMO) || defined(TEST) || defined(FX_READ_COUNTER)
#include "arena/ArenaTypes.hpp"
#endif

// Script execution pauses tile movement, so the two-byte movement cursor can
// share the script slot. Coordinates stay in GameState::playerLocation; the
// saved origin lets WorldEngine discard an in-progress step after a teleport.
struct WorldMotion {
    uint8_t directionAndFlags;
    uint8_t step;
    uint8_t originLow;
    uint8_t originHigh;
};

struct WorldTransient {
    union {
        uint8_t script[128];
        WorldMotion motion;
    };
    uint8_t propertyWindow[TilePropertyWindow::STORAGE_BYTES];
    uint8_t zoneTableCache[20];

    void activateScript() {
        for (uint8_t i = 0; i < sizeof(script); ++i) script[i] = 0;
    }
    void activateMotion() { motion = WorldMotion{}; }
};

namespace battle {
struct BattleMode : BattleSession {
    BattlePresenter presenter;

    BattleMode() : BattleSession(), presenter() {}
};

#ifdef __AVR__
static_assert(sizeof(BattleMode) <= 191, "battle playback must fit the world overlay");
#endif
} // namespace battle

#if defined(CGFX_ARENA_DEMO) || defined(TEST) || defined(FX_READ_COUNTER)
namespace arena {
struct ArenaMode {
    ArenaUiState ui;
};
}
#ifdef __AVR__
static_assert(sizeof(arena::ArenaMode) == 24, "arena UI overlay must remain 24 bytes");
static_assert(alignof(arena::ArenaMode) == 1, "arena UI overlay must remain byte aligned");
#endif
#endif

static_assert(sizeof(WorldMotion) == 4, "world motion cursor must fit the script-slot overlay");
static_assert(alignof(WorldMotion) == 1, "world motion cursor must stay byte aligned");
static_assert(sizeof(WorldTransient) == 191, "world transient payload must remain 191 bytes");
static_assert(alignof(WorldTransient) == 1, "world transient payload must stay byte aligned");

// Only these two modes are mutually exclusive. SaveController state remains
// separate because saving can suspend either mode.
union ModeState {
    battle::BattleMode battle;
    WorldTransient world;
#if defined(CGFX_ARENA_DEMO) || defined(TEST) || defined(FX_READ_COUNTER)
    arena::ArenaMode arena;
#endif

    ModeState() : world{} {}
    ~ModeState() {}

    void enterBattle();
    void exitBattle();
#if defined(CGFX_ARENA_DEMO) || defined(TEST) || defined(FX_READ_COUNTER)
    void enterArena();
#endif
};

static_assert(__has_trivial_destructor(battle::BattleMode),
              "ModeState transitions rely on BattleMode having no owned resources");
static_assert(sizeof(ModeState) >= sizeof(WorldTransient), "ModeState must hold the world payload");
#ifdef __AVR__
static_assert(sizeof(ModeState) == 191, "AVR mode storage must be the 191-byte world member");
static_assert(alignof(ModeState) == 1, "AVR mode storage must remain byte aligned");
#endif

extern ModeState modeState;

// Call these only when the matching union member is active. A save state does
// not change the active member and must never be used to select one.
inline __attribute__((always_inline)) battle::BattleSession &battleSession() {
    return static_cast<battle::BattleSession &>(modeState.battle);
}
inline __attribute__((always_inline)) battle::BattlePresenter &battlePresenter() {
    return modeState.battle.presenter;
}
inline __attribute__((always_inline)) WorldTransient &worldState() { return modeState.world; }
inline __attribute__((always_inline)) void enterBattle() { modeState.enterBattle(); }
inline __attribute__((always_inline)) void exitBattle() { modeState.exitBattle(); }
#if defined(CGFX_ARENA_DEMO) || defined(TEST) || defined(FX_READ_COUNTER)
inline __attribute__((always_inline)) void enterArena() { modeState.enterArena(); }
#endif
