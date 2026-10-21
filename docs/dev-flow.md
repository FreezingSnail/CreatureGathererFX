# Dev flow conventions (lessons from the xrm / eqi / mm2 / zz5 waves and the 2026-08-24 RAM audit)

Workflow rules distilled from past waves. They are about the repo, not any particular tool or
agent. `AGENTS.md` carries the short list.

## Inner loop vs full gate

- Host: `make test`; ScriptVM: `make testvm`.
- One device suite while iterating:
  `make fxtest-headless FXTEST_INOS=tst/fxdatatest/test_save.ino` (`FXTEST_INOS` is a `?=` override;
  the default is the whole wildcard).
- First device spike for work that can affect call depth or stack use:
  `make fxtest-spike FXTEST_SPIKE_INO=tst/fxdatatest/test_save.ino`; this runs the selected suite
  with `test_stack`. Set `ARDENS=/path/to/Ardens`; the target errors instead of silently skipping.
- Full device gate: `make fxtest-headless` compiles every `tst/fxdatatest/*.ino` suite and
  requires each to print an exact `P` or `F` marker. Unset `ARDENS` reports a skip; an Ardens
  without `captureserial` is BLOCKED, never a pass.
- Device compilation (`fxtest-build`) uses one persistent stage per flag profile
  (`build/fxtest/profiles/<profile>/suite`) with a stable `suite.ino` whose content is swapped per
  suite. Only the sketch TU recompiles and relinks per later suite (~1.4-2.3 s); the first suite
  per profile is a full ~10 s compile on a cold stage. Repeat runs reuse the stage's objects, so
  an unchanged `src/` is not recompiled. Run `make fxtest-build FXTEST_CLEAN=1` for a from-scratch
  audit build that wipes the profile stages. Per-suite hex/ELF still land in
  `build/fxtest/<name>/output/`, unchanged.
- Final pre-commit gate: `make final-gate ARDENS=/path/to/Ardens` runs `make check` and then the
  shipping RAM report, sequentially. Confirm generation left no unexpected tracked changes; also
  run `make test-pack-parity` when packed bytes can change.
- `final-gate` prints result counts and resource figures while saving complete output in
  `build/final-gate/check.log` and `ram.log` (under `BUILD_DIR`, overridable with
  `FINAL_GATE_LOG_DIR`). Failures print the last 80 log lines and stop the gate. The device stage
  always selects every suite, even when a focused `FXTEST_INOS` override was supplied.
- `make check` bundles gen + host + VM + manifest + generated-libs + verify-generated + fxtest
  (device stage skipped without `ARDENS`). It does not include pack-parity or doctor tests.

## AUDIO builds

- `AUDIO=0` is the default and the shipping image: no engine translation units, no theme table, no
  runtime state — the flash image is byte-identical to a build that never had audio. `AUDIO=1` adds
  the compact ATMlib2FX runtime and the packed theme streams; it is FX-only (`make mini AUDIO=1`
  fails intentionally). `AUDIO=1` is not part of `make check` or `final-gate`, so a bead touching
  audio must run both profiles explicitly.
- Focused verification:
  - `AUDIO=1 make ram` and `AUDIO=0 make ram` (link + flash/static delta).
  - `AUDIO=1 make fxtest-spike FXTEST_SPIKE_INO=tst/fxdatatest/test_readcounter.ino` — proves the
    service path, save pause/resume, state routing, stream health (`underruns`, `error`,
    `max_service_gap`) and painted stack headroom in one run.
  - `AUDIO=0` spike of the same suite as the control: the binary must stay identical and the suite
    must still pass.
  - `AUDIO=1 make battle-test` boots the trainer demo on the combat theme; `AUDIO=0` keeps it
    silent. The battle-test build now uses the shipping profile plus `CGFX_TRAINER_DEMO`.
- Theme data: `make test-music` validates the packed streams, `data/music/catalog.tsv` and the
  `src/audio/MusicCatalog.hpp` rows. Never hand-edit a theme `.bin`; follow `data/music/README.md`.
- Routing lives at the state edges (`ModeState::enterBattle`/`exitBattle`,
  `BattleFlow::finishBattle`); keep transitions off the per-frame path so FX reads only happen on
  changes. The theme table is PROGMEM-resident and read once per change.

## Generated artifacts

- `make gen` is the only generation entry; it requires `cgfx-tools` on `PATH` (`make doctor`
  verifies the selected executable; `make setup` prints non-mutating guidance). Arena and lure demos
  call `make gen-if-needed`, which reuses outputs only after the authored manifest, packed images,
  installed tool, generation recipe and generated-header mirrors match the last successful `gen`.
  Explicit `make gen`, `make check` and `make final-gate` still regenerate forcefully.
- Never hand-edit `fxdata/generated/`, `tst/fxdatatest/generated/`, `src/vm/opcodes.hpp`,
  `src/flags/*`, `src/fxdata.h`, or `fxdata/Sprites.txt` — change the JSON/CSV/PNG/TOML source and
  regenerate.
- Regen must produce an empty diff on tracked generated sets. `make verify-generated` checks the
  provenance manifest non-mutating; `make test-manifest` runs fixture-backed shell coverage.
