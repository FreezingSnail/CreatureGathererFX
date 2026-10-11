# Fine-grained battle firmware flash audit

Measured 2026-10-10 against the current dirty shipping tree, including the
owner's applied compiler/type trims and uncommitted M1 work. No production
source was edited by this audit. All feature removals below are diagnostic
source edits in an isolated snapshot; their images are intentionally not
playable and are not acceptance builds.

## Controlled image and reproduction

The isolated source tree is `build/battle-fine-audit/CreatureGathererFX`. Its
content hash (excluding `.git`, `.beads`, `.codex`, and nested `build/`) is
`45837e4e1bc128cd7c0167e06d7d3d056d6385ae128c5e6de6def215df31cc14`.
It came from HEAD `2644476fe5e5c82a22ce4cb89c810fb076a91601` plus the dirty
tracked diff SHA-256
`0594b94d652abe858c47ffb706bd55f0d40ed03e59294e3c96d86a3feb59f9ea` and the
then-present untracked project files. `make gen` ran only inside the snapshot.

Toolchain: Arduino CLI 1.2.0, Arduboy homemade AVR core 1.4.0, AVR-GCC
7.3.0-atmel3.6.1-arduino7, FQBN `arduboy-homemade:avr:arduboy-fx`.
All builds used C/C++/link relax and shared prologues, strict-X and
`-fno-move-loop-invariants`; C++ additionally used
`-DCGFX_SHIPPING_NO_USB -fno-inline-small-functions`. The shipping physical
flash limit remained 29,696 B and static RAM limit 2,160 B.

```sh
PATH=/Users/connorfranc/Applications/CreatureGathererTools/bin:$PATH make gen
PATH=/Users/connorfranc/Applications/CreatureGathererTools/bin:$PATH make build \
  BUILD_DIR=build/battle-fine-audit/build-<case> \
  AVR_SHIPPING_CPP_FLAGS='-mrelax -mcall-prologues -fno-move-loop-invariants -mstrict-X -DCGFX_SHIPPING_NO_USB -fno-inline-small-functions'
```

The current control reproduces at **29,790 B firmware flash / 1,842 B static
RAM**, which is 94 B over the physical flash limit. `avr-size` independently
reported 29,790 B program and 1,842 B data for the retained control ELF. The
build command therefore exits at the expected flash-budget check, but its ELF
and whole-image measurement are retained. Rows marked `MEASURED_OVER_BUDGET`
also compiled and produced a size-bearing ELF; their resulting image still
exceeds the same limit. They are not passes of the shipping gate.

Each matrix row was rebuilt from the restored control sources with one local
diagnostic edit. `bytes released` is control flash minus that row's flash;
positive values mean the diagnostic image is smaller. Static RAM is reported
separately. Rows overlap through LTO, shared callees, and reachability. Do not
sum them. The raw row descriptions and measurements are in
[`battle-flash-audit.csv`](battle-flash-audit.csv); per-build logs and ELFs are
under `build/battle-fine-audit/`. The retained control ELF is
`build/battle-fine-audit/build-baseline-repro/arduino-build/fx/CreatureGathererFX.ino.elf`;
the complete-damage and whole-family ELFs are in the similarly named
`build-full-damage-core` and `build-current-family-removal` directories.

## Measured boundaries

| Boundary removed or replaced in the diagnostic copy | Flash released | Static RAM change | What the measurement includes |
|---|---:|---:|---|
| AI strongest-move scorer | 170 B | 0 B | Replaced best-damage scan with first legal move; utility and tactical switch policy stayed. |
| AI utility/setup/heal policy | 214 B | 0 B | Replaced non-switch policy with its damage-choice result; strongest-move scorer and switch policy stayed. |
| AI tactical switch-risk policy | 384 B | 0 B | Returned the preferred action without evaluating threatened switches. |
| Priority and speed ordering | 278 B | 0 B | Fixed the first actor to Player instead of comparing action priority and staged speed. |
| Damage type/STAB/status modifier | 22 B | 0 B | Used neutral modifier in damage arithmetic; presentation's separate effectiveness lookup stayed reachable. |
| Damage attack/defense stages | 78 B | 0 B | Used raw attack and defense terms in the damage formula; speed stages stayed. |
| Physical/special damage selection | 38 B | 0 B | Forced the physical stat pair in place of selecting stats from move class. |
| Complete `computeDamage` core | 776 B | −18 B | Replaced the complete entrypoint with zero damage, removing its damage formula while leaving other AI, attack presentation and status paths linked. |
| Move effect application | 662 B | 0 B | Removed the post-hit authored-effect loop; end-turn status effects stayed. |
| Action status gates | 240 B | 0 B | Replaced attack and gather turn gates with the no-gate result. |
| End-turn resolution (ticks and terminal handling) | 434 B | 0 B | Replaced the session's end-turn resolver call with an empty EndTurn result, removing ticks and its terminal handling. |
| Move-use consumption and exhaustion | 84 B | 0 B | Removed attack-use spending and resolver exhaustion rejection; menu-side availability checks stayed. |
| Gather progress action | 136 B | 0 B | Replaced gather progress with a refused Gather result. |
| Escape terminal action | 16 B | 0 B | Made Escape return a refused result. |
| `applySwitch` active/bench rebuild | 1,194 B | 0 B | Replaced the helper with a refused Switch result; wild/trainer setup stayed. |
| Battle menu input router | 1,392 B | −12 B | Replaced `menu.run` with a fixed first-move intent; rendering stayed. |
| Battle menu drawing | 990 B | −4 B | Removed the battle `menu.printMenu(view)` call. |
| Battle sprites and HUD scene | 942 B | 0 B | Removed the `drawScene` call, including both creature sprites and HP bars. |
| Presenter feedback/caption drawing | 1,152 B | 0 B | Replaced `BattlePresenter::draw()` with an empty method; timing/state transitions stayed. |
| Wild/trainer party setup | 580 B | 0 B | Replaced the session's two setup calls with an over-state; switching helper remained live. |
| Whole battle-family removal reference | 17,914 B | −58 B | Removed encounter dispatch into battle, BattleFlow, battle menu display, battle scene, and resulting unreachable battle runtime from the shipping path. |

