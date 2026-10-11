# M1 collection checkpoint design

This contract supersedes the detached store/SaveFile designs in `lzc`,
`jp8.2.8`, and `jp8.2.9`. The existing uncommitted implementation is an
over-budget prototype, not the implementation of this contract.

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
id(1), exp(2), moves(4), reserved(1). Experience determines level. The reserved
record byte remains zero/default, not checkpoint metadata.

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
also replaces the value of an occupied slot for experience/move updates. Its first
eight payload bytes are the StoreRecord. `StoreRemove` addresses a slot.
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

After the slice fits: finish checkpoint robustness (jp8.2.4), then pending/window
reads (jp8.2.5), then complete Player/SaveFile v3 integration (jp8.2.6), then the
exhaustive interruption/endurance suite (jp8.2.7), then mgc.8 integrated gate.
These share save files and run in later serial waves. Independent inventory work
can remain separate. No completed prototype is promoted without the final gate.
