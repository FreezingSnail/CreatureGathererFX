# Simplified party battle spike

Status: planning contract, 2026-10-10. Epic: `CreatureGathererFX-bsd`.
Implementation children are deferred until the user requests handoff/begin.
This is an isolated spike; no production adoption is authorized by this document.

## Purpose and evidence

The current dirty shipping tree measured 29,790 B flash and 1,842 B static SRAM,
94 B beyond the 29,696 B application limit. `docs/battle-flash-audit.md` measured
about 17,914 B of whole-image battle-family removal sensitivity, leaving 11,876 B.
That experiment is a reachability measurement, not an additive function budget.
The previous behavior-preserving carve recovered only 276 B; a full streamed
battle VM grew the historical control by 684 B and materially slowed resolution.
The proposed spike therefore simplifies battle interaction and presentation.

Aim to recover about 6 KB against a freshly reproduced current-tree control.
Use a 12,000 B battle-family target as a planning estimate. With today's control,
that would put the image around 23,876 B and leave around 5,820 B for future work.
Neither this estimate nor individual audit rows constitute proven savings.
Publish actual whole-image headroom and reserve at the end; do not claim all free
bytes are available to the world, since other unfinished systems need space too.

The user requires parties and continuation when a creature is knocked out. They
permit investigating removal of voluntary switches. They explicitly require
both automatic and player-selected knockout replacement behind `#ifdef`/compile
time selection in the same demo source, with separate size comparisons.

## Frozen feature contract

| Preserve in both builds | Proposed simplification in this spike |
| --- | --- |
| Existing party size, trainer parties, wild battles | No voluntary player or opponent switching |
| Replacement while a living party member remains | Compare automatic and chosen player replacement |
| Current native damage, types/STAB, physical/special rules | No full battle VM rewrite |
| Status gates, confusion, end-turn damage and utility/stages | One feedback phase instead of staged playback |
| Move PP and original party-slot ownership | No projectile, shake, HP interpolation or faint blink |
| Tactical usable-move scoring and utility decisions | Remove AI voluntary-switch/risk policy only |
| Gathering, escape, all terminal outcomes | Two direct menu pages instead of generic menu stack |
| Stable creature sprites, exact current/max HP | Remove move type/power/physical detail panel |
| Party HP synchronization and terminal callback | No battle item menu or consumable execution route |

The last row removes a dormant battle route, not world inventory or item data.
Do not remove utility moves, stages, status types, PP, gathering, party members,
trainer variety, or damage features to make a target appear met. If this shell
cannot recover enough, report the remaining cost and request a separate scope
decision. New AI tiers remain outside this spike.

## Source isolation and reproducible measurement

1. Record HEAD, dirty diff and untracked production inputs, tool versions,
   generation manifest, selected FQBN and exact shipping flags. A git revision
   alone is insufficient because the control includes unfinished M1/save work.
2. Freeze one control outside `build/`. Exclude `.git`, `.beads`, `.codex`, build
   trees, caches and prior spike directories when copying; do not clone scratch
   trees recursively. Retain a manifest/checksum identifying every source input.
3. Use one candidate checkout with patches/checkpoints; do not make a full source
   clone per experiment. Build outputs belong under its `build/slim-battle/`.
4. Reproduce control sizes with the ordinary shipping flags, including
   `-fno-inline-small-functions`. If the dirty tree has changed, report the new
   control and historical difference before attributing any candidate delta.
5. Control is already over budget: retain its actual ELF/size output even if the
   budget command exits nonzero. Never report that failure as a passing gate.
6. Measure after each coherent shell layer and again after integration. LTO can
   defeat the sum of independent savings; use final paired ELFs as authority.

## Compile-time replacement comparison

Define one isolated spike selector, `CGFX_BATTLE_AUTO_REPLACE`, constrained to 0
or 1 by the build configuration. Both builds are the same game/demo, roster,
encounters and shipping optimization settings. Use `#if CGFX_BATTLE_AUTO_REPLACE`
so inactive replacement code is absent; do not use a runtime boolean or include
both implementations in a measurement image. Do not add another mode selector.

- `1`: player and opponent automatically select the lowest original party slot
  with HP greater than zero, excluding the active slot.