- Stage generated sets together after a regen (`git add -A`). Hand copies once shipped a stale
  opcode table (missing `SMsg = 2`) and stale flag ids.
- Tests read generated symbolic constants, never literal record indices.
- `fxlayout.toml` is an ordered flash layout, not a build script: within-file declaration order is
  ABI; an `expand` entry owns name/namespace/align; unqualified symbolic initializers permanently
  resolve to the first matching declaration.

## Budget-first for flash/RAM work

- Current flash requirement is the physical application limit, 29,696 B. The
  former 24,000 B project ceiling was retired by the owner on 2026-10-03.
  Static RAM remains limited to 2,160 B; preserve the measured stack reserve.

- Measured 2026-08-24 on the production LTO ELF: globals 2018/2560 B (78%), 542 B left for
  stack+locals; the save path alone needs 804 B (≥262 B overflow, 331 B+ with the USB ISR).
  Flash 18314/29696 (61%). SRAM is the binding constraint; the FX image uses 609 KB of 16 MB.
- Audit targets: ~203 B safe shrink → globals 1815; drop saveState → 1688 (recommended stop);
  USB/opponent streaming are reserves, taken only when a feature needs the bytes.
- `make ram` builds the shipping FX image and reports flash bytes, static RAM, free SRAM, and the
  largest static symbols. Run it alone during a budget spike, or use `make final-gate` to pair it
  with the integrated checks after the implementation settles.
- Spike before any change that can flip flash/RAM; report whole-image deltas (LTO makes per-symbol
  arithmetic meaningless).
- 2026-10-02 `-mrelax` current-head spike (Arduino CLI 1.2.0, Arduboy homemade AVR core 1.4.0,
  AVR-GCC 7.3.0-atmel3.6.1-arduino7): isolated FX baseline was 17736 B flash / 1857 B static RAM;
  adding `-mrelax` to C++, C, and ELF linker extra flags measured 17412 B / 1856 B (-324 B flash,
  -1 B static RAM). Shipping FX, Mini, and device-test builds share these properties. Focused
  device-suite verification is recorded in `output.md`.
- One measured item per bead; record each number in the bead notes. Bundled shrinkings make savings
  unattributable and the audit's per-item numbers unverifiable.
- Plan waves against measured headroom minus a reserve; never land below ~150 B free.

## Evidence ledger

- `output.md` is the per-bead ledger: exact commands, tails, numbers, documented deviations, and
  wall time (worker / gate / orchestrator) so the next retro is data.
- Long builds should emit phase lines (suite start/finish) so progress is observable without
  polling.
- Worker reports carry exact numbers and tails; a build that cannot fit or a failing gate is
  reported BLOCKED with the deficit and options, never as a faked pass.

## Build storage cleanup

- After each completed spike or budget experiment, preview `make clean-build-artifacts`; apply the
  reviewed cleanup with `make clean-build-artifacts CLEAN_APPLY=1`.
- The helper only removes Arduino compiler caches and `.o`/`.d` files from the generated sketch
  copy. It retains reports, source copies, CSV/measurement evidence, save images, and ELFs/HEXes.
  Unknown paths and scratch clones remain for manual review; there is no unattended cleanup.
- Keep one selected control ELF and the final candidate ELF with their measurement reports. Remove
  duplicate generated build copies after their result is recorded.
- Verify the helper's path and evidence guards with `make test-clean-build-artifacts` when changing
  its scope.

## Experiment source snapshots

- Never recursively copy the dirty repository into a child directory for a spike or variant. One
  full repository copy can duplicate all prior builds and experiments, then multiply again per
  variant. Keep one control source snapshot for the wave; represent each experiment with its exact
  tracked-source patch, allowlisted untracked source and required generator inputs, measurement
  report, and chosen ELF.
- Prefer `git archive` for the committed base, then apply only the reviewed dirty tracked-source
  patch and copy explicitly required untracked source/generator prerequisites. A filesystem-based
  snapshot must exclude `build/`, `.battle-carve-spike/`, all scratch/spike work directories,
  `.git/`, `.beads/`, `.codex/`, and compiler or generator caches.
- Keep the chosen control ELF and final candidate ELF with their reports. Do not create a full source
  clone per experiment. When an existing recursive copy is found, measure and report it; preserve
  unknown source or evidence until the owner decides its disposition.

## Review checklist (generated data, save, render)

- Generated diff: `make gen` empty diff; manifest valid; `src/fxdata.h` matches the packed image;
  no literal indices in tests.
- Save diffs: sector-aligned offsets; `SAVE_VERSION` unchanged unless the bead says otherwise;
  journal window sizes remain multiples of `JOURNAL_RECORD_BYTES` (8 B); torn-write and compaction
  paths exercised.
- FX reads: transitions only; the device suite asserts zero reads on the touched path once jp8.1.7
  lands.
- Render: 1bpp only (`BLACK`/`WHITE`); `FRAME(x)` is the logical frame, with no FX reads between
  render and `FX::display(CLEAR_BUFFER)`.
