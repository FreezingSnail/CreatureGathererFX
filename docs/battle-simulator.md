# Battle balance simulator

The batch runner uses generated creature, move, and trainer-preset fixtures, then
executes battles through the production `BattleSession`, resolver, effects,
damage, and opponent AI. It is a sequential host tool. Its outputs describe the
current engine rules; it does not add accuracy or critical-hit rolls.

## Build and run

Run commands from the repository root. `make sim` builds the host executable
and runs the two anchor presets with seed 1, both player policies, and a 100
turn cap. It replaces `build/balance/matches.csv` and
`build/balance/summary.csv` on each run. Everything under `build/` is ignored.

```sh
make sim
```

To collect an exhaustive ordered 1v1 matrix for all 64 generated species at
level 10, with ten trials per matchup and both player policies:

```sh
build/tools/battle-sim/battle-sim \
  --mode pairwise --species-count 64 --level 10 --trials 10 \
  --seed 20261004 --policy both --max-turns 100 \
  --output-dir build/balance/pairwise-level10
```

Every ordered species pair gets its own scenario, including mirror matchups and
both seat assignments (`pair_00_01` and `pair_01_00`). For a sampled 3v3 run,
species are selected without replacement across both teams. Each selection
chooses among the currently least-used species, keeping total appearances
balanced. Both player policies for one trial use the same teams.

```sh
build/tools/battle-sim/battle-sim \
  --mode random-3v3 --species-count 64 --level 10 --trials 100 \
  --seed 20261004 --policy both --max-turns 100 \
  --output-dir build/balance/random3-level10
```

The `anchors` mode runs the generated `opening` and `switch_drill` presets.
Use `--help` to see all options. The runner rejects batches above 100,000
matches. The default output directory is `build/balance`.

The `level` option sets generated species levels in pairwise and random-3v3
modes. Anchor creatures keep their authored preset levels; the report records
the selected level setting alongside the preset roster IDs.

## Reports

Both CSV files begin with `# key,value` metadata lines, followed by a fixed
header and data rows. Metadata records the report and simulator versions, the
production battle engine source, PRNG version, batch options, generator version,
and SHA-256 hashes for the generated creature, move, and preset fixtures. The
raw report stores the base seed and each match's derived `match_seed`.

`matches.csv` has one row per battle, in deterministic scenario, trial, then
policy order. It records ordered player/opponent species IDs, outcome, timeout
flag, turn count, final party HP totals, damage totals, and move use counts.
Move lists use `moveID:count` pairs separated by `|`. IDs refer to the generated
move fixture.

`summary.csv` groups rows by scenario, policy, player roster, and opponent
roster. Keeping both rosters in the group key preserves seat assignments. It
contains match, win, loss, and timeout counts, total turns, damage totals, and
aggregated move-use counts. Timeouts remain explicit and are never treated as
wins, losses, or draws.

Damage totals sum positive HP loss to the opposing active creature across
attack results. They exclude end-turn status damage and do not count self-hit
damage as damage dealt. If an attack's effects heal the target during that same
action, the metric reflects the net HP change captured by the engine result.
Move uses count attacks that produced an `Attack` result. Final HP is the sum
across each side's party.

The simulator uses versioned `xorshift32-v1` randomness for current engine
random rolls, sampled teams, and the random-valid-move policy. Identical
options, generated fixtures, and seed produce byte-identical reports.

## Replaying a match

Pass the row's scenario and derived `match_seed`, along with the level, policy,
and turn cap from the report metadata. Replay prints the ordered action/result
trace to standard output and does not read the CSV file.

```sh
build/tools/battle-sim/battle-sim \
  --replay-scenario pair_00_01 --replay-seed 123456789 \
  --level 10 --policy greedy --max-turns 100
```

For a `random_3v3` row, also pass its recorded player and opponent roster IDs.
Quote the team strings so the shell passes the `|` separators as data:

```sh
build/tools/battle-sim/battle-sim \
  --replay-scenario random_3v3 --replay-seed 123456789 \
  --level 10 --policy random-valid-move --max-turns 100 \
  --replay-player-team '1|4|7' --replay-opponent-team '2|5|9'
```