- `0`: opponent follows that same automatic rule. A player knockout pauses turn
  continuation for a direct choice of living bench members. A sole survivor is
  still shown and confirmed, keeping this variant's interaction unambiguous.
- Both modes remain in the same source checkout and compile independently.
  Compare the entire candidate image, static SRAM and painted stack reserve.
- The legacy reference remains a separate frozen control, not extra firmware in
  the candidate. Never compare two different demo harnesses as replacement cost.
- Host tests must exercise both defines explicitly; FX suites must receive the
  same define through `AVR_FXTEST_CPP_FLAGS`. The Makefile currently propagates
  only world profile flags automatically; shipping flags alone do not select the
  host or device test policy.

## Session and party implementation contract

Ownership: `BattleSession.hpp/.cpp`, voluntary-switch branches in `Ai.cpp` and
`Resolve.cpp`, and their native/device tests. Preserve the core APIs consumed by
the simulator unless an explicit adapter is supplied in the integration bead.

`BattleState` is currently 124 B on AVR, `BattleSession` 163 B, `ActionResult`
28 B and `TurnCursor` 9 B. Keep the 28 B result and combat rules for the first
spike. A result-format rewrite is a later measured proposal, not worker latitude.
`ActionKind` numeric values must not be renumbered when routes are removed.

1. Reject voluntary `SelectParty` while awaiting an ordinary action. Neither
   menu can emit it there. `chooseAction` uses the existing non-switch tactical
   action chooser; remove its switch risk/scoring call path. Preserve move
   scoring, utility logic and random-consumption order for non-switch actions.
2. Keep the forced replacement loader. `applySwitch` loads species, stats,
   moves and metadata; deleting it outright would break parties. Preserve the
   original-slot `moveUsesSpent` and `partyModifiers` ownership, HP and levels.
   Clear volatile state according to the existing forced-replacement rules.
3. Keep the frozen turn order and invalidated-actor mask. A creature knocked out
   before its scheduled action loses that action; its replacement must not gain
   an extra attack. Do not re-plan the opponent or recalculate first mover.
4. When both sides need replacement, handle player first, then opponent, before
   resuming the remaining valid frozen actor. Replace only once per emitted
   forced-replacement result, including simultaneous end-turn knockouts.
5. Auto mode enters the existing replacement/result path without waiting for
   `SelectParty`; choose mode accepts only a living bench slot. It cannot cancel
   into ordinary moves, escape or gather while replacement is required.
6. A knockout with surviving bench members is not a loss/win. Party exhaustion
   is terminal. Preserve current resolution precedence for gathering completion,
   fainting and end-turn effects, including pending acquisition suspension.
7. Synchronize player HP back to PARTY by original slot exactly once at exit.
   An invalid or repeated UI intent cannot consume PP or run another action.

Do not replace authored-data trust with runtime validation layers. Continue to
check legitimate input state, missing-move sentinels, zero PP and living choices.
Host validation must reject malformed authored records before packaging.

## Compact interaction contract

Ownership: new `BattleMenu.hpp/.cpp` and its native tests. Do not change generic
world menus, inventory menus or dialog behavior. Leave legacy battle-only menu
sources unreferenced in the candidate; LTO/link evidence must show removal.

Normal action entry opens the move page immediately. It shows four slots in a
2x2 footer: x=4/68, y=48/56. Cursor starts at the lowest usable slot. Directional
edges move through the existing four-slot geometry, skipping empty/zero-PP slots
in the same direction with a bounded four-slot scan. Pin horizontal successor
to `(slot + 1) & 3` for right and `(slot + 3) & 3` for left; vertical successor
is `slot ^ 2` for either up or down. If that direction finds no usable slot,
keep the cursor. If no usable move remains,
show the authored `Pass` caption and A emits `Pass` once. B opens actions.

The actions page has two cells: Gather (left) and Escape (right), y=56. Left or
right selects the other cell; up/down does nothing. B returns to moves without
consuming a turn. Both captions remain visible; an unavailable action has no
selection highlight and A has no effect. Eligibility matches the existing session/resolver contract,
including trainer restrictions. It must not invent new escape/gather rules.

