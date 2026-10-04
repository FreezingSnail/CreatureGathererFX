# One-action battle contract

Frozen by `CreatureGathererFX-jp8.3.13`, 2026-10-03. This document replaces
the historical EventSink/resolveTurn/popEvent design. It defines implementation
contracts, not completed mechanics or measured savings.

## Shared shapes

All new enums use `: uint8_t`. Pure headers include no Arduino, FX, dialogs,
menus, timers, or legacy Battle.hpp. `src/engine/battle/BattleTypes.hpp` owns
`battle::Side { Player=0, Opponent=1 }`; array indices use these values.

```cpp
namespace battle {
struct ActiveView { uint8_t id, hp, maxHp; };                    // 3
struct PartySummary { uint8_t id, hp, alive; };                  // 3
struct BattleView {                                            // 34
    ActiveView active[2]; PartySummary party[2][3];
    uint8_t partyCount[2], activeSlot[2], moveIds[4];
    uint8_t gatherProgress, gatherNeed;
};
struct MoveSnapshot { uint8_t moveIds[4]; };                     // 4
struct PartyChoice { uint8_t id, slot, hp; };                    // 3
struct PartySnapshot { PartyChoice choices[2]; uint8_t count, forced; }; // 8
enum class ActionKind : uint8_t { Attack, Switch, Gather, Escape, Skip };
struct BattleAction { ActionKind kind; uint8_t index; };         // 2
struct TurnPlan { BattleAction action[2]; };                    // 4
struct Combatant {                                             // AVR 35
    uint8_t id; DualType types; uint8_t level, hp, maxHp;
    stats_t stats; Move moves[4]; uint8_t moveIds[4];
    StatusEffect status; StatModifer statMods;
};
struct BenchSlot { uint8_t id, level, hp; };                     // 3
struct GatherState { uint8_t progress, need, fleeTurns, tierRate; }; // 4
struct BattleState {                                           // AVR 94
    Combatant active[2]; BenchSlot bench[2][2];
    uint8_t partyCount[2], activeSlot[2]; GatherState gather;
    bool gatherable, trainer, over; uint8_t trainerId;
};
}
enum class MenuIntentKind : uint8_t { None, SelectMove, SelectParty, Gather, Escape, Back };
struct MenuIntent { MenuIntentKind kind; uint8_t index; };        // 2
```

`BattleView.hpp` owns views/snapshots; `MenuIntent.hpp` owns menu intents.
`BattleViewAdapter.hpp/.cpp` exposes only
`battle::BattleView legacyBattleView(const BattleEngine&)` during .1; .11 removes
it. .1 ports the draw helpers, and .11 finishes session-based view ports.
Namespace compatibility: the existing global `battle()` accessor in ModeState.hpp
conflicts with namespace `battle`. During .1, mechanically rename that accessor
and its callers to `legacyBattle()`; keep the union member named `battle`.
No alias named `battle` may remain at global scope. The session port .11 introduces
`battleSession()` for the new session accessor, and .12 removes all remaining
`legacyBattle()` uses/declaration with legacy engine cleanup. Root owns the current
mechanical compatibility edit; the .1 view worker owns its view/adapter edits.
Empty party slots are id=0/hp=0/alive=0 under the existing party terminator
contract. Species ID 0 remains a valid raw asset/table entry and fixture.
Move ID 0 is a valid move; move ID 255 means absent. Party snapshots carry
original party slots, never `cursor/2`. SelectParty.index is that original slot.
Bench order is ascending original slot excluding activeSlot; rebuilding that
order on a switch preserves HP/writeback identity without adding slot bytes.

## One resident result

`ActionResult.hpp` owns these exact byte-only shapes:

```cpp
namespace battle {
enum class ResultKind : uint8_t { None, Attack, Switch, Gather, Escape, Skip, EndTurn };
enum class Outcome : uint8_t { None, Win, Lose, Escaped, Gathered, Fled };
struct Consequence { Effect effect; uint8_t side, value; };      // 3
struct ActionResult {                                          // 28, offsets below
    ResultKind kind; Side actor; uint8_t index, flags;
    Outcome outcome; Modifier effectiveness;                    // offsets 0..5
    uint8_t speciesBefore[2], maxHpBefore[2];                    // offsets 6..9
    uint8_t hpBefore[2], hpAfter[2];                             // offsets 10..13
    uint8_t progressBefore, progressAfter;                      // offsets 14..15
    Consequence consequences[4];                               // offsets 16..27
};
}
```

