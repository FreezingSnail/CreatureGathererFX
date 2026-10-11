# Candidate: 64 external records with explicit-save checkpoints

Feasibility experiment for the owner-authorized flash redesign. This does not
supersede docs/m1-storage.md until the measured candidate fits and its recovery
and stack checks pass. Capacity remains 64 instances including the party.

The semantic change is explicit-save rollback: after reboot, ALL collection,
party, XP and other state comes from the last completed save. Unsaved changes
are lost together. Individual collection edits do not create durable saves.

Use unchanged checkpoint sectors store_a/store_b at 0x2000/0x3000. Use the
existing reserved_0/reserved_1 offsets 0x4000/0x5000 as disposable working tables.
No FX declaration reordering, no additional image bytes and no 512B RAM table.
The 8B StoreRecord, stable IDs0..63, canonical empty representation, exact XP,
96B snapshot and 16B header are unchanged except header byte9 becomes canonical
zero (journal cleanup state is retired). Keep checksum, body validation,
generation selection, commit-last publication and legacy-reset/corruption policy.

Boot validates coherent checkpoints and points the live table directly at the
selected checkpoint. It never reads working sectors or the old save_log.
Fresh boot has a virtual table of64 canonical empty entries until party seeding.
Party slot IDs and HP are RAM state restored from the checkpoint.

For an actual record mutation, validate ID/XP/moves/party constraints first.
If unchanged, return success without erasing. Copy the512B table from its current
source into a freshly erased alternate WORKING sector, applying the edit during
streaming. Retain one bounded64B scratch; verify erase and written bytes before
switching the RAM live-source pointer. Working tables keep records at offset112
so the sole existing base+112+slot*8 address helper is unchanged. They need no
persistent header/generation/commit because reboot ignores them. A mutation must
never erase either checkpoint sector. On failure retain the old RAM source and
fail closed for further edits until recovery; never publish a partial working table.

Freeze gameplay while erase/write is in progress. The first experiment can use
synchronous verified operations, which block the frame loop; do not render from
FX while busy. Describe the latency limitation explicitly. Acquisition/XP hooks
are not added. Each actual edit incurs one sector erase and table copy, so this
candidate trades cheaper code for wear/latency; unchanged writes must be elided.
Batch initial three-party seeding and save-time party updates before adopting if
separate updates would cause three erases per save/bootstrap. Do not reconstruct
stored XP from an expanded Creature's level and discard fractional progress.

Explicit save freezes/captures the96B snapshot, erases/verifies the inactive
checkpoint, streams the snapshot and current512B table, validates/checksums and
writes header commit bytes last. Retain the previous checkpoint. On successful
publication, ALWAYS rebind the live table source to the newly committed checkpoint
before another save or mutation can start. Otherwise a subsequent save could erase
a sector still serving as source. Delete journal append/replay/pending-index and
journal-cleanup boot/save machinery; party assignment is a RAM edit with existing
live/distinct reference and HP validation, included in the next snapshot.

For the first measurement, change only an isolated repository snapshot under
build/m1-explicit-save/CreatureGathererFX. Control must match the current post-
Animator image30958flash/1842static under standard shipping flags. Build the thin
candidate, then STOP and report before expanding tests or production integration.
If it fits, adoption still requires permanent host/device coverage for duplicate
add-save-reboot, slot63/full refusal, unchanged-write elision, working-copy cuts,
checkpoint body/header/commit cuts, repeated-save source rebinding, invalid party
IDs/HP, corrupted candidates, and measured stack headroom. An unsaved working edit
must disappear on reboot, while a completed checkpoint must survive subsequent
interrupted working edits. Hardware flash capacity is unchanged.

Planning flash <=29184; hard flash29696/static2160/painted stack>=219.
No measured saving is claimed in this design.

## Second measurement: disposable journal overlay

The working-sector prototype measured30004B flash/1807B static versus30958/1842
control:954B/35B saved, still308B over hard limit and820B over planning target.
It is not adopted. The second experiment uses the same explicit-save semantics
and64slot/checkpoint format but retains the existing log as a disposable overlay.

Keep checkpoint addresses0x2000/0x3000, log0x1000, 8B records, 16-entry32B index,
uint16 tail supporting256 journal records, and existing16B record wire format.
Only StoreAdd/replace numeric2 and StoreRemove numeric3 are logged. Byte15 isA5
publication, written ONLY after exact readback of bytes0..14. Decoder/checksum,
unknown-op rejection at append, dirty-tail refusal, and17th-distinct-slot refusal
before append remain. Slot IDs stayuint8 in the index; bounds before all reads.
The source is either the active checkpoint table or the indexed latest log record.
No full-table RAM; retain one64B save/erase-validation scratch.

Boot validates/selects coherent checkpoints and restores96B snapshot/party.
Regardless of checkpoint generation, erase AND verify the entire log before
allowing gameplay or appends. No boot journal replay or index rebuilding exists.
Erase failure fails closed. Interrupted boot cleanup retries on reboot; neither
checkpoint is changed. Fresh-state seeding can log ordinary records into an empty
overlay. Stable64slot IDs, duplicate species, exact XP, live/distinct party refs
and explicit HP validation remain. Party assignment/unassignment changes RAM only,
no PartyAssign log operation. All unsaved changes disappear together on reboot.

Save freezes the snapshot/table view, erases/verifies inactive checkpoint and
streams96B snapshot+512B records through the pending index, computes/checks body
and publishes commit bytes last. Header byte9 is canonical zero; no pending-
cleanup header write exists. After publication rebind active checkpoint and
clear overlay index/tail before any edit or save can resume. Erase/verify the
log before appends resume; no cross-generation replay is possible because boot
never replays it. If postcommit log cleanup fails, keep published checkpoint
and fail closed for appends. Never resolve stale index entries into a partially
erased log. Old checkpoint is retained until the next explicit save.

The first isolated build is a measurement only. Stop for budget review; later
acceptance must exercise body/marker cuts, dirty tails, index cap, interrupted
checkpoint, repeated save/rebind, cleanup failure/reboot, and painted stack.
Do not weaken a checksum/publication check for code savings.
