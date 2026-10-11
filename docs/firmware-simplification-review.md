# Remaining firmware simplification review

Epic: `CreatureGathererFX-62u`. Review date: 2026-10-10.

The owner ranks correct requested outcomes first and firmware size second.
`AGENTS.md` now states that reusability, abstractions, general code-quality and
generic safety conventions have no independent priority. Trust proven internal
and authored-data invariants; enforce content constraints in host tools. Keep
the behavior of actual gameplay and hardware states.

Three Sol reviewers examined current source and the available 06g shipping ELF.
No runtime code, tests, generated assets or builds changed during this review.
The measured checkpoint is 23,488 B flash / 1,747 B static SRAM, 6,208 B flash
free, and 509 B painted stack headroom. Some mutable save source timestamps are
newer than the inspected ELF; these are checkpoint figures, not a fresh build of
every current byte. No proposed saving below has been measured. Named symbol
sizes identify work to investigate and cannot be added as projected recovery.

## Outcome defect: fix first

`GameState::flags` points at generated `FLAG_BIT_ARRAY[1]`, but both capture and
restore in `SaveController.cpp` copy `sizeof(gameState.flags)`: two bytes on AVR,
eight on the host. Capture reads beyond the array; restore writes beyond it.
In the inspected ELF the adjacent byte is framebuffer byte zero. A nonzero
neighbor can become a forbidden reserved snapshot byte and reject a valid save.

Priority-zero bead `CreatureGathererFX-8fu` specifies the correction: use the
generated array extent for both directions, compile-time assert that it fits the
eight-byte save field, preserve canonical zero padding, and test a nonzero
neighbor/framebuffer through save and load. This is a mechanics defect, not a
request for defensive-programming layers. It remains deferred for implementation
handoff; the review did not edit save code.

## Recommended measurement order

| Candidate | Why it deserves a spike | Outcome contract |
| --- | --- | --- |
| 16-bit damage stage arithmetic | All live predivide products fit: maximum 47,430. The current ELF links 32-bit helpers used by this path. | Exact damage/stage division, saturation, immunity, AI decisions and RNG unchanged. |
| Omit unread shipping result facts | The new feedback never reads the four consequence entries, yet effects still build and reset them. | Apply every status, stage, HP and duration effect; preserve KO flags and feedback sequence. |
| Remove the unused flash page-write member | No caller uses it, but the backend table keeps its thunk linked. | Actual byte-write, erase, busy and interrupted-write behavior unchanged. |
| Remove redundant attack captures | The private attack function captures post-state, then its public action wrapper captures it again. | Identical action result, faint flags and RNG trace. |
| Remove unused negative raster mode | No shipping/test caller selects it, but its branch is in the live pixel loop. | Identical overwrite/masked sprite output, clipping and edge CRCs. |
| One journal scan at boot | Setup counts the journal before boot replay scans it again; the M1 contract requires one scan after cleanup. | Replay, tail ownership, torn-tail refusal, cleanup retry and full-log handling unchanged. |
| One-pass checkpoint validation | A checksum pass is followed by rereading the snapshot and every record. | Identical accepted/rejected wire images, generation/commit/cleanup behavior and scratch budget. |
| Reuse AI damage calculation | The scan computes selected damage, then scoring recomputes it; DEFUP candidates may rescan the opponent. | Same tactical choice, tie order, immunity/zero-damage fallback and usable-move handling. |
| Default-profile dialog omission | The compact profile has no message producer but retains an 85 B dialog object. | Full-world/dialog profiles retain dialogue; host profile checks reject unsupported content. |

The order starts with narrow, independently measurable changes. Save experiments
require a valid new-API device baseline; the current stale save fixture and
compaction assertion must not be weakened or treated as passes. Each proposal
needs its own source checkpoint, whole-image measurement and relevant mechanics
tests before combination. Prefer the smallest winning implementation over a
general-purpose abstraction; discard no-saving rewrites.

## Deeper and profile-dependent avenues

