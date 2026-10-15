# Agent Instructions

Creature collecting demake for the **Arduboy FX** (ATmega32u4), rendered in 1bpp with native
`Arduboy2Base` at 52 fps. Each frame updates and renders once, then calls
`FX::display(CLEAR_BUFFER)`. Read `README.md` for architecture/status and `docs/dev-flow.md` for
the workflow conventions distilled from past waves.

This project uses **bd** (beads) for issue tracking. Run `bd prime` for full workflow context.

## Engineering priorities

1. **Correct requested outcomes and game mechanics.** Preserve the behavior the owner wants,
   including real gameplay states, timing, persistence and hardware operation.
2. **Small firmware.** Choose the implementation with the lowest measured whole-image program
   flash cost that achieves those outcomes within SRAM, stack and frame-time limits.

Reusability, abstraction, architecture cleanliness, general code-quality conventions and generic
safety practices have no independent priority. Prefer direct, specialized code when it works and
is smaller. Do not add layers, general-purpose APIs, fallback paths, validation or guards merely
because they are considered good programming practice. Keep a refactor only if it produces a
measured resource win or is needed for a requested outcome.

Trust controlled authored data and internally produced values where their invariants are known.
Enforce content constraints in host authoring/generation/build tools instead of spending AVR
program flash on malformed-data defenses. Check the generator's actual guarantees before relying
on them; fill missing authoring checks there. Impossible internal API misuse is not a gameplay
requirement, and tests that invent such misuse must not force defensive code into shipping builds.

Empty entries, no usable moves, knocked-out creatures, party exhaustion, fresh input edges, valid
sentinels and interrupted save writes are real states with required outcomes. Preserve their
behavior with the smallest necessary logic; do not confuse it with generic defensive programming.
Tests should prove mechanics and hardware outcomes, not enforce style or unnecessary abstractions.
Diagnostic symbol sizes are leads; whole-image AVR measurements decide which changes save bytes.

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
make clean-build-artifacts # preview generated Arduino-cache cleanup; CLEAN_APPLY=1 to apply
make test-clean-build-artifacts # verify cleanup bounds and preserved evidence
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

- **Visual verification**: Manual visual checks and Ardens observations are not required for acceptance unless the user explicitly requests them. Use automated tests and device-suite results as verification.
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
- **Bound build storage**: follow the build-storage rules below. A separate bead, variant or worker
  is not a reason to create another complete build tree. Clean disposable outputs after each
  measured item; keep only the evidence needed to reproduce and assess it.
- **Spike snapshots**: never recursively copy the dirty repository into a child directory. Make one
  control source snapshot, then save each experiment as its exact tracked-source patch, allowlisted
  untracked source/generator prerequisites, report, and chosen ELF. Exclude `build/`,
  `.battle-carve-spike/`, all scratch/spike work directories, `.git/`, `.beads/`, `.codex/`, and
  caches from filesystem snapshots. Prefer `git archive` for the committed base, then apply only
  the reviewed dirty patch and required allowlisted inputs. Inspect existing copies and report their
  size; keep unknown source/evidence until the owner decides to remove it.
- **Single FX image**: `dist/fxdata.bin` is the only flashable cart image; `src/fxdata.h` mirrors it.
- **Budget both program flash and SRAM.** The measured player-choice checkpoint (2026-10-10) uses
  23,488/29,696 B program flash and 1,747/2,560 B static SRAM, with 509 B painted stack headroom.
  These are a dated checkpoint, not guaranteed future headroom. Keep the existing ~150 B effective
  reserve and the budget rules in `docs/dev-flow.md`; static free bytes alone do not prove stack fit.
- **FX/OLED share SPI**: metadata/table reads happen only on transitions, never per-step or
  per-frame. Existing sprite/text raster transfers during rendering are expected. The metadata
  read counter (jp8.1.7) keeps the transition rule enforceable.
- **24-bit FX table reads**: ArduboyFX 1.4.0's AVR `readIndexedUInt24` path returns an incorrect
  top byte. Route packed address-table reads through `FxRead::indexed24` in `src/lib/FxRead.hpp`;
  it reads the three big-endian bytes and reconstructs `uint24_t`. Keep the device table test,
  including entries above `0x010000`, as evidence for this workaround.

## Build storage and cleanup

The 2026-10-11 disk incident left roughly 37 GiB of disposable pack stages. Source isolation must
not duplicate build caches, metadata or old experiments.

- **One build lane.** Only one worker or orchestrator may run AVR builds, generation, packing or
  integrated gates at a time. Assign the lane explicitly before dispatch; other workers may prepare
  non-overlapping source changes and small host checks. Workers must finish and release the lane
  before the next worker starts.
- **Reuse directories.** Pin one working `BUILD_DIR` and one integrated-gate directory per session;
  reuse existing suitable directories. Do not create per-bead or per-variant compiler/cache trees.
  Save the control ELF and measurements before overwriting the working outputs with the candidate.
  Keep per-bead logs under the shared report directory. For isolated sources, use
  `make -C /absolute/source BUILD_DIR=/absolute/shared-build ...` and confirm build provenance;
  invalidate incompatible outputs when switching sources or flags.
