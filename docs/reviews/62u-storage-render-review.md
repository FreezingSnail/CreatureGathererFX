# 62u.4: save, rendering, and shared FX code-size audit

Read-only review on 2026-10-10. The priority is unchanged gameplay, save recovery,
and hardware behavior followed by **measured whole-image AVR flash size**. None of
the changes below has been built or assigned a byte saving. The 06g choice
checkpoint reported 23,488 B flash / 1,747 B static SRAM, with 509 B painted
stack headroom. I inspected `build/06g-choice-shipping/CreatureGathererFX.ino.elf`
using AVR `nm -C -S --size-sort`; it is a reachability clue only. The ELF is
timestamped 19:40 while the shared working-tree `Compaction.cpp` is timestamped
23:18, so I cannot prove that ELF is an exact build of every current source byte.
LTO folds and duplicates code; symbol sizes below are diagnostic, never additive
or a forecast. No production source, tests, generated assets, or build outputs
changed in this review.

## Correctness finding (fix before size work)

`src/GameState.hpp` declares `GameState::flags` as `uint8_t *`, and
`src/GameState.cpp` points it at generated `FLAG_BIT_ARRAY`. The generated
`src/flags/flag_bit_array.hpp` currently declares exactly **one byte**;
`CreatureGathererTools/crates/core/src/builder.rs` computes that extent from
the number of authored flags. `src/save/SaveController.cpp:32` copies
`sizeof(gameState.flags)` bytes into `saveState.flags`; line 134 copies the same
number back to the pointed-to array. On AVR the pointer is two bytes, so capture
reads one byte beyond the flag array and load writes one byte beyond it. This is
an **out-of-bounds source read on save and destination write on load**, not an
overflow of the eight-byte `SaveFile::flags` member. The inspected 06g ELF places
`FLAG_BIT_ARRAY` at SRAM 0x8001a6 and `Arduboy2Base::sBuffer` immediately after
at 0x8001a7. Thus the extra byte in that build is framebuffer byte 0; the current
mutable checkout needs a fresh build to establish its exact neighbor. Snapshot
validation requires byte offset 3 (`saveState.flags[1]`) to be zero, so a nonzero
neighbor can make an otherwise valid save fail before erase. A valid snapshot's
zero then clears that neighbor on load. Host pointer width is larger, so host
coverage does not excuse the bug. The bounded correction is to use
`sizeof(FLAG_BIT_ARRAY)` in both copies, with a compile-time assertion that it
fits `SaveFile::flags`; preserve zero padding and generated format constraints.
Add a focused round-trip test that makes the neighboring byte nonzero and proves
save/load touch only the generated flag extent. This is a separate correctness
bead for the orchestrator, with priority above all flash experiments.

## Ranked experiments

1. **Scan the journal once during boot.** Production `setup()` in
   `CreatureGathererFX.ino` calls `journalInit()` before `SaveController::load()`.
   `journalInit()` calls `journalCount()`, which reads the log in 32-byte windows
   until the first invalid record. A valid `storeBoot()` then calls
   `journalReplay(replay)`, which reads the same windows and reconstructs the
   pending index and party assignments. `docs/m1-storage.md` explicitly requires
   one scan after cleanup. Remove the production setup call, and make the
   production append path rely on the tail set by `storeBoot()`'s replay or
   `storeFresh()`'s erase. Otherwise `journalAppend()`'s lazy
   `JOURNAL_UNINITIALIZED` branch still pulls `journalCount()` into the image.
   Keep host/test API behavior if standalone journal tests need it. Valid
   checkpoint, cleanup-pending checkpoint, and fresh/legacy reset each establish
   the tail before gameplay can append; corrupt committed saves fail closed.
   A torn nonblank tail must still stop replay and refuse overwrite until an
   explicit save clears it. The checkpoint ELF has a linked `journalCount()`
   scanner (0x72 diagnostic bytes). Verify that it disappears from the new
   shipping ELF and measure the whole image. Run host save tests, a corrected
   device save suite and `test_stack`; exercise valid log prefix, torn tail,
   cleanup retry, fresh seeding, and 256-record full tail. This experiment also
   removes one duplicate cold-boot scan (up to 4 KB for a full log). **Risk: medium**,
   because startup ordering is the proof, not generic validation policy.

2. **Validate the checkpoint body in one bounded stream.** In
   `src/save/Compaction.cpp::validate`, the checksum pass reads bytes 16..623
   in ten chunks (64 B first, 32 B last). It then rereads the 96 B snapshot in
   two calls and each of the 64 eight-byte store records separately for semantic
   checks: 76 FX reads total per candidate checkpoint. Read the first 64 B of
   snapshot, the last 32 B, then eight 64 B record blocks. For each block,
   update the existing two checksum accumulators **and** run the current
   `saveSnapshotByteValid`, `savePartySlotsValid`, `recordValid`, live-count, and
   party-reference checks before replacing `scratch`. Keep a local three-byte
   party-slot copy and the existing 64 B global scratch; retain the exact
   header normalization (`scratch[9]=1` for either cleanup-bit state), body
   checksum, generation choice, commit markers, and fail-closed corruption
   behavior. This reduces 76 reads to 10 per valid candidate without widening
   scratch or introducing a second snapshot. Flash benefit is plausible because
   the separate record-read loop and repeated scan disappear, but unmeasured.
   Test both checkpoint sectors, slots 0/63, empty referenced slot, duplicate
   party slots, live-count mismatch, bad reserved byte, checksum mismatch,
   interrupted header/body/commit/cleanup writes, and stack headroom.
   **Risk: medium**: reading and checking in the same order must preserve every
   accepted/rejected wire image.

