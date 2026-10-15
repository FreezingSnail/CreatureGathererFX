# M1 collection checkpoint design

This contract supersedes the detached store/SaveFile designs in `lzc`,
`jp8.2.8`, and `jp8.2.9`, and it is the format the committed foundation now
uses: `src/save/Compaction.cpp`, `SaveFile.{hpp,cpp}`, `StoreRecord.hpp` and
`Journal.cpp` implement this v3 checkpoint on current main. The earlier
uncommitted over-budget prototype (`docs/m1-storage-candidate.md`, ~29k–30k B
flash) is historical evidence only and is not this implementation. The
remaining M1 children (jp8.2.4 → jp8.2.7) complete feature and endurance proofs
on this committed foundation; they do not redesign the format.

## Product scope

The initial collection holds **64 creature instances, including the three party
members**. Duplicate species with different experience and moves are supported.
Capacity is finite; a full collection refuses an add without overwriting anything.
Saving is explicit. No box UI or battle acquisition integration is required for
the first slice.

Slots 0 through 63 are stable across saves. Removing a creature writes a tombstone;
the allocator can reuse an unreferenced tombstone. Never remove or reuse a slot
still assigned to the party. There is no tombstone squeeze or party remapping.

## One coherent checkpoint

Use the existing `save_store_a` (0x2000) and `save_store_b` (0x3000) sectors as
alternating complete checkpoints. Each contains the small state snapshot and the
collection. `save_log` remains at 0x1000. `save_main` and all four reserved sectors
are unused by v3. Existing declarations and offsets remain unchanged.

All integers are explicitly serialized little-endian. Never persist runtime
`Creature`, enum, or compiler-dependent struct representations.

| Checkpoint offset | Bytes | Meaning |
| --- | ---: | --- |
| 0 | 2 | Magic bytes 0x43, 0x47 |
| 2 | 1 | Version 3 |
| 3 | 1 | Header size 16 |
| 4 | 2 | Generation, modulo 65536 |
| 6 | 1 | Capacity 64 |
| 7 | 1 | Snapshot size 96 |
| 8 | 1 | Live creature count, 0 through 64 |
| 9 | 1 | Cleanup pending: 1 initially, cleared to 0 after journal erase |
| 10 | 2 | Fletcher-16 over all 608 body bytes |
| 12 | 2 | Fletcher-16 over header bytes 0 through 11, normalizing byte 9 to 1 |
| 14 | 2 | Commit bytes 0xA5, 0x5A, written last |
| 16 | 96 | SaveFile v3 snapshot |
| 112 | 512 | Exactly 64 eight-byte StoreRecords |

The occupied extent is [0, 624). The sole record address helper is
`checkpointBase + 112 + slot * 8`, with `slot < 64` enforced before reads/writes.
Unoccupied records have id 0xFF; other bytes are canonical zero. StoreRecord stays
id(1), exp(2), moves(4), hpEncoded(1). Experience determines level. Byte 7 is
backward-compatible per-instance HP: zero means legacy/default full HP; values
1 through 255 encode actual HP as `value - 1`, including KO as 1. Authored data
must keep every supported maximum at or below 254 so the encoding is lossless.
Tombstones always encode byte 7 as zero.

The fixed 96-byte snapshot is:

| Snapshot offset | Bytes | Meaning |
| --- | ---: | --- |
| 0 | 2 | Player location |
| 2 | 8 | Flags; current flags followed by zero extension bytes |
| 10 | 3 | Party slot IDs; 0xFF means empty |
| 13 | 3 | Party HP |
| 16 | 8 | Plant stage bytes |
| 24 | 2 | Plant pair bytes |
| 26 | 1 | Plant ticker |
| 27 | 24 | Lure counts |
| 51 | 8 | Consumable counts |
| 59 | 4 | Key-item bitset; unused bits zero |
| 63 | 33 | Future state, zero initialized |

Snapshot bounds and counts are validated. An occupied party slot must refer to a
live creature, and party references must be distinct. There is no separate
SaveFile checksum or append envelope: checkpoint integrity covers the snapshot.
Header cleanup values other than 0 or 1 are invalid. The stored live count must
equal the number of non-tombstone records, and StoreAdd must reject id 0xFF.
v1/v2 saves are intentionally reset at v3. No legacy migration parser is shipped.

