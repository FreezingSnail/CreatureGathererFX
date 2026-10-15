# Duel system: proposed spike contract

Planning deliverable for CreatureGathererFX-d73i, 2026-10-15. **Owner authorized
implementation dispatch to DeepSeek as able**, including the legacy archive
amendment. The plan approval gate `CreatureGathererFX-pjfd` is resolved.
The epic's design remains the brief; this document supplies the spike contracts.
Child beads retain serial prerequisites. Measured results still require review;
production adoption remains a separate owner decision. The storage reserve still
applies before any AVR build or generation.

## Findings that shape the experiments

- `Player` already has three party entries and three HP bytes. `BattleState` is
  124 B on AVR; the compact `BattleSession` is 142 B. These are representation
  contracts, not whole-program SRAM measurements.
- Wild setup currently creates one opponent. Trainer setup supports up to three.
  Keep ordinary wild encounters at 3v1 capacity, and trainer duels at up to 3v3;
  do not invent two additional wild creatures or require a full player party.
- `BattleSetup.cpp` owns transition metadata reads. AI bench-counter selection
  therefore needs cached bench attack-type masks; it cannot inspect FX each round.
- `BattleFlow` owns fresh input edges, the menu/feedback union, terminal callbacks
  and world restoration. A new round result must represent both hits together;
  feeding two legacy sequential actions through it would break simultaneous KO.
- `Outcome` has no Draw. Adding it requires checking trainer callbacks, rewards,
  boss flags and defeat recovery, not merely adding a caption.
- `mock/battle-mock.js` retains eight revealed attack types for persona history.
  Its dual-type rule is advantage first: any winning defender type gives x1.5,
  otherwise any losing type gives x0.5, otherwise x1. No multiplied dual weakness
  or STAB. No-read choices prefer the best creature-matchup tier; failed reads
  with no available counter fall back uniformly across attack slots.
- `tools/battle-duel-spike.mjs` explicitly runs matrix `8`, team size 1. The
  brief's read-rate percentages are evidence for that experiment only. Its player
  counter policy calls `wheelBeats`, so changing only the matrix option would
  still give the wrong five-type policy. The new balance run must fix that.
- The five-type tournament is asymmetric: spirit has one outgoing win, earth
  and water three each. Coverage and team balance need measurement; a complete
  tournament does not imply equally strong types.
- Native tooling package `cgfx-core` owns `src/battle_presets.rs`,
  `src/config.rs` and `src/builder.rs`. The builder already checks every supported
  level's max HP <=254 and the one-byte HP seed bound; preserve those checks.
  Folded duel outputs, retype validation and nonempty opponent attack kits are
  new authoring contracts, not guarantees to assume from current generation.

## Decisions proposed for approval

These answer all nine open questions in the brief. Approval accepts them as
spike inputs; measured results return for review before shipping adoption.