- After the first fact-write spike, compact the shipping ActionResult separately.
  Most snapshot fields are unread; two pre-action HP bytes are still needed for
  faint detection. Actor names can come from the active species at feedback entry.
  Keep legacy detailed results only where host/simulator callers need them, and
  test the actual compact shipping path rather than a different checked seam.
- Replace the 18 B SRAM stage-ratio table with bounded ratio arithmetic as a
  separate experiment after narrowing multiplication. SRAM removal can increase
  flash; neither result is promised.
- Carry a validated remaining-PP value into spend instead of recomputing it.
  Status gating does not change PP, but confusion self-hit must still spend none.
- Make the compact warp interpreter one pass only after the host checker covers
  flag indices, conditions, branch boundaries and destination coordinates.
  Current checks cover opcode/slot structure but do not prove all those values.
  Full VM capability remains available; authored future features cannot disappear
  silently when a compact profile is selected.
- Narrow the property cache to walkable, encounter and tile presence only in a
  profile whose callers require no other bits. Future water/plant/lure mechanics
  need an explicit capability check. Do not substitute per-step cart reads.
- Validate hand-authored encounter records on the host before removing duplicate
  runtime scans or replacing the eight-zone loop with an authored active count.
- Advance four two-bit plant stages per packed byte instead of 32 indexed updates;
  preserve modulo-four growth and committed-step timing exactly.
- Specialize AVR save serialization to its asserted 96 B packed layout, keeping
  the exact wire bytes/checksum and a portable host oracle. Do not omit required
  canonical zero initialization.
- Remove journalDecode's blank-record precheck: a blank record already fails its
  commit-marker test. The append-time blank check remains necessary to avoid
  overwriting a dirty/torn tail.

## Cross-domain traps

- ModeState is a union: world is 191 B and battle is 190 B in the checkpoint.
  Shrinking BattleSession alone does not reclaim global SRAM. Narrowing the
  property cache from 43 B to 20 B would make world 168 B; with battle still 190 B,
  that recovers at most one byte from this union at that checkpoint. A compact
  battle result may unlock the larger cache reduction later. Measure both
  independently, then combined; do not book struct reductions as static savings.
- The 85 B dialog object and 27 B VM object are linked checkpoint facts, not
  measured removal deltas. Existing bead `2ua` already owns the older 16 B VM
  array trim; do not count or create it again.
- Legacy presenter, old view/snapshot APIs, generic inventory/menu helpers and
  many draw helpers are already unlinked in the default image. Deleting their
  source cannot be claimed as current firmware recovery. Future world feature
  costs still need planning even when dormant code currently costs zero.
- Current power-loss recovery, sentinels, no-PP/KO/party exhaustion, fresh input
  ownership and FX metadata transition rules are outcomes. Repeated validation
  of internally proven values is different and may be removed. Unknown authored
  effects and encounter fields are not all rejected by existing generators:
  close those host-contract gaps before relying on them.

## Detailed evidence and handoff contracts

- [Battle](reviews/62u-battle-review.md): five ranked experiments, exact live
  callers, arithmetic proofs, result consumers, AI/PP risks and test targets.
- [World/VM](reviews/62u-world-vm-review.md): shipping/full-profile reachability,
  generator gaps, collision/encounter/cache contracts and future feature costs.
- [Save/rendering](reviews/62u-storage-render-review.md): flag-copy defect,
  journal/checkpoint wire semantics, backend reachability and raster contracts.

Deferred implementation epic `CreatureGathererFX-1as` contains 17 bounded spike
contracts matching these findings. Its save experiments depend on `8fu`, and
deeper result compaction depends on the fact-write and duplicate-capture spikes.
Existing `2ua`, `jp8.2.7` and `1x8` remain separate work; no duplicate was created.

All proposed implementation tasks are deferred. This review authorizes neither
runtime implementation nor adoption. Future spikes should report baseline,
independent and combined flash/static bytes, painted reserve, exact flags and
commands, test counts and wall time. Preserve real-AVR coverage of trusted code.