## Journal and live reads

Keep the existing 16-byte journal record format and op numeric values. `StoreAdd`
also replaces the value of an occupied slot for experience/move/HP updates. Its first
eight payload bytes are the StoreRecord, including byte-7 HP. On checkpoint publication,
current party HP from the existing 96-byte snapshot is mirrored into each matching party
StoreRecord; boxed records retain their own byte-7 value. During journal replay, nonzero
StoreRecord HP on a party slot supersedes an older snapshot value, while legacy zero leaves
snapshot/PartyAssign HP authoritative. A later PartyAssign takes precedence over an earlier
record update. `StoreRemove` addresses a slot.
`PartyAssign` uses record.slot as the creature slot, payload[0] as party index,
and payload[1] as the new occupant's HP. 0xFFFF is the unassign sentinel and
requires HP zero. Runtime and boot replay both require party index < 3, a live
slot < 64 (or the exact unassign sentinel), distinct references, and HP no greater
than that creature's derived maximum. The assignment API accepts explicit HP;
callers preserve that creature's current HP when moving it, or supply full HP
for a fresh instance. Never retain the previous occupant's HP implicitly.
Unused payload bytes are zero. The former pad
at byte 15 becomes commit marker 0xA5: write bytes 0 through 14, read back their
exact values, then write and verify marker 0xA5 last. Decode requires that marker
in addition to the checksum. XOR checksum alone does not prove publication; its
expected value can equal erased 0xFF. The stride and existing op values do not move.
Unknown operations are skipped; malformed known operations never address memory.

Use a 16-entry pending index of uint8 slot + uint8 journal index: 32 bytes, not
the former estimated 48-byte uint16-pair index (which would actually be 64 bytes).
Store slots are bounded at 64, journal indices at 256 records. Its tail uses
uint16 to represent full capacity 256. Repeated mutations of a slot replace its
index entry. Party assignments use the three RAM party IDs. Refuse a seventeenth
distinct pending slot before appending its mutation. Index/full-log refusal
prompts an explicit save; there is no autosave. A boot log that would overflow
the pending index fails closed; never overwrite the index or discard accepted
mutations silently. Update live state only after an
append validates, and gate appends during checkpoint/boot cleanup.

A slot read uses the pending record or checkpoint record. Reads occur on menu and
other state transitions. The first slice needs single-slot access only; later
window caching must fit the measured budget. At boot, scan the journal once after
cleanup, rebuild the index, and replay party assignments. A torn nonblank tail is
not overwritten; an explicit save can fold its preceding valid prefix and clear it.

## Commit and recovery

1. Freeze mutations and capture the 96-byte live snapshot. Use one 64-byte copy
   scratch; do not allocate a whole checkpoint or multiple snapshots in SRAM.
2. Erase the inactive checkpoint sector and verify it is blank before writing.
   The active checkpoint and journal remain intact.
3. Stream the snapshot and all 64 fixed slots, applying pending operations and
   counting live records. Bound work per advance; do not replay the entire journal
   once for every chunk. Accumulate the body checksum.
4. Write the header with generation + 1 and cleanup pending = 1. Write commit
   bytes last. Validate header, body, party references, and bounds on read-back.
5. A fully valid newer checkpoint is authoritative for both snapshot and creatures.
   Erase the journal, wait, and verify all 4096 bytes are blank.
6. Clear header byte 9 from 1 to 0 with a one-byte NOR write; read back before
   allowing gameplay or appends. The normalized header checksum remains valid.
7. Keep the previous checkpoint. Erase it only as the inactive destination of the
   next save, reducing cleanup phases and preserving a fallback.

Boot validates both checkpoints and selects the newer generation using signed
16-bit subtraction. Committed generations are adjacent, including 0xFFFF to 0.
If its pending bit is 1, the selected checkpoint already includes the journal:
ignore the journal, repeat erase/verification, and clear the bit before gameplay.
If its pending bit is 0, replay the valid post-checkpoint journal prefix.

| Interrupted phase | Recovery |
| --- | --- |
| Inactive erase or body/header write before validation | Prior valid checkpoint plus intact journal |
| New checkpoint valid, journal intact or partially erased | New checkpoint; ignore log and retry cleanup |
| Journal blank, pending-bit clear interrupted | Either pending and retry, or clean and proceed; no appends occurred yet |
| Clean checkpoint, later append torn | Replay valid prefix; refuse overwrite of dirty tail |
| Next save erases previous checkpoint | Current checkpoint plus its journal remain intact |

