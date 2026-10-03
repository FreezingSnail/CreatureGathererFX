# Agent Instructions

Creature collecting demake for the **Arduboy FX** (ATmega32u4, 4-shade grayscale via ArduboyG
`L4_Triplane`). Read `README.md` for architecture/status and `docs/dev-flow.md` for the workflow
conventions distilled from past waves.

This project uses **bd** (beads) for issue tracking. Run `bd prime` for full workflow context.

## Commands

```sh
make test                  # host C++ suites (tst/)
make testvm                # ScriptVM suites (tst/script_tests/)
make fxtest-headless       # every tst/fxdatatest/*.ino suite through Ardens serial;
                           #   ARDENS=/path/to/Ardens required, else skip
make gen                   # generate data/sprites + pack the FX image (cgfx-tools on PATH)
make verify-generated      # non-mutating manifest check of generated artifacts
make test-manifest         # permanent generated-artifact tests
make test-generated-libs   # packed image <-> src/fxdata.h <-> generated sources
make test-pack-parity      # native packed-image SHA-256 baseline
make test-doctor           # setup-diagnostic tests
make build | mini | run    # shipping FX / Mini / interactive Ardens run
make ram                   # shipping FX build with flash/RAM report
make doctor                # tool readiness (cgfx-tools, arduino-cli, Ardens)
make check                 # gen + host + VM + manifest + generated-libs + verify-generated + fxtest
make fxtest-spike FXTEST_SPIKE_INO=tst/fxdatatest/test_save.ino
                           # selected device suite plus test_stack headroom check
make final-gate            # full make check followed by shipping RAM report; requires ARDENS
```

Before committing, run `make final-gate ARDENS=/path/to/Ardens`; it runs `make check` and then the
shipping RAM report. Confirm generation left no unexpected tracked changes, and run
`make test-pack-parity` when packed bytes can change. During a device spike, use
`make fxtest-spike FXTEST_SPIKE_INO=tst/fxdatatest/test_save.ino ARDENS=/path/to/Ardens` to run
the touched suite with `test_stack`; use `make fxtest-headless FXTEST_INOS=...` for other focused
device iterations. Run the final gate after implementation and edge-case fixes are settled.
`final-gate` prints concise results and preserves complete diagnostics in
`$(BUILD_DIR)/final-gate/{check,ram}.log` (override `FINAL_GATE_LOG_DIR` to relocate them).
It always runs every FX suite, even if a focused `FXTEST_INOS` override is present.

## Hard rules

- **Testing**: use each language's native framework; tests are permanent and co-located (`tst/`,
  `tst/script_tests/`, `tst/fxdatatest/`, `tools/tests/`); never write test code to `/tmp`; never
  use Python/perl/ruby as a harness to drive C++ tests.
- **Fixed point**: no `float`/`double` in device code; integer math only.
- **Generated artifacts**: `make gen` is the only generation entry. Never hand-edit
  `fxdata/generated/`, `tst/fxdatatest/generated/`, `src/vm/opcodes.hpp`, `src/flags/*`,
  `src/fxdata.h`, or `fxdata/Sprites.txt` — change the JSON/CSV/PNG/TOML source and regenerate.
  Stage generated sets together after a regen; a half-staged set once shipped a stale opcode table.
  Tests read generated symbolic constants, never literal record indices. `fxlayout.toml`
  declaration order is a permanent ABI.
- **Single FX image**: `dist/fxdata.bin` is the only flashable cart image; `src/fxdata.h` mirrors it.
- **SRAM is the binding constraint** (flash 61% used; globals were 2018/2560 B with a save path
  that overflows by ~262 B). Budget rules live in `docs/dev-flow.md`.
- **FX/OLED share SPI**: cart reads happen only on transitions, never per-step or per-frame. The
  per-frame read counter (jp8.1.7) keeps this enforceable.
- **24-bit FX table reads**: ArduboyFX 1.4.0's AVR `readIndexedUInt24` path returns an incorrect
  top byte. Route packed address-table reads through `FxRead::indexed24` in `src/lib/FxRead.hpp`;
  it reads the three big-endian bytes and reconstructs `uint24_t`. Keep the device table test,
  including entries above `0x010000`, as evidence for this workaround.

## Dev-cycle speed rules (from past waves)

- **Spike before any budget-touching bead.** Implement the thinnest end-to-end slice and report
  whole-image flash/RAM deltas; LTO makes per-symbol arithmetic meaningless.
- **Small beads, checkpoints inside.** One measured item per bead (qu9 pattern: measure each item
  separately so savings stay attributable). Stop for a trim when the budget is blown — do not finish
  the feature first.
- **Headroom reserve.** Plan against measured headroom minus a reserve; never land below ~150 B free.
- **Freeze interaction details before dispatch.** Formats, offsets, caps, labels and row models are
  pinned in the bead's `design`; workers may not reinterpret them.
