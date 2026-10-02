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
- Final pre-commit gate: `make final-gate ARDENS=/path/to/Ardens` runs `make check` and then the
  shipping RAM report, sequentially. Confirm generation left no unexpected tracked changes; also
  run `make test-pack-parity` when packed bytes can change.
- `final-gate` prints result counts and resource figures while saving complete output in
  `build/final-gate/check.log` and `ram.log` (under `BUILD_DIR`, overridable with
  `FINAL_GATE_LOG_DIR`). Failures print the last 80 log lines and stop the gate. The device stage
  always selects every suite, even when a focused `FXTEST_INOS` override was supplied.
- `make check` bundles gen + host + VM + manifest + generated-libs + verify-generated + fxtest
  (device stage skipped without `ARDENS`). It does not include pack-parity or doctor tests.

## Generated artifacts

- `make gen` is the only generation entry; it requires `cgfx-tools` on `PATH` (`make doctor`
  verifies the selected executable; `make setup` prints non-mutating guidance).
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

## Review checklist (generated data, save, render)

- Generated diff: `make gen` empty diff; manifest valid; `src/fxdata.h` matches the packed image;
  no literal indices in tests.
- Save diffs: sector-aligned offsets; `SAVE_VERSION` unchanged unless the bead says otherwise;
  journal window sizes remain multiples of `JOURNAL_RECORD_BYTES` (8 B); torn-write and compaction
  paths exercised.
- FX reads: transitions only; the device suite asserts zero reads on the touched path once jp8.1.7
  lands.
- Render: `L4_Triplane` shade/plane handling; erase frames clear every plane.