The broad whole-family row leaves **11,876 B flash / 1,784 B static RAM**.
`avr-size` confirmed both values. It is a reachability reference for the
current tree, not a proposed gameplay cut and not the sum of the rows above.
The exact `applySwitch` helper replacement releases 1,194 B. A separate,
broader call-site bypass (replacing both session calls with `false`) measured
1,268 B; that result includes caller control-flow effects and overlaps the
helper measurement.

Two visual simplification spikes increased image size: replacing impact shake
with zero offsets produced **−108 B released** (108 B larger), and removing
the `overlay(view)` call produced **−12 B released** (12 B larger). Their ELFs
and logs are retained under `build/battle-fine-audit/build-impact_shake_animation`
and `build/battle-fine-audit/build-hp_facts_overlay_animation`. These results
do not support dropping either visual behavior as a flash-saving change.

## Runtime and data inventory

| Area | Live firmware work and current reachability |
|---|---|
| `Damage.cpp` | Integer physical/special power, staged attack/defense, type matchup, status modifiers and STAB. Damage type modifiers and type-effectiveness presentation are separate call paths. |
| `Effects.cpp` / `Resolve.cpp` | Move stat/status effects, attack/gather action gates, post-hit effects, end-turn ticks, faint flags, terminal outcomes, gathering progress, escape refusal/exit, and the partially implemented item resolver. |
| `Ai.cpp` | Legal-move scoring by damage, lethal preference, regeneration/setup choices, threat evaluation and trainer tactical switching. The three AI rows isolate scoring, utility selection and switch-risk choice. |
| `BattleSession.cpp` | Player intent validation, opponent planning, action ordering, action/result cursor, forced replacement, end-turn transition, outcome handling, and synchronization of player HP to the party. |
| `BattleSetup.cpp` | Wild/trainer party preparation and the shared active/bench reconstruction used for voluntary and forced switches. `applySwitch` crosses the FX read boundary during transitions. |
| `BattleFlow.cpp` / `MenuV2.cpp` | Battle choice and replacement menus, intent routing, presentation ownership, terminal completion and battle exit. Input routing and menu drawing have separate rows. |
| `BattlePresenter.cpp` | Prepared names/details/captions, action facts, HP overlays, animation timing, impact offsets, faint/replacement/terminal stages and feedback drawing. |
| `CreatureGathererFX.ino` / `draw.h` | Battle frame dispatch, native creature sprites, HP bars and feedback presentation. External sprite rasters are read from the FX cart. |
| Item route | A UseItem resolver exists, and `SelectItem` exists in the intent enum, but the live session intent switch does not accept `SelectItem`; no complete battle item-selection path is integrated. This audit does not claim the partial resolver is zero-cost. |

The FX cart is external to application firmware. Existing inventory data gives
742,765 B of FX data plus the reserved save area; asset bytes do not become
firmware flash just because the code reads or draws them. Reader, selector,
transition and renderer code remain firmware costs.

## Design directions worth measuring next

- Keep the tactical AI. Compare compact shared integer helpers for move scoring,
  setup checks and threat comparison; the isolated deltas are 170 B, 214 B and
  384 B, with shared computations between them. These are upper boundary
  references, not guaranteed savings from a rewrite.
- Preserve switching while examining the transition representation and shared
  party loading. The active/bench rebuild is 1,194 B and wild/trainer setup is
  580 B. Any redesign must retain invalid-slot handling, HP/modifier ownership,
  and the transition-time FX read contract.
- Preserve battle visuals while reviewing common presenter drawing and data
  preparation. Feedback drawing and scene/HUD rendering are 1,152 B and 942 B;
  menu input and drawing are 1,392 B and 990 B. Simplifying shake/HP overlay
  dispatch is not a candidate based on these compiler results because both
  local simplifications increased flash.
- Keep damage and status behavior while looking for shared branch/data
  representation across the damage kernel, effects and turn resolver. Damage
  arithmetic pieces individually release only 22–78 B here, while post-hit
  effects and end-turn ticks are 662 B and 434 B. Any smaller replacement
  needs another whole-image build before it can be credited.

No gameplay removal or redesign was adopted from this audit. Elapsed audit
wall time was about 12 minutes; the compiler/build runner did not record a
separate aggregate compile timer. No functional tests or device gate were run
on these deliberately incomplete diagnostic images.