| Question | Proposed contract and reason |
| --- | --- |
| 1. Damage | Freeze 14/4/6 and x1.5/x0.5/x1. Compute integer amounts 21/7/14, 6/2/4, 9/3/6 respectively. No power, STAB, damage RNG, accuracy or speed order. Evaluate both hits against the same pre-hit state, then clamp both HP values at zero. Balance may recommend tuning later, with a new paired measurement. |
| 2. Switching | SWAP opens a two-row living-bench picker in original party order; A commits and B cancels without consuming a round. Suppress SWAP with no living bench. Both swaps complete and deal zero damage; one swap receives the opponent's scaled default win against the incoming type. Incoming KO restores the outgoing member if living, otherwise the first living member. Ordinary KO promotes the first living original slot. |
| 3. Types | Freeze the brief's ten edges and 8-to-5 fold. Fold moves and creature types during generation, deduplicate folded dual types, and preserve distinct dual types with the mock's advantage-first rule. Trainer retypes are encounter overrides and survive all switches. Player types derive from species; demo player retypes remain fixtures. Persistent per-instance retyping needs a separate save design. |
| 4. Personas | Spike random, reader, frequency and anti-repeat; exclude exploiter from beta firmware, retain it in the simulator. Proposed rates: wild/random 0%, introductory trainer/reader 25%, ordinary/frequency 50%, boss/anti-repeat 50%. All remain below 75%. Reuse persona/rate pairs across trainer groups. History resets per encounter and stores the last eight revealed player attack types; switches and gathering do not enter it. Tied predictions and tied eligible move slots use uniform RNG. |
| 5. Other actions | Keep gathering and escape through a wild-only action page reached with B from the pick row. Gathering adds existing tier progress, takes an opposing default-win hit, then checks KO before acquisition before the existing six-round flee countdown. A round with either active KO cannot acquire; replacement completes before the next choice. Both ordinary attack and gather rounds decrement that countdown once when eligible. Wild escape is immediate and cancels the pending round; trainers cannot escape. Keep world items; add no battle item interface. Status/utility moves supply no attack cell or effects. |
| 6. Stances | Keep as a browser demo only. Production uses creature-owned attack slots. |
| 7. Assets | Append feedback strings to `data/text/strings.txt`; use `fxsprites.toml` and `make gen`. Vocabulary: BONUS WIN, BONUS LOSE, WEAK WIN, WEAK LOSE, RPS WIN, RPS LOSE, TRADE, DEFAULT WIN, SWAP, DRAW, WAIT; reuse existing terminal/gather/escape text where suitable. Import existing five icon designs through authored image inputs. Keep names, HP numbers/bars, round number, badges and alive/active pips. No power digits or AI READ tell in release firmware. |
| 8. Party/save | Keep three slots, species IDs, levels/XP, current derived max HP, collection-instance identities and original-slot HP ownership. No save version, 8 B StoreRecord, 96 B snapshot or sector-offset changes. Boxed HP encoding remains intact. Sync every participating player's HP on exit, including swaps, escape, draw and defeat. Draw grants no victory XP/boss completion and uses defeat recovery to avoid returning an exhausted party to the world. |
| 9. Measurement | One serialized lane, one source control snapshot, one reused build root, paired whole-image measurements and compact evidence as specified below. A full gate occurs once at the accepted batch checkpoint, per the owner's gate-cadence instruction. |

Additional pinned edge rules:

- Empty slots and already-KO members cannot be selected. Species zero is valid;
  level zero is the empty sentinel. Start with the first living occupied member;
  an exhausted player party follows existing defeat handling.
- Preserve one cell per usable authored attack slot, in slot order, even when
  multiple moves fold to the same type. Filter using the same attack eligibility
  rule as the mock, expressed and validated by the generator, rather than
  interpreting effect bytes during battle.
- Saved player kits may contain no attack moves. Show WAIT in that case; it
  commits no attack and receives the opponent's default win. SWAP and wild
  actions remain available. Validate every authored opponent kit has at least
  one attack, including replacements, so normal encounters cannot deadlock
  between two unusable kits. Never silently give a creature an invented move.
- Commit the opponent's pick once on entry to Choice, before accepting the
  player's current choice. AI can inspect cached public teams and previously
  revealed history only. Navigation/cancel/repaint consumes no new RNG. Record
  player history only after reveal, for use next round.
- State sequence: Choice -> concealed commitment -> reveal/resolution -> result
  dwell -> automatic replacements/terminal or next Choice. At 52 fps, use 29
  ticks for the 550 ms concealed interval (557.7 ms) and 68 for the mock's
  1300 ms result dwell (1307.7 ms). No same-edge skip into a fresh choice. Both
  reveal types and both HP changes must be visible before terminal dismissal.

## Experiment sequence

Four serial child beads match the brief. Each records its control, candidate,
incremental delta and cumulative delta against the initial control. Stop a slice
when its measurements fail the budget; trim or bring the deficit to the owner.

| Child blocked by plan approval | Measured item |
| --- | --- |
| CreatureGathererFX-d73i.1 | Kernel, session and personas |
| CreatureGathererFX-d73i.2 | Compact interface and assets |
| CreatureGathererFX-d73i.3 | Legacy archival and state compaction |
| CreatureGathererFX-d73i.4 | Generated content and balance |

### 1. Duel kernel, session and personas behind CGFX_DUEL_BATTLE

Use an isolated source workspace and opt-in compile flag. Keep the existing
Flow/session entry and exit shell and current scene/menu temporarily. Adapt
selection and feedback just enough to exercise hidden commitment and one atomic
round; the old menu's PP gates must not limit duel choices. Legacy builds keep
their current behavior. The flag excludes legacy resolver/AI execution from the
duel build immediately; step 3 is not credited with bytes LTO already removed.