Flags: bit0 SelfHit, bit1 Refused, bit2 StatusSkipped, bit3 PlayerFainted,
bit4 OpponentFainted, bit5 ForcedSwitch; bits6..7 reserved zero. `index` is the
semantic move ID for Attack, incoming species ID for Switch, 255 otherwise.
`effectiveness` is Same unless Attack; damage includes immunity (None) and may
be zero. Actual main damage is `hpBefore[target]-hpAfter[target]`; no intended
damage amount is narrated. For EndTurn, hpAfter is final after all four facts.

Unused consequences are `{Effect::NONE,255,0}`. Facts are **successful semantic
changes**, not a queue of events/pages: two move effects maximum, or four tick
facts maximum (Player slot0, slot1, then Opponent slot0, slot1). Stat `value` is
the final clamped stage encoded as stage+3 (0..6); status `value` is zero;
SAPPD/INFSED `value` is HP immediately after that tick. Tick playback computes
each actual delta from the preceding displayed HP, preserving mixed damage/heal
that cancel to a zero net delta. Failed rolls/full status slots/capped unchanged
stats emit no fact. Both packed effect bytes are supported, though the current
native generator writes authored effect1 and NONE effect2. No general event
array, replay log, dialog FIFO, heap allocation, or floating point is permitted.

## Pure boundaries and mechanics

```cpp
Side firstMover(const BattleState&, const TurnPlan&);
void resolveAction(BattleState&, Side, BattleAction, Rng&, ActionResult& out);
void resolveEndTurn(BattleState&, Rng&, bool acquisitionPending, ActionResult& out);
bool sideDefeated(const BattleState&, Side);
bool canSwitch(const BattleState&, Side, uint8_t originalSlot);
// FX transition boundary, outside Resolve.cpp:
bool applySwitch(BattleState&, Side, uint8_t originalSlot, bool forced, ActionResult& out);
TurnGate gateTurn(BattleState&, Side, Rng&);
bool applyEffect(BattleState&, Side, Effect, Consequence& out);
bool rollMoveEffect(BattleState&, Side attacker, Effect, Rng&, Consequence& out);
void tickEffects(BattleState&, ActionResult& out);
```

`ActionResult` and `Consequence` are raw POD aggregates with **no default member
initializers or constructors**. Brace-zero initialization alone does not establish
the semantic sentinels. `ActionResult.hpp` provides
`inline void resetActionResult(ActionResult& out)`: explicitly sets kind=None,
actor=Player, index=255, flags=0, outcome=None, effectiveness=Same; zeroes every
before/after species/maxHP/HP/progress byte; fills all four consequence slots by
a loop with `{Effect::NONE,255,0}` through explicit member stores. Do not copy a
constant default result/consequence template into SRAM. Call reset before every
new result is populated, including fixture builders and resolver/end-turn/switch
paths. Repeated frames never reset an in-flight result. .2 sentinel tests call
reset explicitly. Layouts remain exactly28/3 bytes.

Resolver resets caller-owned output, then fills before facts and unused sentinels,
then mutates once. Switch dispatch belongs to session/setup; Resolve has no setup
include. `applySwitch` validates before reads, preserves outgoing/incoming HP,
resets incoming statuses/stages, and fills a separate Switch result. `Rng` keeps
the existing function-pointer seam (AVR2 bytes), scripted state stays in tests.
Define named `enum : uint8_t` flag masks `SELF_HIT=1<<0`, `REFUSED=1<<1`,
`STATUS_SKIPPED=1<<2`, `PLAYER_FAINTED=1<<3`, `OPPONENT_FAINTED=1<<4`,
`FORCED_SWITCH=1<<5` in ActionResult.hpp. Cursor flags uses
`ACQUISITION_PENDING=1<<0`, other bits reserved zero. Flags and timing constants
are enum constants to prevent AVR SRAM materialization; names/values stay pinned.
Existing damage formula/stage/immunity/saturation contract is unchanged; accuracy
and critical rolls remain out of scope. Effect rates use the PROGMEM table owned
by .4, initially100 for every recognized effect, matching current getEffectRateFX
(which actually returns100 without reading FX); no per-action FX lookup.