An entirely blank checkpoint area, or recognized incompatible pre-v3 save, starts
fresh. If the first-ever save is interrupted and neither header has its complete
commit marker, no snapshot committed: verify journal cleanup, then seed a fresh
game. A v3 header with complete commit markers but invalid integrity, and no valid
alternative, fails closed. Missing/invalid headers never import arbitrary bytes
as creatures. Fresh-game initialization seeds the three starting creatures into
slots 0 through 2 before the first checkpoint. Test cuts throughout that first
checkpoint and throughout journal body/read-back/marker publication.

Legacy reset also verifies journal cleanup before replay/appends; any readable
committed v3 checkpoint takes precedence over legacy data. The failure policy
covers power interruption and detectable integrity failure. Destruction of all
v3 magic/commit evidence by arbitrary bit corruption is indistinguishable from
an uncommitted first save; this format does not promise fail-closed recovery for
that case. No extra initialization metadata is introduced for bit-rot diagnosis.
Recovery of a corrupted latest committed checkpoint using its predecessor and
an untagged journal is also outside the guarantee: the journal may belong to the
lost newer checkpoint. Power-cut recovery assumes committed checkpoint bytes
remain intact while only the currently issued erase/program operation is torn.

## Measured first slice and wave order

First archive the abandoned storage diff and recover its committed baseline using
file/hunk-specific edits. Preserve the inventory changes, ArenaView edits, .codex,
and unrelated reports. Measure that combined baseline sequentially.

The first slice adds one duplicate instance through the store API, saves, simulates
power-cycle, reads it back, and reconstructs its level/party. Include ordinary,
last-slot 63, empty-slot, full-collection, and torn-checkpoint cases before expanding.
Run `make test`, `make testvm`, `make verify-generated`, the save/stack device spike,
and a fresh shipping build/RAM report. Compare whole-image deltas against the same
baseline. Hard limits: flash 29,696 B, static SRAM 2,160 B, painted stack at least
219 B. Planning target: flash at most 29,184 B to leave 512 B for later M1 layers.
Missing the planning target triggers a trim/scope review before expansion.

The committed v3 slice (jp8.2.11) already satisfies the first-slice proof. mgc.8 is the
current foundation batch gate: it runs now, after box.10–.14 + 2ua and the clean-generation
verification (every mgc.8 dependency is already closed), and it publishes the first-beta
budget — it is not gated by jp8.2.4–.7. The remaining M1 children complete feature and
endurance proofs on that foundation: finish checkpoint robustness (jp8.2.4), then
pending/window reads (jp8.2.5), then complete Player/SaveFile v3 integration (jp8.2.6), then
the exhaustive interruption/endurance suite (jp8.2.7). jp8.2.4 and jp8.2.7 then feed
jp8.11.16, the one-area beta release gate — that later feature/integration/release
checkpoint is where the post-integration endurance acceptance closes. These share save files
and run in later serial waves; independent inventory work (jp8.5.13) can remain separate. No
working-table prototype is adopted, and no resource figure from the retired draft is current.

## Acceptance audit (2026-10-14, jp8.11.1)

Read-only mapping of the current M1 criteria to committed source, permanent tests, and recorded runs.
`[R]` = recorded run in a report log; `[I]` = inspected source/test (not re-executed during the audit);
`[H]` = historical ledger entry in `output.md`. The audit ran no tests (shared AVR lane was active).

Committed revision at audit: `3ec1436`; root re-dispatched review at `9d2f6b7` (the 2ua ScriptVm
deletion commit). `git diff 3ec1436 9d2f6b7 -- src/save` is empty, so every save line/source ref below
is identical at both revisions; recorded report logs predate the current savings commits and are
dated evidence, not current re-runs. Source refs: `src/save/Compaction.cpp`,
`src/save/SaveFile.{hpp,cpp}`, `src/save/StoreRecord.hpp`, `src/save/Journal.{hpp,cpp}`,
`src/save/SaveController.cpp`. Host test refs use the post-box.13 line numbers.