- **Inspect data formats and edge entries before dispatch.** For rendering or generated-data work,
  inspect both the generator and its inputs. Pin whether assets include headers, how dimensions are
  derived, and how empty or sentinel IDs behave. A focused device spike should draw one ordinary
  entry and the relevant edge entry before the implementation expands to every case.
- **Check stack during the first device spike.** For changes that add calls, return large structs,
  or create local copies, run `test_stack` alongside the touched device suite before broadening the
  implementation. Record the headroom change and trim immediately if it crosses the reserve.
- **Finish edge-case review before the final gate.** Use host and focused device suites while
  iterating; check table bounds, empty entries, and sentinel IDs before running the integrated
  gate. Run the full gate after edits are settled. If it fails or code changes afterward, fix the
  cause and rerun the gate before committing; record failed attempts and their cause in `output.md`.
- **One full gate per bead.** The worker runs host/VM suites, touched device suites, and generation
  checks; the orchestrator runs the final full gate once before committing. Focused checks do not
  replace that final gate.
- **Parallelize non-overlapping beads.** Docs/tooling beads can run alongside code beads; only
  same-file work serializes. Split oversized beads before dispatch (2–3 logical layers each).
- **Route implementation by risk.** Use Sol for high-risk AVR, save-format, SPI, or stack work.
  Use Luna for bounded tests, tables, and documentation once the contract is pinned. For mixed work,
  have Sol settle the risky design first, then dispatch non-overlapping bounded implementation work.
- **Do not repeat blocked commands unchanged.** If a build or Git write is blocked by the sandbox at
  an external tool cache or repository metadata path, use the permitted access path for that command
  instead of retrying the same unprivileged invocation.
- **Log wall time per bead** (worker / gate / orchestrator) in `output.md` so the next retro is data.

## Worker protocol (agents spawned for bd tasks)

- One bead = one implement-verify loop. Read `bd show <id>`, implement, run the bead's exact
  verification commands, write the report to `output.md` (exact commands, tails, numbers, wall
  time, documented deviations), then `bd close <id>`.
- Keep reports and tool output compact: include exact commands, pass/fail counts, resource figures,
  elapsed time, and only the relevant output tail or failure trace. Do not paste full test logs or
  reopen full bead descriptions just to confirm a close already reported by `bd close`.
- **Do not commit or push** — the orchestrator commits between bead waves and runs the full gate.
- If the build cannot fit or a gate fails, report BLOCKED with the deficit and options; never fake a
  pass.
- Never implement, claim, dispatch a worker for, or close a bead labeled `human` (see Human-Only
  Beads below).