Player-choice replacement uses at most two living bench rows at y=48/56, with
cached names. Up/down toggles when two rows exist; A submits the original party
slot; B does nothing. Auto builds must contain neither these caches nor renderer.
Use original slot numbers, never the visible row index, as the selected intent.

Prepare move snapshots and name addresses only on page/creature transitions.
`MoveSnapshot` is 9 B, `PartySnapshot` 8 B; cache only what this menu consumes.
No generic stack, descriptor dispatch, dialog queue, type/power details or full
`BattleView` construction per menu frame. Store menu state in the battle mode
overlay, not another global object. Cursor edits and drawing use RAM only.

Pin the menu layout to uint8 cursor plus uint8 page/eligibility flags and a
payload union: moves = MoveSnapshot (9) + four uint24 addresses (12) + four
uint8 widths (4), total 25 B; chosen replacement = PartySnapshot (8) + two
uint24 addresses (6) + two widths (2), total 16 B. Menu total is 27 B on AVR.
Use page bits 0..1, allow-gather bit 2 and allow-escape bit 3 in the flags byte.
Suggested API: `beginMoves(const MoveSnapshot&, bool allowGather, bool allowEscape)`,
`beginReplacement(const PartySnapshot&)` (choice build only),
`update(uint8_t freshButtonEdges) -> MenuIntent`, `draw() const`.
Returning an intent does not itself resolve it; session remains authoritative.

Sprites and HP stay visible on both pages. Selected move PP uses a small centre
overlay between the sprite panels, inside x=48..79, y=40..47; use existing compact
PP glyphs and maximum-width checks. Do not truncate a move name silently: host
validation asserts its generated bitmap width fits its 60-pixel footer cell.
Empty moves and authored empty records use existing generated/symbolic predicates,
not hardcoded record numbers. Species/move ID 0 is valid; 255 remains a sentinel.

## Single-phase feedback and scene contract

Ownership: new `BattleFeedback.hpp/.cpp`, scene drawing helpers and their native
tests. `BattleFlow.cpp` and `ModeState.hpp` belong to integration, not parallel
menu/session/feedback workers. Workers expose interfaces; they do not each edit
the shared flow controller.

Replace presenter stages with one cached two-row feedback footer. On entry,
prepare line addresses and widths from the emitted result; render current HP
immediately. Hold for 26 update ticks at 52 fps, or accept a fresh A edge after
four ticks. B has no effect. Terminal feedback uses the same timing. No HP drain,
shake, projectile, consequence queue, faint blink or switch animation.

Pin `BattleFeedback` storage to two 24-bit addresses, two uint8 widths, uint8
elapsed and uint8 phase: 10 B on AVR. No copied ActionResult, heap allocation or
additional result buffer. Address zero/width zero denotes an unused row.
Suggested interface: `begin(const ActionResult&, const BattleState&)`,
`update(bool freshA)`, `done() const`, `draw() const`, `reset()`.

Caption selection is determined at entry in this priority order:

| Result | Row 1 | Row 2 |
| --- | --- | --- |
| Any terminal outcome | Win / Defeat / Escaped / Gathered / Fled | Empty |
| Attack succeeds | Acting species before action | Move name |
| Refused/status-skipped attack | Acting species | Cannot act |
| Confusion self-hit | Acting species | Hurt itself |
| Forced replacement | New active species | Enters the field |
| Gather attempt | Acting species | Gather |
| Failed escape | Acting species | Cannot escape |
| Pass/Skip | Acting species | Pass |
| End turn | Turn end | Empty |

Consequence details are intentionally omitted; all underlying consequences still
apply. Append missing caption assets to the authored text source and regenerate
with `make gen`; never hand-edit addresses. All caption rows must fit 124 pixels
from x=2, y=48/56, as validated by host data checks. Keep existing packed asset
layout/entry ordering; new text must not reorder existing ABI declarations.