| # | Criterion | Source | Test evidence | Status |
| --- | --- | --- | --- | --- |
| C1 | 64 stable slots, duplicate species, full refusal without overwrite | `storeAdd` Compaction.cpp:184, `storeRead`:168, StoreRecord.hpp | host `CheckpointCollectionTest`/`CollectionCapacityTest` save_test.hpp:26/55; device `test_saveCheckpointCollection`/`test_saveCollectionCapacity` save_test.hpp:183/228. `[R]` test_save 284/0 (reports/12/check.log:985) | COVERED |
| C2 | Party refs distinct+live; party-referenced removal refused; unassign sentinel HP0; HP ≤ derived max | `storeRemove`:194, `assignmentValid`:93, `replay`:100 | host :36-50 (refs/HP 36,38,43,45,46,49; `storeRemove` refusal :50); device :205-223 | COVERED |
| C3 | Exact instance XP persisted; no rounding to threshold when level unchanged | `SaveController::captureLiveSaveState` SaveController.cpp:44-47 | host save_test.hpp:39; device save_test.hpp:209 | PARTIAL — exp 125 is itself a threshold (`LEVEL_EXP[5]`); no non-threshold case |
| C4 | Party snapshot HP explicit per instance; previous occupant not implicitly retained; empty slot → 255/0. **Boxed-instance HP (`StoreRecord` byte7) is a separate, unimplemented feature** | party snapshot: Compaction.cpp:13-14,93-104, `SaveController::load`:138-149; boxed HP: StoreRecord.hpp:19-23 (`storeRecordEncode` currently writes byte7 = 0) | party snapshot: host save_test.hpp:38,43,46,49; device save_test.hpp:207,215-222 | PARTIAL — party snapshot HP covered; boxed HP is pending jp8.11.22 |
| C5 | Coherent commit-last checkpoint; newer-generation authority; no cross-generation mix | `saveStepAdvance`:214-266, `storeBoot`:137-167 | host `CheckpointCutTest`:69 — asserts only `loaded.playerLocation` (:77) and one `record.exp` (:78), **not** the full snapshot or all 64 records; device `test_saveCheckpointCuts`:252. `[R]` test_save 284/0 | PARTIAL — mechanism present; per-cut full-state comparison unproven (residual jp8.2.4) |
| C6 | 16 B journal record, marker 0xA5 written last after readback; decode needs marker+checksum; torn tail preserved | Journal.cpp:63-134; Compaction `append`:119 | host `JournalDecodeTest`/`JournalPublicationTest`:88/112; device `test_saveJournalLayout`/`test_saveJournalPublication`:153/292. `[R]` test_journal_boot 290/0 (reports/13/device-stack.log) | COVERED |
| C7 | Pending index 16 entries, replace-on-repeat, 17th distinct refused pre-append, boot overflow fails closed | Compaction.cpp:11,82,110,179,194 | host :112-128 (bounded write 119, 17th refusal 120-121, replace 122, overflow 123-128); device :305-325 | COVERED (behavior); bounded-read proof missing (jp8.2.5) |
| C8 | Power-cut recovery matrix across erase/body/header/commit/cleanup + torn tail | `saveStepAdvance`:214-266, `storeBoot`:143-166 | host prior-checkpoint matrix `{0,4096,4608,4718,4719,4720,4730,8816,8817}`:71; device prior-checkpoint matrix `{4718,4720,8816,8817}`:255 plus a **separate** first-save 4608 cut (:81 host / :276 device) | PARTIAL — see G1/G2/G7 |
| C9 | Blank / interrupted-first-save reset; recognized v1/v2 committed header resets fresh; invalid committed v3 fails closed; no arbitrary import | `storeBoot`:143-152, `storeFresh`:132 | first-save reset and invalid-committed-v3 fail-closed: host :81-85, device :276-289 | PARTIAL — the recognized v1/v2 reset branch (`storeBoot`:147-150) is **inspected only**; no test seeds a v1/v2 committed sector (G7) |
| C10 | Explicit save restores party/plants/flags; no autosave | `SaveController`:95-155 | host `ShippingSaveRestoreTest`:132 (`SaveController::begin/advance/load` round trip incl. plants) + `SaveFlagExtentTest`:152; device `test_saveCheckpointCollection`:183-226 round-trips via the Compaction API. (`test_saveStatusRender`:330 is status-render only, not persistence.) | PARTIAL — **no device `SaveController` shipping integration round trip**; inventory out of scope → jp8.5.13 |
| C11 | flash ≤29696, static ≤2160, painted ≥219, planning ≤29184 | — | `[R]` reports/12 final-gate: flash 24870 / static 1760 (2026-10-13); box.14 candidate 24844/1760; 2ua candidate 24844/1744; painted 484 (reports/13), 502 (2ua scripts) | COVERED (dated) |