> **Architecture in one line:** Issues live in a local Dolt database
> (`.beads/dolt/`); cross-machine sync uses `bd dolt push/pull` (a
> git-compatible protocol), stored under `refs/dolt/data` on your git
> remote — separate from `refs/heads/*` where your code lives.
> `.beads/issues.jsonl` is a passive export, not the wire protocol.
>
> See [SYNC_CONCEPTS.md](https://github.com/gastownhall/beads/blob/main/docs/SYNC_CONCEPTS.md)
> for the one-screen overview and anti-patterns (don't treat JSONL as the
> source of truth; don't `bd import` during normal operation; don't
> reach for third-party Dolt hosting before trying the default).

## Quick Reference

```bash
bd ready              # Find available work
bd show <id>          # View issue details
bd update <id> --claim  # Claim work atomically
bd close <id>         # Complete work
bd dolt push          # Push beads data to remote
```

## Human-Only Beads

Beads labeled **`human`** are implemented by hand by the repository owner. The plan in the bead is
the deliverable; human authorship of the code is the point.

Agents MUST NOT implement, claim, dispatch a worker for, or close a bead labeled `human`. This
includes Maduin and any other background worker.

Use the exclusion when looking for work:

```bash
bd ready --exclude-label human     # available agent work
bd ready --label human             # the owner's queue
```

`human` is the only gating label; `human-only` is retired. Domain labels (`m0`, `save`, `ram`,
`test`, `data`, `build`, `design`, `roadmap`, `difficulty:*`) carry no gating meaning.

Agents MAY read these beads, answer questions about them, and review the resulting diff once the
owner reports the work done. Review checks the diff against the bead's `acceptance` field, not
against the agent's preferred implementation; a better human approach is a plan correction, not a
defect. File follow-up beads for leftovers.

When authoring a `human` bead, populate `description` (why now, current state with concrete
evidence), `design` (ordered `STEPS`, then `PITFALLS`), and `acceptance` (observable conditions
plus the exact commands). An empty `design` leaves the owner flying blind. See the `human-beads`
skill for the full contract.

## Non-Interactive Shell Commands

**ALWAYS use non-interactive flags** with file operations to avoid hanging on confirmation prompts.

Shell commands like `cp`, `mv`, and `rm` may be aliased to include `-i` (interactive) mode on some systems, causing the agent to hang indefinitely waiting for y/n input.

**Use these forms instead:**
```bash
# Force overwrite without prompting
cp -f source dest           # NOT: cp source dest
mv -f source dest           # NOT: mv source dest
rm -f file                  # NOT: rm file

# For recursive operations
rm -rf directory            # NOT: rm -r directory
cp -rf source dest          # NOT: cp -r source dest
```

**Other commands that may prompt:**
- `scp` - use `-o BatchMode=yes` for non-interactive
- `ssh` - use `-o BatchMode=yes` to fail instead of prompting
- `apt-get` - use `-y` flag
- `brew` - use `HOMEBREW_NO_AUTO_UPDATE=1` env var

<!-- BEGIN BEADS INTEGRATION v:1 profile:minimal hash:970c3bf2 -->
## Beads Issue Tracker

This project uses **bd (beads)** for issue tracking. Run `bd prime` to see full workflow context and commands.

### Quick Reference

```bash
bd ready              # Find available work
bd show <id>          # View issue details
bd update <id> --claim  # Claim work
bd close <id>         # Complete work
```

### Rules

- Use `bd` for ALL task tracking — do NOT use TodoWrite, TaskCreate, or markdown TODO lists
- Run `bd prime` for detailed command reference and session close protocol
- Use `bd remember` for persistent knowledge — do NOT use MEMORY.md files

**Architecture in one line:** issues live in a local Dolt DB; sync uses `refs/dolt/data` on your git remote; `.beads/issues.jsonl` is a passive export. See https://github.com/gastownhall/beads/blob/main/docs/SYNC_CONCEPTS.md for details and anti-patterns.

## Agent Context Profiles

The managed Beads block is task-tracking guidance, not permission to override repository, user, or orchestrator instructions.

- **Conservative (default)**: Use `bd` for task tracking. Do not run git commits, git pushes, or Dolt remote sync unless explicitly asked. At handoff, report changed files, validation, and suggested next commands.
- **Minimal**: Keep tool instruction files as pointers to `bd prime`; use the same conservative git policy unless active instructions say otherwise.
- **Team-maintainer**: Only when the repository explicitly opts in, agents may close beads, run quality gates, commit, and push as part of session close. A current "do not commit" or "do not push" instruction still wins.

## Session Completion

This protocol applies when ending a Beads implementation workflow. It is subordinate to explicit user, repository, and orchestrator instructions.

1. **File issues for remaining work** - Create beads for anything that needs follow-up
2. **Run quality gates** (if code changed) - Tests, linters, builds
3. **Update issue status** - Close finished work, update in-progress items
4. **Handle git/sync by active profile**:
   ```bash
   # Conservative/minimal/default: report status and proposed commands; wait for approval.
   git status

   # Team-maintainer opt-in only, unless current instructions forbid it:
   git pull --rebase
   bd dolt push
   git push
   git status
   ```
5. **Hand off** - Summarize changes, validation, issue status, and any blocked sync/commit/push step

**Critical rules:**
- Explicit user or orchestrator instructions override this Beads block.
- Do not commit or push without clear authority from the active profile or the current user request.
- If a required sync or push is blocked, stop and report the exact command and error.
<!-- END BEADS INTEGRATION -->

<!-- BEGIN BEADS CODEX SETUP: generated by bd setup codex -->
## Beads Issue Tracker

Use Beads (`bd`) for durable task tracking in repositories that include it. Use the `beads` skill at `.agents/skills/beads/SKILL.md` (project install) or `~/.agents/skills/beads/SKILL.md` (global install) for Beads workflow guidance, then use the `bd` CLI for issue operations.

### Quick Reference

```bash
bd ready                # Find available work
bd show <id>            # View issue details
bd update <id> --claim  # Claim work
bd close <id>           # Complete work
bd prime                # Refresh Beads context
```

### Rules

- Use `bd` for all task tracking; do not create markdown TODO lists.
- Run `bd prime` when Beads context is missing or stale. Codex 0.129.0+ can load Beads context automatically through native hooks; use `/hooks` to inspect or toggle them.
- Keep persistent project memory in Beads via `bd remember`; do not create ad hoc memory files.

**Architecture in one line:** issues live in a local Dolt DB; sync uses `refs/dolt/data` on your git remote; `.beads/issues.jsonl` is a passive export. See https://github.com/gastownhall/beads/blob/main/docs/SYNC_CONCEPTS.md for details and anti-patterns.
<!-- END BEADS CODEX SETUP -->

## Generation and Validation

`make gen` generates native game data, sprite/font sources, and packages the FX image with `cgfx-tools`; no Python bridge is required. `make gen-sprites` remains explicit for native sprite/font source regeneration through `cgfx-tools`. `make check` runs generation, host tests, and FX tests when Ardens is available.

`cgfx-tools` must be installed on `PATH`; `make doctor` verifies the selected executable. Do not restore the retired standalone data converters or a local tool resolver.
