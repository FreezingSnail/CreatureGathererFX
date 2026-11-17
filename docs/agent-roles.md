# Project agent roles

Shared operating contract for the project-local OpenCode roles in `.opencode/agents/`. It
supplements `AGENTS.md`; repository rules, `docs/dev-flow.md`, and the selected content contracts
still govern. Role boundaries are task contracts, not filesystem isolation. Frontmatter permissions
here make the five specialists and the reviewer deny all subagent delegation, while the producer
allows exactly its six named targets; the reviewer also denies edit and shell. Other file, shell,
edit, and external-directory permissions come from the existing global configuration and are not
broadened or replaced here. A hosting or session overlay that exposes extra tools does not expand a
role's contract.

## Source of truth and configuration

- Beads (`bd`) is the only work queue. No role queues, markdown TODO lists, or parallel task lists.
- Root `opencode.json` selects `cg-producer` by default, sets the project model to
  `opencode-go/deepseek-v4.1-flash`, and lists this file in `instructions`.
- All seven profiles use `opencode-go/deepseek-v4.1-flash`. Preserve their modes, models, and
  delegation permissions. The producer may delegate only to the five named specialists plus
  `cg-reviewer`; each specialist and the reviewer deny all subagent calls. The reviewer also denies
  edits and shell. Preserve inherited plugins, providers, and permissions. Do not copy global
  configuration or credentials into this repository.

## Starting a role

Run from the CreatureGathererFX root:

```sh
cd /Users/<user>/code/creaturegather/CreatureGathererFX

# Default interactive primary role: cg-producer
opencode

# Direct non-interactive selection (v2.0.15 supports --agent on `run`)
opencode run --agent cg-world --model opencode-go/deepseek-v4.1-flash "bounded task prompt"
opencode run --agent cg-story --model opencode-go/deepseek-v4.1-flash "bounded task prompt"
opencode run --agent cg-art --model opencode-go/deepseek-v4.1-flash "bounded task prompt"
opencode run --agent cg-gameplay --model opencode-go/deepseek-v4.1-flash "bounded task prompt"
opencode run --agent cg-tooling --model opencode-go/deepseek-v4.1-flash "bounded task prompt"
```

The reviewer is dispatched by the producer as a fresh child with the review packet; that is the
supported route for its `subagent` mode. Do not run the review as a continuation of the
implementation session. No restart is needed for a newly added profile unless live discovery is
stale.

- The top-level `opencode` command has no `--agent` flag in v2.0.15; use `opencode run --agent`.
- Agent discovery is `opencode debug agents`. TUI `@cg-...` mentions are optional and depend on
  live discovery. Use `--agent` or the bridge when dispatch must be deterministic.
- No server restart is required for this setup. Do not restart or interrupt an existing service or
  another user's sessions. Run `opencode run --help` / `opencode debug --help` rather than assuming
  flags from older versions.

## Roles

| Role | Mode | Owns | Does not own |
| --- | --- | --- | --- |
| `cg-producer` | primary | Player-facing task definition, sequencing, ownership, bounded dispatch, acceptance, integration | Epic implementation, unlimited backlog sweeps, creative decisions without the owner |
| `cg-world` | all | Bounded areas/routes/landmarks, `GameMap`, blocks, WFC, TSX properties, encounter placement | Art redraws, firmware, or gameplay proof from WFC/decorative tiles |
| `cg-story` | all | Premise, canon, characters, motivations, beat/place/action tables, environmental storytelling, ending intent | Runtime implementation or unsupported implementation claims |
| `cg-art` | all | Terrain, creature, player, NPC, and UI source art in bounded batches | State selection, rendering, firmware, or unverified visual approval |
| `cg-gameplay` | all | Accepted firmware interactions, rendering, progression/persistence, menus, gameplay wiring | Redesigning accepted contracts or silently substituting another implementation model |
| `cg-tooling` | all | Bounded native Rust/Tauri authoring, compiler, and validator work in `../CreatureGathererTools` | Alternate editors/converters, unassigned sibling edits, or firmware changes |
| `cg-reviewer` | subagent | Independent read-only acceptance review of one bounded implementation slice | Editing, repairing, shell/delegation, queue changes, whole-project audits, or self-review |

