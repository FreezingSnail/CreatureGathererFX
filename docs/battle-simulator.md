# Battle simulation and balance toolchain

The battle engine is the **5-type RPS duel**: both sides pick hidden each round
(attack or switch), the pick types decide win/lose/neutral, and damage is flat.
See `docs/duel-system-brief.md` for the rules and `docs/battle-wheel-matrix.md`
for the type web and sample teams.

The retired ordered-turn `make sim` batch runner (`tools/battle-sim/`) no longer
reflects the duel and was removed; its source and provenance are preserved under
`archive/battle-legacy/tools/battle-sim/` and its reports under
`archive/battle-legacy/`. Do not resurrect those instructions — the current
toolchain below is the accepted seam.

## Acceptance: `make sim-test`

`make sim-test` is the permanent, reproducible check of the shipped duel kernel.
It builds a host driver over the **real** `src/engine/battle/DuelKernel.hpp` and
compares it, strictly, against the canonical browser/CommonJS mock.

```sh
make sim-test
# equivalent: node --test tools/tests/duel-kernel-parity_test.mjs
```

`tools/tests/duel-kernel-parity_test.mjs` compiles
`tools/duel-parity/duel_kernel_driver.cpp` (when stale) into
`build/duel-sim/duel_kernel_driver` and drives it over a host line protocol. The
6 cases cover the type relation, folded `attackMask` kits, RNG tie-breaking,
TRADE rounds with asymmetric modifiers and dual-typed defenders, and the AI
persona picks. No cases are quarantined or skipped. The wire contract, command
grammar, and mask bit order are documented in `tools/duel-parity/README.md`.

The driver rebuilds on every run, so a change to `DuelKernel.hpp` or
`BattleRng.hpp` is picked up automatically.

## Balance measurement: `tools/battle-duel-spike.mjs`

Balance work runs through the duel balance spike, a Node tool over the same
canonical mock (`mock/battle-mock.js`). It measures the default 5-type table
(matrix `5`) with three-creature teams.

```sh
node tools/battle-duel-spike.mjs --battles 1000 --rounds 200
node tools/battle-duel-spike.mjs --persona reader --policy counter --read 45
node tools/battle-duel-spike.mjs --pair-sweep --battles 2 --rounds 30
```

- **Personas** (AI): `reader`, `frequency`, `anti-repeat`, `exploiter`,
  `random`.
- **Player policies**: `random`, `repeat`, `counter`, `rotation`, `swap`,
  `tier`, `anti-repeat`. `tier` mirrors the AI's non-read branch: it picks an
  owned option at the best creature-type x1.5/x0.5 matchup tier (the shipped
  up/down marker a player can read), so the paired baseline reflects that
  decision the tier web rewards.
- **Read rates**: 0/25/50/75/100 %.

Policies and the AI see only *revealed* history; a policy never reads the
opponent's current hidden pick.

The tool reports wins/losses/draws/timeouts **separately** and never folds a
timeout into a win, loss, or draw. It also reports round-time behaviour metrics
(median/p95 rounds, switch frequency, one-hit KOs, type and species skew). With
`--pair-sweep` it pins teams per authored pair and splits outcomes per policy and
read rate so paired comparisons stay controlled. Identical options and seed
produce identical reports.

Its own regression suite is
`node --test tools/tests/battle-duel-spike_test.mjs`, which asserts determinism,
accounting, and rotation/read behaviour. The wider duel tool family and their
test counts are listed in `docs/duel-system-brief.md`.

### Machine-readable artifact: `--json`

Adding `--json` switches the tool from the human-readable text (the default) to a
single versioned JSON object printed with two-space indentation. Text output is
unchanged when `--json` is absent.

```sh
node tools/battle-duel-spike.mjs --json --pair-sweep --battles 1000 --rounds 200
```

The object's `schemaVersion` is `1` (exported as `DUEL_SPIKE_SCHEMA_VERSION`);
bump it on any breaking field change. Additive fields keep the version. Shape:

```jsonc
{
  "schemaVersion": 1,
  "mode": "pair-sweep",           // or "grid" when --pair-sweep is absent
  "config": {
    "matrixId": "5", "teamSize": 3, "scoring": "bonus", "maxRounds": 200,
    "battles": 1000, "persona": "reader",
    "policies": ["random", "repeat", "counter", "rotation", "swap", "tier", "anti-repeat"],
    "rates": [0, 25, 50, 75, 100]
  },
  "provenance": {
    "gitRevision": "<40-hex sha or null>",
    "gitDirty": false,
    "nodeVersion": "vX.Y.Z",
    "seedKeys": { "scheme": "seed = 1..battles per cell (identical across cells)",
                  "battles": 1000 },
    "files": { "tools/battle-duel-spike.mjs": "<sha256>",
               "mock/battle-mock.js": "<sha256>",
               "src/engine/battle/DuelKernel.hpp": "<sha256>" },
    "fixtures": { "creatures": "<sha256>", "matrix5": "<sha256>", "teams": "<sha256>" }
  },
  "pairs": [                       // pair-sweep mode
    { "you": "balanced", "foe": "anchors",
      "wins": 0, "losses": 0, "draws": 0, "timeouts": 0, "total": 0,
      "playerWinRate": 0.0, "flags": { "timeouts": false, "sweep": false },
      "cells": [ /* one per policy x read-rate */ ] }
  ],
  "flagged": [ /* pairs with timeouts or a >80% sweep in every cell */ ]
}
```

Each **cell** is the `runMatrix` aggregate over seed keys `1..battles` for one
`policy`/`rate` (and, in grid mode, `persona`):

```jsonc
{
  "policy": "counter", "rate": 100, "persona": "reader",   // persona: grid mode only
  "seed": { "from": 1, "to": 1000, "count": 1000 },
  "battles": 1000,
  "wins": 0, "losses": 0, "draws": 0, "timeouts": 0,
  "rounds": 0, "avgRounds": 0.0, "medianRounds": 0, "p95Rounds": 0,
  "switches": 0, "switchRate": 0.0,
  "kos": 0, "oneHitKOs": 0,
  "attacks": 0, "typeUse": { /* folded type -> committed attacks */ },
  "speciesSamples": 0, "speciesUse": { /* species -> party samples */ }
}
```

Provenance notes: file hashes are over raw bytes; fixture hashes are over a
canonical (key-sorted) JSON serialization, so they do not depend on object
insertion order. No timestamps are emitted, so the same seed/scenario produces
byte-identical JSON; the seed keys are identical across cells, so same-seed
cells are correlated by construction and are not independent samples.

## Reporting seam

- **Kernel correctness** is reported by `make sim-test`
  (`tools/tests/duel-kernel-parity_test.mjs`), strict C++/JS parity with no
  skips.
- **Balance and pacing** are reported by `tools/battle-duel-spike.mjs`, with
  timeouts kept as an explicit outcome category.
- **Generated data and pack integrity** are reported by the repository gates
  (`make verify-generated`, `make test-manifest`, `make test-generated-libs`,
  `make test-pack-parity`).

The legacy CSV batch reports (`matches.csv`, `summary.csv`) belonged to the
retired ordered-turn runner; they are archived with it and are not produced by
the current toolchain.