Implement the ten-edge relation, integer damage, switch/KO rules, draw, WAIT,
cached attack-type masks and four persona policies. Use a single resident round
result and original-slot HP, not full copies of two battle states. Add the
gather/escape bridge now so the first device slice is a complete encounter loop.
Temporary fixture tables supply folded types and teams until step 4. Do not
mutate canonical authored eight-type inputs for the control build.

Before the first replacement of existing battle code, preserve a source-only
reference under `archive/battle-legacy/`. Capture the original battle engine,
AI, setup/session, menus, rendering/presentation, related tests and simulator,
plus the authored data/build inputs needed to understand and reproduce it.
Keep original relative paths and an archive README identifying source revision,
any uncommitted source patch, tool versions, build/test commands, dependencies
and known limitations. Include a file/hash inventory. Reference shared unchanged
assets explicitly rather than duplicating complete asset or repository trees.
Archive files must be outside shipping compilation and ordinary test discovery.
This is a durable reference deliverable, separate from disposable spike outputs;
Git history alone does not fulfill it. Create the archive only after plan approval.

Compare five PROGMEM win masks with a packed FX chart loaded once at encounter
entry. Both implement the same 25 relations; choose by whole-image flash and
stack/SRAM, including reader overhead. Never read the FX chart per lookup. The
brief's proposed FX placement is amended if PROGMEM is smaller overall. Keep
one implementation after this bounded comparison.

Proof: all 25 pick relations; each modifier and dual-type conflict; win/chip,
trade, simultaneous KO/draw; both switches, incoming KO and original-slot HP;
no living/empty/zero-attack states; gather/escape precedence; fixed-seed persona
predictions and no hidden-pick access; zero per-choice metadata reads. Exercise
switches, terminal callbacks and save after a duel under painted stack checks.

### 2. Compact pick row, feedback, badges and team pips

Replace the temporary menu/presenter route with the approved bonus spelling.
At most four attack cells plus SWAP, with a WAIT cell for an unusable kit. Add
the two-row target picker and wild action page. Reuse existing sprite/bar
rendering and the UI union; cache text addresses, types and markers on transitions.
Prepare new captions/icons via authored inputs and `make gen`.

Proof: cursor bounds for zero/one/four attacks; duplicate folded types; target
selection/cancel; dead/empty pips and active underline; two distinct badges or
one deduplicated badge; simultaneous damage visibility; held buttons and edges
at every phase boundary; zero metadata reads on navigation, dwell and draw.
Use automated framebuffer assertions for device layout. Manual visual acceptance
is required only if requested by the owner.

### 3. Archive legacy battle machinery and compact active state

In the isolated candidate, replace remaining battle-only `Combatant`, bench,
move-use, stage/status/effect, ordered-turn and presenter storage with the
measured duel representation. Finish and verify the source archive captured in
step 1 before retiring any remaining legacy shipping routes. Retire those routes without removing
world Creature fields still used for persistence, growth or menus. Preserve the
gather/escape bridge and terminal integration. Keep any legacy-only host
reference outside shipping reachability where needed for existing tools.

Proof: source/call-site inventory of each deletion, same seeded duel traces
before/after compaction, full HP ownership through switching and defeat,
save/reload plus boxed duplicates/KO, transition counters and painted stack.
Verify the archive inventory against the original control, account for all
superseded battle sources/tests/data, and confirm no archived translation unit
enters shipping builds. Retain the archive README and reproduction inputs in the
repository; do not discard them during build cleanup.
Expect some deletions to save zero bytes because LTO already discarded them;
report the actual delta. This step does not itself authorize shipping adoption.

### 4. Generated folded data, beta teams and balance

Inspect the native generator in sibling `CreatureGathererTools` before schema
edits; current project config exposes presets but no duel fold/retype contract.
Define an additive duel output, leaving legacy record layouts and layout entry
order intact. Generate compact attack slots, folded unique creature types,
trainer overrides, bench masks and persona/rate values. Validate bounds, types,
sentinels, max HP <=254, and nonempty opponent attack kits in authoring tools.
Change native tooling only within its permitted workspace/approval boundary;
do not substitute a local converter. Use a pinned tool version for regeneration.