## Phases

**PLANNING** produces proposals and artifact specs. Specialists use supplied bead excerpts and
facts, inspecting only relevant unknowns; they do not run board sweeps or repeat issue histories.
The producer owns relevant current queue/dependency checks. Planning does not update implementation
contracts. Instructions already in ambient context count as read; still read missing applicable instructions or an assigned
contract before edits. A directly started worker with no packet reads its assigned bead, not the
whole queue. Read-only advice may live under the producer's planning bead; no child bookkeeping
bead is required.

**IMPLEMENTATION** follows the assigned bead: exact acceptance, native checks, build lane, actual
states, and repository rules. Planning budgets never cap implementation, testing, or required
verification. A worker reports ready-for-review and never closes its own bead.

**REVIEW** is a fresh, bounded, source-read-only pass by `cg-reviewer` over one implementation
revision. The packet supplies acceptance, the relevant diff or an exact patch/snapshot, commands
and results with log paths, artifact/profile identity, resource deltas, and known limitations.
Narrow reads verify claims; the reviewer does not scan the queue or widen into a project audit.
Verdicts are `REVIEW: APPROVED`, `REVIEW: CHANGES_REQUIRED`, or `REVIEW: INCOMPLETE`, each with
concrete `file:line` or artifact references and bounded actions. Missing required proof is
`INCOMPLETE`; unrelated or invented checks are not required. Repairs stay inside the accepted scope
and a focused re-review covers the relevant revisions. The reviewer cannot review its own
implementation, and approval does not replace producer acceptance, the integration gate, or an owner
decision. For journey-level slices, schedule the first review after the first integrated end-to-end
path (including acquisition/economy), before widening UI/presentation polish; a final whole-wave
review is not a substitute for that early acceptance-path review.

Proposals stay labeled **PROPOSED** until adopted by owner direction or existing delegated
authority. Routine choices already authorized proceed normally. Do not silently change existing
implementation acceptance, design, or dependencies, introduce blockers, or call unreviewed choices
"frozen". Genuine new owner approvals use the repository gate workflow; do not invent approval gates
for routine choices. Preserve existing WIP, `human` labels, assignees, and unresolved gates.

## Dispatch

The producer starts with the next player-facing decision or output. Gather relevant current
Beads/source context once, reuse prior evidence, and verify only facts that affect that output. No
default all-specialist dispatch and no renewed project-wide audit.

Send one small packet: phase; one concrete deliverable plus acceptance; known facts and provenance;
exact relevant inputs and the specific unknown; owned paths; budget/checks. An implementation packet
also includes the existing bead ID, exact verification, and build lane/report location.

Acceptance is fixed before dispatch, not discovered after: name the actual player/gameplay outcome
and path, accepted bead requirements, owned source/diff scope, required evidence, and exact checks.
Name any map, data, RNG, or input substitution and the proof limit it creates. Resource-changing
work names the changed call paths that need painted-stack coverage and requires fresh whole-image
control/candidate numbers with profile identity; an unchanged standalone `test_stack` number does
not prove an unexercised menu or swap path. Deterministic tests are legitimate within their scope;
do not require production data for every unit test. One real-data integration checkpoint at the
relevant batch/slice boundary supplies assembled-path evidence. Do not retroactively reopen old
Beads or turn game test gaps into a workflow task's requirements.

Implementation flow is bounded task -> implementation plus focused checks -> fresh reviewer ->
worker repairs -> focused re-review -> producer closure -> batch integration gate.

Planning usually sequences story -> world -> art -> narrow gameplay feasibility, using only needed
roles. Independent outputs may run in parallel. Tooling is dispatched only for a demonstrated
blocker. Do not force this sequence onto unrelated tasks. Distinguish actual dependency edges from
chosen execution order, and use full bead IDs in handoffs.

A useful first-area output is one recommended opening-to-ending experience, its places and player
actions, required changes, and at most three unresolved creative decisions.
Check action prerequisites and supplied gameplay facts before accepting a plan. Separate creative
framing from current mechanics; added mechanics are PROPOSED and must name their dependencies.

## Planning checkpoints and reports