Priority: Switch/Gather/Escape fast, Attack/Skip normal; compare effective staged
speed in a wide integer and Player wins ties. Freeze order once at turn start.
Gate Attack/Gather/Escape once per actor, scanning status slot0 then slot1 and
returning on the first triggered gate: PINNED 1/3 skips; CONCUSED 1/4 replaces the
action with self-hit using the selected attack move, or move slot0 for Gather/
Escape. Switch and explicit Skip do not roll status gates. Self-hit applies damage
only, no move effects. Ordinary Attack processes both move effect slots after
damage, suppressing changes on fainted targets (including self-target effects if
the attacker is down). Effects use explicit sides.

Faint flags mark only live-to-zero transitions. `sideDefeated` checks the actual
party count and live bench. Player defeat has priority over opponent defeat when
both sides are defeated. Death is absorbing for ticks: later INFSED cannot revive
a combatant at zero. EndTurn applies all live slot ticks in the fixed order,
then decides Lose, Win, pending Gathered, then flee. Countdown decrements once
only when gatherable, still active, and no earlier terminal result; saturates at
zero. Wild Escape succeeds immediately; trainer Escape is refused and costs the
turn. Non-gatherable Gather is refused and costs the turn. Gather increases
progress saturating at need; reaching need is recorded pending in cursor.flags,
and opponent action and end ticks still execute. This reconciles the previous
contradictory immediate-acquisition/full-turn-damage wording: player loss beats
acquisition, acquisition beats flee. KO loses the gather opportunity. No party,
box, lure-charge, item or material write is added.

## Case table

| Resolution | Result / visible stages | Progression |
|---|---|---|
| Ordinary or immune attack | Attack: announce+animation, impact HP/damage/effectiveness, successful effects | Next pending actor |
| PINNED | Skip+StatusSkipped: status consequence text | Next pending actor |
| CONCUSED | Attack+SelfHit: self-hit announce, self impact, faint if needed | Next pending actor or replacement |
| Explicit Skip / invalid action | Skip; invalid adds Refused, no mutation | Next pending actor |
| Two move effects | Up to two facts in authored order, explicit targets | No reroll during playback |
| SAPPD/INFSED, including net-zero mixed ticks | EndTurn: each fact impact in fixed slot order, then faints/outcome | One end-turn completion |
| Voluntary switch | Separate Switch: old species until switch impact, incoming HP/species thereafter | Frozen opposing action targets incoming combatant |
| Player faint with live bench | Faint consequence; forced PartySnapshot afterward, Back disabled | Cancel that side's unexecuted action; resume remaining live actor against replacement |
| Opponent faint with live bench | Faint consequence; auto-switch separate result to lowest original live slot | Cancel opponent's unexecuted action; resume remaining live actor against replacement |
| Both faint with live benches | Player forced replacement first, then opponent switch | Both pending actions cancelled, proceed EndTurn once |
| Last faint | Faint then Win/Lose attached to current result | No further actions/ticks |
| Gather progress | Gather: old progress then updated progress at impact | Opponent acts; ticks/countdown once |
| Gather acquired | EndTurn outcome Gathered after surviving opponent/ticks | Terminal presentation then exit |
| Gather refused | Gather+Refused consequence | Opponent acts |
| Wild escape | Escape outcome Escaped | Cancel other action/ticks |
| Trainer escape | Escape+Refused | Opponent acts |
| Countdown expiry | EndTurn outcome Fled | Terminal presentation then exit |

An invalidated actor's frozen action never transfers to its replacement. After
replacement, an already planned *other* actor uses the replacement as its current
target; priority/AI are not recomputed. Replacements after EndTurn complete before
opening a fresh turn. A player with no live party on entry gets terminal Lose
feedback before exit; it cannot attack.