Author beta encounter teams explicitly; demo presets are fixtures, not beta
content. Retain 3v1 wild encounters, incomplete player parties and three-member
trainer teams. Retypes change badges/modifiers only, not species, HP or move
ownership. Verify switch reconstruction reapplies the same override.

Extend the native C++ host simulator to run the actual duel kernel, and compare
selected deterministic traces against the browser reference. Extend the JS
balance tool to five-type, real team size, folded policy lookups and beta HP
levels. Compare all authored beta trainer pairings and demo presets across
random/repeat/counter/rotation/swap policies, seeds 1..1000 and rates
0/25/50/75/100, capped at 200 rounds. Report wins/losses/draws/timeouts separately,
median/p95 rounds, switch frequency, one-hit KOs and species/type skew. Player
policies observe only revealed history. Flag any timeout, trivial full-team
sweep or pairing with >80% wins across all policies for review. These are
review triggers, not permission to alter constants or declare balance solved.

Proof: native generator checks, generated library/image parity, symbolic IDs,
first/last/sentinel entries, addresses above 0x010000, retypes across switching,
ordinary new game -> wild gather -> party/store -> trainer -> reward/defeat ->
save/reload. Run the one full integration gate here after fixes settle.

## Commands and acceptance budgets

Commands below are to run **after approval**, not during this planning pass.
Resolve `ARDENS` to a captureserial-capable binary and `cgfx-tools` to the pinned
native version. Reuse `BUILD_DIR=build`; retain reports in `build/reports/d73i/`
and the integrated gate in `build/final-gate/`. Root artifacts already exist;
preserve their provenance/evidence before overwriting them. The lane owner is
the agent executing the currently approved slice; no parallel AVR/gen work.

Before any build and after each slice:

```sh
df -h . /private/tmp
du -sh build
make doctor
```

Planning inspection found **5.8 GiB free**, repository `build/` **775 MiB**, and
no `/private/tmp/CreatureGathererFX`. No cleanup was performed. Builds are
blocked until free space reaches 10 GiB. Identify disposable data and preview
cleanup before deleting; preserve unknown source/evidence. Both repo build and
owned session scratch budgets are 5 GiB each. Inventory any chosen scratch
root with `du -sh` before use; retain no duplicate build trees.

Set flag variables with these exact defaults (record any required project
changes in the report); candidate flags append, never replace shipping options:

```sh
DUEL_SHIP_FLAGS='-mrelax -DCGFX_SHIPPING_NO_USB -fno-inline-small-functions -DCGFX_BATTLE_NO_FACTS -DCGFX_BATTLE_COMPACT_RESULT -DCGFX_COMPACT_WARP_VM -DCGFX_DUEL_BATTLE'
DUEL_DEVICE_FLAGS='-mrelax -DFX_READ_COUNTER -DCGFX_COMPACT_WARP_VM -DCGFX_DUEL_BATTLE'
make ram BUILD_DIR=build
make ram BUILD_DIR=build AVR_SHIPPING_CPP_FLAGS="$DUEL_SHIP_FLAGS"
make test BUILD_DIR=build CPPFLAGS='-I. -DCGFX_DUEL_BATTLE'
make testvm BUILD_DIR=build
make fxtest-spike BUILD_DIR=build ARDENS="$ARDENS" AVR_FXTEST_CPP_FLAGS="$DUEL_DEVICE_FLAGS" FXTEST_SPIKE_INO=tst/fxdatatest/test_battle_duel.ino
```

`test_battle_duel.ino` and its co-located host/device assertions are planned new
permanent suites, not existing commands that have already passed. Extend
`test_stack` to execute the duel paths; merely compiling it with the flag does
not prove those paths ran. Host tests need the duel macro too. Keep legacy host
coverage separately with `make test BUILD_DIR=build` in steps 1 and 2. Extend
`make sim-test BUILD_DIR=build CPPFLAGS='-I. -DCGFX_DUEL_BATTLE'` in step 4 to
cover seeded simulator traces and the actual kernel.

Step 2 adds:

```sh
make gen BUILD_DIR=build
make verify-generated BUILD_DIR=build
make test-generated-libs BUILD_DIR=build
make test-pack-parity BUILD_DIR=build
```