Default planning checkpoint: deliver a draft after at most 10 investigative tool calls or 5
measured minutes, whichever comes first. If the budget is reached, return the partial artifact plus
the exact unknown and a recommendation; do not churn, silently extend, or start another audit.
Required instruction reads count within reconnaissance; do not skip governing rules to meet the
budget. The producer may explicitly expand a focused investigation when justified. These are soft
prompt behaviors, not enforced timers.

Review is its own focused investigation checkpoint, not subject to this small planning budget. It
still returns the verdict and findings concisely; expansion should serve the review question, not
become a second audit or implementation.

Return the artifact first, then concise evidence, uncertainty, and next handoff. Planning reports
default to <=500 words excluding an explicitly requested artifact; producer synthesis defaults to
<=400 words plus the requested artifact. Keep verification findings separate from creative
proposals. No multi-thousand-word inventories unless requested. Give progress updates at useful
milestones and at least once per 60 seconds of active work; after a background dispatch, return the
initial handoff promptly instead of waiting silently or polling. Report wall time and known active
work separately; exclude laptop suspension or idle gaps from active-time claims and otherwise mark
active time `unmeasured`. Derive elapsed time from the clock or session metadata; if unavailable,
report `unmeasured`, never an estimate of time spent. Estimates for resources are labeled as
estimates.

For implementation, report changed/artifact paths first, then exact commands and results, the
data/path/device/profile exercised, missing coverage or substitutions, whole-image deltas and stack
coverage where applicable, and elapsed time. Each new permanent test names the standard Make,
native, or CI target that runs it; if no standard target legitimately applies, record why and the
exact alternative instead of adding a bespoke harness. A one-off green command alone does not prove
gate inclusion. Shipping static RAM, test-sketch stack, hardware observation, automated tests, and
creative approval are separate evidence classes. Specialists mark work ready-for-review and leave
closure coordination to the producer. Avoid concurrent `output.md` edits: serialize them or return
report text for the producer to append.

## Bridge dispatch

The Gangway bridge exposes `mcp__gangway__opencode_start_session`. Pass role, project directory,
and model explicitly, plus a stable `requestID` and a bounded prompt in the packet shape above:

```json
{
  "directory": "/Users/<user>/code/creaturegather/CreatureGathererFX",
  "agent": "cg-world",
  "model": "opencode-go/deepseek-v4.1-flash",
  "requestID": "cgfx-<bead>-cg-world-<short-purpose>",
  "prompt": "<phase>; deliverable + acceptance; known facts/provenance; inputs + specific unknown; owned paths; budget/checks; phase rules"
}
```

Prefer one dispatch with a stable `requestID`, then bounded wait/status/result calls. A completed
turn is not acceptance; check the bead contract before closure. A review dispatch uses the same
shape with `agent: cg-reviewer` and the acceptance/diff/evidence packet; it starts a fresh child
context, not a continuation of the implementation session.

Producer closure requires independent acceptance plus an `APPROVED` review. `CHANGES_REQUIRED`
blocks closure pending repair and focused re-review; `INCOMPLETE` means required evidence is
missing. Closure claims stay within verified scope, and known nonrequired limitations need not
block it.

## Boundaries and current facts

- `cg-world` authors maps; `cg-story` does not implement content/runtime; `cg-art` does not select
  state or implement rendering; TSX/compiled bits own semantics; native Tools are reused.
- The shipping compact VM supports only `If`, `TpIf`, and `End`; onboarding is affordance-only.
  Encounter AI persona is not narrative personality.
- Selected manifest `cgfx-project.json` points to `map_json: maps/world_map.json`,
  `tileset_png: images/tiles.png`, `map_tileset_png: images/tiles-editor.png`, and
  `npc_tileset_png: images/npc_16x16.png`.
- `blocks.json` currently contains only one `rock wall` block. Do not assume houses, trees, or other
  prefabs exist.
- OpenCode tools, Tauri editor commands, and script-host capabilities are distinct from firmware.
  A host-side inspection or catalog feature does not establish an installed device capability, so do
  not turn dated prose or source-only features into verified installed capability.
- No mandatory manual visual checks beyond user or repository requirements.