3. **Remove the unused flash page-write backend and then measure direct AVR
   dispatch separately.** `FlashBackend` in `src/save/FlashBackend.hpp` carries
   a `writePage` function pointer plus a device thunk. `rg` finds no
   `flash.writePage` caller in `src/` or `tst/`, yet the 06g ELF retains
   `flash_backend_detail::writePage` (0x4c diagnostic bytes), evidently through
   the initialized backend table. Remove the field, device thunk, and fake
   initializer member as one tiny experiment; retain the actual `writeBytes`
   implementation and all erase/read/busy calls. In a *separate* experiment,
   consider direct device-only calls rather than an address-taken mutable
   function-pointer table, while keeping the host fake adapter. LTO may already
   fold most dispatch, so this second step earns its place only if whole-image
   measurement improves. The fake's interrupted-write limit and busy behavior
   remain essential. Run save power-cut tests, focused device save/stack, and
   check that no flash operation starts while the chip is busy. **Risk: low**
   for dead member removal, **medium** for dispatch specialization.

4. **Use the proven AVR snapshot layout for compaction copy.**
   `src/save/SaveFile.cpp::saveFileEncode` loops over 64 then 32 bytes and
   branches for offsets 0/1 to encode `playerLocation`. On AVR, the `SaveFile`
   fields are contiguous and `uint16_t` is little-endian; its fixed 96 B size
   is already asserted. For the AVR path, assert `offsetof(SaveFile, flags)==2`
   and the relevant final offset, then copy the selected byte slice into the
   existing 64 B scratch directly. Keep the portable explicit encoder for
   host tests if needed. Do **not** omit `SaveController::captureLiveSaveState`'s
   zero initialization: reserved snapshot bytes must be canonical, and its
   live battle HP sync is required. This targets only the byte-wise serializer,
   not the validated on-flash format. Compare every saved snapshot byte and
   checksum against the current encoder, then run interrupted-save replay and
   stack tests. Flash saving is unmeasured; if `memcpy`/LTO costs more, discard.
   **Risk: low to medium** because it relies on the fixed AVR ABI.

5. **Remove the unreachable negative raster mode.**
   `src/lib/Blit.cpp::draw` checks `mode == NEGATIVE` for every pixel. An
   all-source caller search finds only `OVERWRITE` and `PLUSMASK`; the FX blit
   fixture also uses only those two. Remove the `NEGATIVE` enum value and its
   per-pixel complement branch without changing clipping, fractional-height
   masks, or 1bpp framebuffer composition. `Blit::draw` is linked in the
   checkpoint ELF (0x1ae diagnostic bytes), so this is inside live code, unlike
   unused `draw.h` battle helpers. Compare the existing draw CRCs at all edges,
   text and masked sprites, and the battle feedback/device stack suite. No
   asset or visual-output change is intended. **Risk: low**; flash result is
   still unmeasured.

6. **Drop the redundant all-blank test from journal decode.**
   `src/save/Journal.cpp::journalDecode` tests `journalBlank(in)` before testing
   `in[15] != 0xa5`. An all-0xff record necessarily fails the marker test, so
   the first test cannot change the result; `journalAppend` still needs its
   separate `journalBlank(bytes)` to refuse writing a dirty/torn tail. Keep the
   checksum and marker-last publication protocol unchanged. This is a small
   local branch deletion, not a request to weaken torn-write validation. Check
   blank, dirty-uncommitted, valid committed, and bad-checksum records, plus
   save/stack. **Risk: low** and savings unmeasured.

## Checked exclusions and measurement contract

- `src/engine/draw.h` still contains old `printType`, `printMoveInfo`,
  `printPackedMoveInfo`, and legacy scene helpers, but source-call search and
  the 06g symbol list show no shipped call to most of them. Removing dead
  declarations or already unlinked branches cannot be booked as flash savings.
  Current compact battle art is drawn by `BattleScene.cpp`; preview/menu paths
  and clear-before-masked-sprite behavior must remain.
- The packed two-bit type table in `src/lib/Type.cpp` is already present in the
  dirty checkout; the 06g ELF contains its 21-byte table. It is not a new
  proposal. `FxRead::indexed24` is required for ArduboyFX 1.4.0's top-byte
  defect; metadata reads belong on transitions, while ordinary raster SPI
  streaming is expected. Name widths are local PROGMEM lookup data, and the
  string generator in `CreatureGathererTools/.../fxpack/font.rs` renders width
  from authored characters, including a leading blank cell. No unsupported
  claim is made that replacing those tables would save AVR flash.
- The checkpoint `test_save.ino` fixture is currently stale against the M1
  `SaveFile` format and blocked the recorded 06g full gate. That is a separate
  fixture bead, not grounds to weaken save validation. Each experiment needs a
  fresh isolated source checkpoint, one change at a time, `make ram` whole-image
  comparison, host save/renderer tests, focused device suite with `test_stack`,
  and the final gate only after integration and the fixture blocker are settled.
  Preserve the 150 B effective reserve; the prior save-chain measurement of
  230 B is closer to that limit than the general 509 B checkpoint.

Review wall time: approximately 4 minutes (source/design/ELF inspection and
report); no builds or tests were run.