Use front enemy/back player creature sprites with the existing 48x48 masked
format and original frame mapping. Preserve current/max HP numbers and low-HP
readability. Scene reads active RAM state directly (or a 6 B active-only view),
without scanning party, move IDs and PP into a full BattleView each frame.
Asset raster reads through the existing FX/Blit draw path are expected. The
zero-idle-read rule refers to metadata reads counted by FxRead, not pixel raster
transfers. Prepare name/table metadata on transitions; use FxRead::indexed24 for
24-bit tables, including high addresses. Do not reread metadata on timer ticks.

## Integration and lifetime

`ModeState` is a 191 B union. Shrinking a member does not itself reduce globals;
measure the full image and static size. Keep session, feedback and compact menu
inside the overlay and assert the battle member fits 191 B on AVR. Session 163 B
plus feedback 10 B leaves 18 B for common/menu state; if selected-name caches
exceed that, reuse storage between mutually exclusive feedback and menu phases
through a disciplined union. Do not grow the overlay silently to fit them.
Use `union { BattleMenu menu; BattleFeedback feedback; }` (27 B), giving a
190 B battle member before any unexpected padding. Avoid a stored phase tag:
session phase/cursor determines the active member. Expose a read-only session
presentation query if needed; do not store a second independent phase machine.
Construct/destroy the active union member on mode transitions; no stale pointers
into inactive menu/result storage. RAM/state lifetime design is settled in the
baseline/interface bead before the parallel shell workers begin.

The controller advances at most one presentation/control boundary per update.
A button edge used to finish feedback cannot select a move or replacement in the
same frame. After feedback: finishPresentation once, then open replacement or
normal choices on a subsequent update, or exit on terminal. Terminal callback,
menu/dialog cleanup and party HP sync run once. No repeated terminal advancement.
No new per-frame FX metadata reads, polling loops or blocking delays.

## Verification and checkpoints

Permanent tests live in `tst/`, `tst/script_tests/` and `tst/fxdatatest/`; no
temporary C++ test harnesses. Use deterministic RNG fixtures. Preserve resolver
tests for damage, type/stage/status math, order, utility, PP and outcomes. Replace
old presentation-duration expectations with tests for the explicit new contract;
do not delete failing mechanics tests as part of a UI cut.

Test both policies for: player/foe first mover; knockout before scheduled action;
forced original-slot PP/modifier restoration; one/two/no surviving bench members;
simultaneous knockout; end-turn poison/burn; gathering completion with knockout;
confusion/self-hit; all PP exhausted; empty move entries; refusal; held versus
fresh A; B cancel on normal pages; B ignored in replacement; last-party loss/win;
terminal callback and sync exactly once; repeated/stale intents and mode reentry.
Auto fixture checks lowest original living slot. Choice fixture selects the other
living slot to prove the row index is not mistaken for party ownership.

Use frozen native resolver fixtures as the mechanics oracle, comparing action
state and RNG consumption where voluntary switches did not occur. Session/menu
changes have explicit new expectations; do not demand old voluntary-switch or
multi-stage UI parity. Reuse generated symbols for edge records and addresses.

Run focused native/device suites and `test_stack` after the first end-to-end
slice, then after integration. Require at least 150 B effective painted reserve,
and retain stricter existing suite thresholds instead of lowering them. Static
SRAM must stay at or below 2,160 B. Flash must fit 29,696 B; mere fit is weaker
than the approximately 6 KB recovery objective. Report misses separately.

Exact command forms (run in the isolated checkout):

```sh
make doctor
make gen
make test CPPFLAGS='-I. -DCGFX_BATTLE_AUTO_REPLACE=0' BUILD_DIR=build/slim-battle/choice
make test CPPFLAGS='-I. -DCGFX_BATTLE_AUTO_REPLACE=1' BUILD_DIR=build/slim-battle/auto
make testvm
make verify-generated
make test-manifest
make test-generated-libs
make test-pack-parity
make ram BUILD_DIR=build/slim-battle/choice AVR_SHIPPING_CPP_FLAGS='-mrelax -mcall-prologues -fno-move-loop-invariants -mstrict-X -DCGFX_SHIPPING_NO_USB -fno-inline-small-functions -DCGFX_BATTLE_AUTO_REPLACE=0'
make ram BUILD_DIR=build/slim-battle/auto AVR_SHIPPING_CPP_FLAGS='-mrelax -mcall-prologues -fno-move-loop-invariants -mstrict-X -DCGFX_SHIPPING_NO_USB -fno-inline-small-functions -DCGFX_BATTLE_AUTO_REPLACE=1'
```