Named anchor scenarios (`opening` and `switch_drill`) rebuild directly from the
generated preset fixtures. The match seed is the per-match value from the raw
CSV, not the batch's original base seed. Replay prints every session result,
including switches and end-turn ticks, plus its final outcome, damage totals,
and turn count.

## Comparing balance changes

Save or copy a baseline report directory before changing creature or move data.
After regeneration, rerun the same mode, species count, level, trial count,
policy, turn cap, output directory, and base seed into a new directory. Compare
the two `summary.csv` files by scenario, policy, and ordered rosters; inspect
the raw rows for timeouts or surprising move selections. Matching options and
seeds preserve the matchup matrix and sampled 3v3 rosters, while the fixture
hashes show which generated data each report used.

## Utility and PP prototype (2026-10-04)

The game and simulator share these trial rules:

- No MP. Basic attacks without effects and power below 10 are unlimited.
- Strong attacks (power 10+) and healing/status moves have two uses per battle.
- Zero-power stat buffs/debuffs have three uses. Uses are per creature move slot,
  retained on switching, and reset at battle entry. Immunity spends a use;
  a prevented action does not. A fully exhausted loadout can pass from the move menu.
- Applied stat stages cap at +/-2, survive switches, and reset at battle entry.
- Regeneration heals 1/8 maximum HP for three end-turn ticks. Pinning and confusion
  also expire after three ticks. Status effects clear on switching; benches do not tick.
- Base damage is halved before type multipliers, with the existing minimum damage
  and immunity rules. Accuracy and critical-hit rolls are still absent.
- Old empty move ID 32 remains empty. Deluge now has semantic ID 44, retaining packed
  row 32. Authored IDs 36–43 map to packed rows 35–42; missing ID 35 is invalid.

`--policy tactical` takes a lethal attack first, heals when at half HP or below,
then uses an appropriate offensive buff, physical defense buff, or speed debuff
before attacking. `greedy` selects immediate damage. `both` remains greedy plus
random player policies. Production opponents use tactical AI under every policy;
these reports compare player decisions against that shared opponent, not symmetric
policy tournaments. `switch-tactical` is a separately labeled player policy: it
uses cached defensive type and defense data for bench members and selects through
the same `BattleSession` party intent as the game. It switches only when the
current legal incoming damage threatens at least half the active creature's HP,
and a live bench member improves estimated damage per current HP by at least 25%
while surviving the hit. It preserves a lethal attack, PP checks, original-slot
tie order and an action-consuming switch; one non-switch action clears its repeat
guard. It does not inspect bench moves or alter device opponent rules. Raw and
summary reports record switch counts; replay traces include the survival or
forced-replacement reason.

Fresh level-10 reports (seed 20261004, ten pairwise trials, 100 random-party trials,
100-turn cap) are in `build/balance/ogh-half/`: `pairwise`, `pairwise-tactical`,
`random3`, and `random3-tactical`. The same new kits before damage scaling are in
`build/balance/ogh-before-scale/pairwise`; the original baseline is retained in
`build/balance/collect-20261004`. The scaling comparison uses the same final fixtures and creature kits;
only the base-damage divisor differs.

| Player policy | Pairwise mean turns | Random party mean turns | Timeouts |
| --- | ---: | ---: | ---: |
| Greedy | 3.24 | 11.46 | 0 |
| Random move | 3.81 | 12.58 | 0 |
| Tactical | 3.28 | 11.69 | 0 |

These are pacing measurements, not final balance targets. Player-first speed ties,
no accuracy rolls, and the runtime/authored chart disagreement remain limitations.
Chart reconciliation is tracked in `CreatureGathererFX-62o`.

To build the playable utility preset:

```sh
make ram BUILD_DIR=build/ogh-playable \
  AVR_SHIPPING_CPP_FLAGS='-mrelax -mcall-prologues -fno-move-loop-invariants -mstrict-X -DCGFX_SHIPPING_NO_USB -DCGFX_TRAINER_DEMO -DCGFX_TRAINER_DEMO_UTILITY'
```

Use `build/ogh-playable/CreatureGathererFX.ino.hex` with the current `dist/fxdata.bin`
in Ardens. Reset restarts the battle. Support kits also exist for Cloud (Sweep) and
Flitfly (Deepthought) in canonical creature data. Four previously finite-only kits
now have a basic fallback, and trainer creatures have unlimited Thought.