Where "covered" means the committed mechanism is exercised by a permanent test that has a recorded
passing run on the current format; it does not mean every interruption state is re-run after every
later, non-save commit.

### Gaps (bounded, mapped to existing residual children — no duplicates)

- **G1 (jp8.2.4)** Generation adjacency is implemented (`static_cast<int16_t>(gb-ga)>0`,
  Compaction.cpp:142) but the `0xFFFF → 0` wrap is never forced by a test.
- **G2 (jp8.2.4)** Device prior-checkpoint cut coverage is narrower than host: host prior matrix
  `{0,4096,4608,4718,4719,4720,4730,8816,8817}` (host :71) vs device prior matrix
  `{4718,4720,8816,8817}` (device :255). The device 4608 (device :276) is a **first-save** cut and is
  **not** prior-checkpoint 4608 coverage. Missing device prior offsets: `0,4096,4608,4719,4730`.
  Prefer observable phase proofs (inactive erase, snapshot/body write, header, commit bytes, candidate
  read-back, journal erase, cleanup-bit) over mirrored raw byte offsets.
- **G3 (jp8.2.5)** No proof that one `storeRead` on a pending hit reads exactly one journal record and
  a checkpoint miss reads exactly one 8-byte record (backend read-count assertion), nor the
  transition-only read rule for that path.
- **G4 (jp8.2.6)** No non-threshold exact-XP round trip (e.g. exp 126 at level 5) and no explicit
  proof that a level change rewrites the record to the new threshold while an unchanged level leaves
  the stored bytes exact; also missing the duplicate-instance-with-fractional-XP and empty-party
  `255`/zero-HP mapping cases for `captureLiveSaveState`. Additionally, there is **no device
  `SaveController` shipping integration round trip**: device persistence is exercised only through the
  Compaction store API (`test_saveCheckpointCollection`:183-226), and `test_saveStatusRender`:330 is
  status-render only.
- **G5 (jp8.2.7, gated on jp8.11.22)** Full-journal (256-record) explicit save and "dirty tail then
  explicit save at capacity" are unproven; repeated save/rollover endurance is absent. The HP
  cut-matrix amendment (encoded nonzero boxed `recordHP` vs an older snapshot, legacy zero-byte
  partyHP, outgoing-HP mirror then PartyAssign, growth/evolution StoreRecord update, cuts during
  checkpoint HP publication) is **not implemented** — `storeRecordEncode` hardcodes byte7 = 0
  (StoreRecord.hpp:22). Boxed-instance HP is owned by the OPEN jp8.11.22; this leaf must not implement
  it, and its HP cases depend on that amendment landing. Party *snapshot* HP (`SaveFile.partyHP`) is
  implemented and tested (C4).
- **G6 (out of scope)** Inventory snapshot capture is not implemented
  (`SaveController::captureLiveSaveState` zeroes `lures`/`consumables`/`keyItems`); owned by jp8.5.13,
  not this epic.
- **G7 (jp8.2.4/.7)** No test seeds a recognized pre-v3 committed checkpoint (magic `0x43 0x47`,
  version 1/2, commit bytes) and asserts the fresh-reset path (`storeBoot`:147-150 → `storeFresh`).
  The only header-corruption test sets version = 4 on a valid v3 header and asserts fail-closed
  (host :84-85), which exercises a different branch.

### Prototype retirement

`docs/m1-storage-candidate.md` (working-sector explicit-save / disposable-overlay prototype) is
retained only as a **superseded, unadopted design direction**; current main has no working-table code
(the save path is the alternating coherent checkpoint in `Compaction.cpp`). This audit does **not**
claim every outcome the experiment targeted is proved: the durable-collection / party / HP / exact-XP
outcome comparison is jp8.2.14's own deliverable (see `proposed-bead-updates.json`). No resource
figure from the draft is current.