Device tests use `make fxtest-headless FXTEST_INOS='tst/fxdatatest/test_battle_slim.ino tst/fxdatatest/test_stack.ino'`
with `ARDENS=/Users/connorfranc/code/arduboy/Ardens/build/Ardens.app/Contents/MacOS/Ardens`
and `AVR_FXTEST_CPP_FLAGS` containing the same four relax flags, `-DFX_READ_COUNTER`
and the selected `-DCGFX_BATTLE_AUTO_REPLACE=0` or `=1` (no shipping no-USB flag).
The baseline bead creates the co-located slim suite before that command is used.
Retain any existing world-profile selection consistently across control/candidates.

Once settled, run `make final-gate` for each replacement build with its host,
shipping and device flags consistently selected and distinct BUILD_DIR/logs.
No full gate in each parallel worker: focused checks first, integrated gates in
the final evaluation bead. No mandatory manual visual review.

Known unrelated baseline failures: M1 save device fixtures use removed SaveFile
fields/APIs, and the prior stack suite reported a compaction expectation mismatch.
Capture fresh baseline outcomes. Do not weaken tests, repair save ownership or
claim an integrated pass if these remain. Report battle-specific verification
separately and leave final integrated acceptance blocked until its owner fixes it.

## Handoff, ownership and stop rules

Wave 0 freezes control, interfaces, storage union and native/device fixture.
Wave 1 may parallelize session/AI policy, feedback/scene, and compact-menu work
after those contracts exist; those workers do not edit shared flow/mode files.
Wave 2 integrates and measures both policies. Wave 3 produces the paired report,
integrated validation and adoption recommendation. Never mix unrelated cuts into
the control/candidate difference. Each checkpoint gets its own size record.

| Bead | Layer | Dependencies |
| --- | --- | --- |
| `bsd.1` | Detailed plan (complete) | None |
| `bsd.2` | Control, layout/interfaces, paired fixture | `bsd.1` |
| `bsd.3` | Party/session and voluntary-switch policy | `bsd.2` |
| `bsd.4` | Feedback, captions and scene | `bsd.2` |
| `bsd.5` | Compact menu | `bsd.2` |
| `bsd.6` | Shared flow/mode integration, paired measurements | `bsd.3`, `bsd.4`, `bsd.5` |
| `bsd.7` | Gates and headroom evaluation | `bsd.6` |

All IDs carry the `CreatureGathererFX-` prefix. Implementation children and epic
are deferred until handoff is requested; use actual Beads status to release them,
not a note that merely says ready. Do not claim or dispatch them during planning.

Pin test ownership before parallel dispatch: `.2` creates shared fixture/test
registration wiring; `.3` owns `tst/battle_slim_session_test.hpp`; `.4` owns
`tst/battle_slim_feedback_test.hpp`; `.5` owns `tst/battle_slim_menu_test.hpp`.
Workers use separate headers, with shared fixture definitions read-only.
`.6` owns integration assertions and device-suite registration. Scene helpers
owned by `.4` are `src/engine/battle/BattleScene.hpp/.cpp`. Shared Makefile/test
registration changes after scaffolding serialize through `.6`.

Luna performs implementation and size checks as requested, with only compact
results returned to the parent. Escalate an unresolved AVR lifetime/SPI/stack
design question for design review before coding around it. All workers must read
their bead and this contract; they are not alone and must preserve others' edits.
No worker may choose a new feature cut or reinterpret a frozen interaction.

Report control, intermediate and final flash/static bytes; difference versus
control; auto-minus-choice cost; painted peak/effective reserve; exact flags and
commands; test counts/failures; generation changes; source hashes; worker/gate/
orchestrator wall time; and remaining firmware headroom. Retain patches, final
ELFs, small logs and reports outside cleaned build caches. Do not commit/push or
copy spike code into production during this research. A later adoption needs its
own authorized bead and gate. Stop and report BLOCKED if reserve or valid battle
semantics fail; a target miss requires another design decision, not hidden cuts.