- **Check space before dispatch and after each item.** Run `df -h . /private/tmp` and `du -sh` on
  the shared build and owned scratch roots. Default storage budgets are 5 GiB for repository build
  outputs and 5 GiB for session scratch, including retained evidence. Keep at least 10 GiB free.
  If either budget is exceeded, free space falls below the reserve, or one item adds an unexplained
  1 GiB, stop new builds and clean or identify the cause. Record a justified larger budget in the
  bead before using it; do not silently multiply build trees or move the same sprawl to `/tmp`.
- **Clean each measured item.** The lane owner previews `make clean-build-artifacts` for each
  actual build root, then applies `make clean-build-artifacts CLEAN_APPLY=1` after preserving its
  evidence. Pass the relative `BUILD_DIR=build/...` explicitly for nested roots. This helper only
  recognizes certain generated Arduino/cache paths: an empty preview does not prove the tree is
  clean. Inspect remaining sizes and remove other positively identified disposable staging/cache
  directories separately. Retain the active shared demo/compiler cache while it remains within the
  storage budget and compatible with the next build; clean obsolete or incompatible experiment
  caches rather than deleting the active reused cache after every item. Never clean another
  worker's active directory.
- **Retain compact evidence.** Keep exact patches, source revision/allowlisted prerequisites,
  reports, measurements, relevant save images and selected control/final ELFs. Use one committed
  control snapshot plus patches; retire completed variant source copies once that evidence suffices.
  Do not retain complete compiler outputs, pack stages or duplicate repository snapshots per trial.
  Unknown sources or unique evidence stay untouched and are reported.
- **Temporary stages must self-clean.** Packing/staging commands must remove their own temporary
  directory on success, failure and catchable interruption. Exclude metadata, build/dist outputs,
  caches and scratch/spike roots from copied inputs. Do not remove the `pack` cleanup trap or broaden
  its copy to the entire repository. After generation, verify no `cgfx-pack.*` stage remains; remove
  positively identified abandoned stages before the next build. Keep final images and logs outside
  temporary stages.
- **Handoff includes storage.** Report free disk space, shared build paths, retained evidence and
  cleanup performed. The orchestrator checks these before assigning another build wave.

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
- **Parallelize non-overlapping source work.** Docs/tooling beads can run alongside code beads;
  same-file edits serialize, and AVR/generation/gate work always uses the single build lane above.
  Split oversized beads before dispatch (2–3 logical layers each).
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
- After implementation and verification, close the completed bead and commit the finished changes. Do not push unless explicitly requested.
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

## Approval Gates and Dependencies

Use the documented [Beads gates workflow](https://github.com/gastownhall/beads/blob/main/docs/workflows/gates.md)
for work awaiting an owner's decision. Do not use `deferred`, `--defer`, or a far-future date
to hold work for approval, hide it from the ready queue, or represent a dependency.
Deferral means deliberate postponement; use it only when the owner explicitly requests postponement.

- **Awaiting approval:** create a `human` gate and attach every issue covered by that decision.
  Record the concrete plan/result and the approval required in the gate's reason.
- **Awaiting other work:** add a blocking dependency with `bd dep add <work> <prerequisite>`.
  Parent-child relationships describe hierarchy; they do not replace sequencing dependencies.
- Keep waiting tasks `open` with their blocking edges; inspect `bd blocked` and `bd gate show`.
  Do not claim or implement work while its approval gate is unresolved, even if a queue view suggests it.
- Resolve a human gate only after the owner explicitly approves its stated condition. A correction,
  status question, elapsed time, or approval of an earlier phase does not satisfy a later gate.
- A gate with `--type human` represents a decision. It is distinct from the `human` issue label
  described below, which reserves implementation authorship for the owner.

```bash
bd gate create --type human --blocks <work> --reason 'Owner approval of <plan/result>'
bd gate add-waiter <gate> <other-work>  # register another waiter
bd dep add <other-work> <gate>         # ensure a blocking edge exists
bd dep add <later-work> <earlier-work>
bd gate show <gate>
bd blocked
# Only after explicit approval of this gate's condition:
bd gate resolve <gate> --reason 'Owner approved <plan/result> in this session'
```

When correcting an approval hold that was mistakenly deferred, create its gate/dependencies first,
then run `bd undefer <work>` and `bd update <work> --defer ''` to clear both status and any scheduled
date. Verify the task remains blocked and is absent from `bd ready`; update its current notes and
planning documents to match. Do not resolve the approval gate as part of that correction.
Check actual dependencies: in the installed CLI, `gate add-waiter` records waiters without adding
their blocking edges. Use `bd dep add` for those edges explicitly.

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

- **Conservative (default)**: Use `bd` for task tracking. After finishing and closing a bead, commit the completed changes. Do not push or run Dolt remote sync unless explicitly asked. At handoff, report changed files, validation, and suggested next commands.
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
- Commit completed changes after closing their bead. Do not push without clear authority from the active profile or the current user request.
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