## Session and presenter lifecycle

```cpp
enum class SessionPhase : uint8_t { Choice, Ready, Presenting, Replacement, Terminal, Inactive };
struct TurnCursor { TurnPlan plan; Side first; uint8_t next, invalidMask, flags; SessionPhase phase; }; //9
// cursor.next: 0 first, 1 second, 2 EndTurn, 3 complete
class BattleSession {
public:
    void beginWild(uint8_t id,uint8_t level,bool gatherable,uint8_t tierRate);
    void beginTrainer(uint8_t id);
    bool submitIntent(MenuIntent);
    bool advance();                  // produce at most one result; never during playback
    const ActionResult& result() const;
    void finishPresentation();       // acknowledges current result once
    bool isActive() const;           // remains true through terminal presentation
    bool awaitingPlayer() const;     // only Choice or Replacement
    bool awaitingReplacement() const;
    bool exitReady() const;          // terminal feedback acknowledged
    void syncPlayerHp();             // save/terminal transfer, original party slots
    BattleView view() const;         // transient; not retained as full clone
    MoveSnapshot moves() const;
    PartySnapshot partyChoices() const;
};
enum class PresenterStage : uint8_t { Idle, Announce, Impact, Consequence, Faint, Terminal, Done };
class BattlePresenter {
public:
    void begin(const ActionResult&); // borrowed until done; prepare first item
    void update(bool freshAEdge);    // timer/stage/animation only
    void draw() const;               // cached assets; no mutation or table/header lookup
    bool done() const;
    PresenterStage stage() const;
    void overlay(BattleView&) const; // before/impact HP/species/max facts on transient view
};
```

Session owns state94+result28+cursor9+Rng2 =133 AVR bytes. Presenter ceiling24
AVR bytes: result pointer2, stage/elapsed/fact cursor/flags4, display HP2, prepared
item16. Prepared item is three uint24 addresses (name/detail/animation), six byte
dimensions (three width/height pairs), one frame count. No persistent BattleView.
The combined payload ceiling is157 AVR bytes (rounding/alignment must be proven,
not assumed); fits the existing191-byte WorldTransient union capacity. If actual
implementation needs extra bytes, trim before expanding and document a contract
amendment; never enlarge ModeState silently. Host alignment may differ.
`trainerId` is set on trainer entry and cleared to 255 on wild entry. Trainer
replacement rereads the original trainer row by that ID and selects the original
party slot, preserving authored moves without a resident row copy.

Timing at the actual shipping52 FPS: named constants ANNOUNCE_TICKS=42,
IMPACT_TICKS=21, CONSEQUENCE_TICKS=31, FAINT_TICKS=31, TERMINAL_TICKS=52,
MIN_DWELL_TICKS=10. A fresh edge reduces remaining dwell to10
(never below total10); the edge is consumed once and never skips several stages.
Held A has no effect; only a newly pressed edge accelerates. Animation8 frames
advances from announce elapsed time, with the last frame displayed by completion;
missing animation draws nothing. Skipped/refused actions use Consequence31 and
complete naturally. For Attack main HP changes at Impact entry; EndTurn updates
one tick fact on each Impact entry. Applied effects follow impact, then faint
Player before Opponent, then terminal. Switch changes species/max/HP at Impact.
Faint sprite remains until Faint completion, regardless of logically zero HP.
After that side's Faint completes, overlay sets its ActiveView.id to255 (absent);
draw helpers validate id<32 before sprite access. Zero is a valid raw species
fixture and must not be used to hide a sprite. Presenter flags holds two hidden
side bits without extra storage. A fresh result whose hpBefore is already zero
starts that side hidden (for example forced-switch announcement), so a defeated
sprite cannot reappear before incoming switch impact. A new live-to-zero result
starts visible and hides only after its Faint completes.
Gather progress changes at Impact. Drawing repeatedly never advances anything.
Timing names above are `enum : uint8_t` constants, with no resident timing table.