For intentional packed changes, report the expected baseline mismatch, inspect
the generated set and update the approved parity baseline together; rerun until
it passes. Do not call a stale-baseline failure a successful parity check.

Step 3 adds:

```sh
make fxtest-headless BUILD_DIR=build ARDENS="$ARDENS" AVR_FXTEST_CPP_FLAGS="$DUEL_DEVICE_FLAGS" FXTEST_INOS='tst/fxdatatest/test_save.ino tst/fxdatatest/test_world_player.ino tst/fxdatatest/test_encounter.ino'
```

Step 4 adds the native generator's library tests, JS native tests and a batch
gate. Add permanent duel authoring cases under the existing `cgfx-core` native
test modules so the first command runs them:

```sh
cargo test --manifest-path ../CreatureGathererTools/Cargo.toml -p cgfx-core --lib
node --test mock/battle-mock.test.js tools/tests/battle-duel-spike_test.mjs tools/tests/battle-teams_test.mjs tools/tests/battle-matrices-spike_test.mjs
node tools/battle-duel-spike.mjs --battles 1000 --rounds 200
make sim-test BUILD_DIR=build CPPFLAGS='-I. -DCGFX_DUEL_BATTLE'
make test-manifest BUILD_DIR=build
make test-generated-libs BUILD_DIR=build
make verify-generated BUILD_DIR=build
make test-pack-parity BUILD_DIR=build
make final-gate BUILD_DIR=build ARDENS="$ARDENS" CPPFLAGS='-I. -DCGFX_DUEL_BATTLE' AVR_SHIPPING_CPP_FLAGS="$DUEL_SHIP_FLAGS" AVR_FXTEST_CPP_FLAGS="$DUEL_DEVICE_FLAGS"
```

The JS command must honor `--rounds` and run the specified five-type team matrix
after step 4; today its main does neither. Old device suites must be assigned
explicitly to legacy or duel contracts rather than disabled to get a green gate.
The final gate's shipping resource build must contain the duel flag; its success
does not authorize changing the production default.

Hard candidate limits for every slice: <=29,696 B program flash, <=2,160 B
static SRAM, >=219 B painted combined-chain headroom where USB ISR allowance is
needed (150 B effective reserve), and >=150 B mode-transition headroom. Measure
duel setup, both switches, AI, reveal/render, terminal callbacks and save chains.
No metadata FX reads per choice/update/render; roster/type/text reads only on
named transitions. Existing raster transfers remain allowed. Maximum update
plus render must fit the 19.23 ms frame period, measured on worst-case paths.

The initial historical control is 29,790 B, 94 B over the physical limit. Treat
that as a failing control to reproduce, not a waiver for the candidate. If a
fresh control overflows, preserve the compile diagnostic and linked ELF/size
when available; do not raise the shipping budget to make it pass. The brief's
3.5–5.5 KiB expected savings is a hypothesis. Target >=3,500 B cumulative savings
by step 4, with no static SRAM increase; report smaller savings for review even
when the hard limits pass. Do not add removal sensitivities to predict a result.

Before overwriting each build, retain one chosen control/final ELF, exact source
patch and required allowlisted inputs, compiler/tool versions, flags, hashes,
flash/SRAM/painted stack, frame timing, counts and relevant log tails. Keep
per-slice reports under `build/reports/d73i/` and commands/times/deviations in
`output.md`. Use one committed control snapshot plus patches; no recursive repo
copies. Clean positively identified disposable pack/compiler outputs after
preserving evidence, with the repository helper first:

```sh
make clean-build-artifacts BUILD_DIR=build
make clean-build-artifacts BUILD_DIR=build CLEAN_APPLY=1
df -h . /private/tmp
du -sh build
```

Inspect remaining sizes and abandoned `cgfx-pack.*` stages; an empty helper
preview is insufficient. Keep the active compatible cache within budget.

## Approval and handoff

Approve or amend the decision table before activating step 1. Each child is one
implement/measure/report loop; its result returns for acceptance before the next
slice is activated. Planning approval permits isolated experiments only.
Production adoption remains a separate explicit owner decision after the
measured candidate and batch gate. Keep the epic open until that work is accepted.