The .14 exercised resource spike exposed30 B of unwanted `.data`: masks6,
timings6, compiler duration table5, generated fixture-count1, default consequence
template12. Root approved enum constants and explicit POD reset as a resource
refinement; the generated fixture-count byte is independently attributable and
not a claimed result/presenter saving. .2 remains closed with sizes preserved;
.14's full gate reruns its layout/sentinel tests. These observations motivate the
contract refinement and do not by themselves establish the final shipping delta.

Each frame has exactly one input owner. Choice input cannot accelerate its newly
created result. Playback completion consumes its A edge before a fresh choice
opens; no fall-through into MenuV2. Presenter done triggers finishPresentation;
next resolution occurs on a subsequent update. Render does not resolve, update,
reroll, switch, or acknowledge. Terminal logical state blocks new combat immediately;
union stays battle until terminal done, HP sync, menu/dialog cleanup, and exitBattle
last. Every exit caller returns immediately after destruction.

## Assets, HP, measurement, and ownership

Current device renderer is Arduboy2Base1bpp, one render per frame. Text string
addresses are raw pixels: explicit width x8 at address-2. Creature frame is
opponent id*2 / player id*2+1, 32x32 at NewecreatureSprites-2. Name table species
0..31, moves0..32; move32 is empty width0; sentinel255 must never index tables.
Animations basicBeamR/L and BasicWaveR/L are raw headerless32x32,8 frames, each
6144 packed bytes (8 frames x3 legacy shades x128 pixels bytes x2 mask). Their
symbols point to raw pixels: render at address-2 with explicit32x32 dimensions,
fixed frame count8, and no spriteHeader lookup. Packed bytes/stride prove the
legacy Animator header assumption incorrect; preserve the packed ABI. All24-bit
table lookups use FxRead::indexed24. Raster SPI streaming is allowed; metadata
lookups are transition-only and checked by the read counter. No assumption that
the old draw-mutating Animator is safe for presenter draw.

Preserve closed jp8.7 decision A: Player::creatureHPs is persistent owner; copy in
at begin, copy all original slots out at every terminal exit; save during BATTLE
syncs HP first and resumes the still-live state. No entry refill. jp8.1.10 adds
partyHP and version1->2; jp8.2.6 store-index migration keeps partyHP and version2->3;
previous-schema saves are intentionally invalidated. .11 owns transfers; .16 owns
their terminal lifecycle wiring; .1.10 owns schema/startup restore.

Measured parent baseline: shipping18492 flash/1871 static/689 free; stack suite229
painted,160 effective after69 ISR, transition370 (device test globals2050). The
191-byte union means removing a174-byte legacy engine alone is not174-byte shipping
savings. Preserve at least approximately150 effective stack bytes. No assertion
that a standalone <=100-byte state is a complete session/presenter resource budget.

Ownership: .1 shared views/types/intents/legacy adapter, draw-view ports and
mechanical global-accessor compatibility rename; .2
BattleState/ActionResult shapes/tests; .3 damage; .4 effects/RNG; .5 FX setup;
.6 one-action resolver/order; .7 faint classification/switch validation+setup seam;
.8 escape; .9 gather/end-turn terminal ordering; .10 AI; .11 session/cursor/general
API, snapshot/view ports and HP transfer; .14 presenter spike; .15 full presenter;
.16 playback/input/forced-choice/delayed-exit wiring; .12 deletion only after .16.
Menu .4.2 owns pure MenuV2 intent production, no BattleEngine interpreter and no
sketch dispatch edit; .16 owns the shared sketch routing. Same-file tasks serialize.

Implementation verification: make test; make testvm when integration changes;
make build; make ram; make fxtest-spike
FXTEST_SPIKE_INO=tst/fxdatatest/test_battlepresentation.ino
ARDENS=/absolute/path/to/headless/Ardens (session uses test_battlesession.ino).
First device spike always includes test_stack. After edge review, orchestrator
runs make final-gate ARDENS=/absolute/path/to/headless/Ardens once per bead;
make test-pack-parity if packed bytes change. Record exact commands, counts,
flash/static/painted+effective stack, failed attempts and wall times in output.md.
