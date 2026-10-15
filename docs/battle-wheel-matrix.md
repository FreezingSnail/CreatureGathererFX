# Eight-type matchup web (matrix proposal)

Status: spike proposal for the simultaneous-pick duel, 2026-10-14. Source of truth is
`tools/battle-wheel.mjs` (`WHEEL`), solved by `tools/battle-matchup-solver.mjs` /
`tools/battle-wheel-solve.mjs`, generated into `mock/data.js`, playable in `mock/duel.html`.
Not adopted into firmware or the shipping damage chart.

The matrix renders as an x/y table (rows attack columns: ▲ win, ▼ lose, · neutral) on the launcher
and inside both duel windows, highlighting the setup creatures' row and column (`mock/matrix.js`).

## Shape: overlapping triangles, not a single ring

The web is a directed graph of 25 "beats" edges over the 8 types — 45% of ordered pairs are
decisive, 55% are deliberately neutral. It contains 16 three-cycles; the four seeded triangles give
it its readable skeleton:

```
fire  > plant > water > fire
earth > lightning > water > earth
fire  > wind  > earth > fire
plant > spirit > elder > plant
```

Completion edges (chosen by the solver, preference-ordered for causal readability):
`wind>spirit`, `plant>lightning`, `earth>spirit`, `spirit>water`, `lightning>wind`, `earth>plant`,
`wind>plant`, `spirit>lightning`, `fire>spirit`, `water>wind`, `lightning>fire`, `water>elder`,
`elder>earth`.

No immunities and no mutual edges: every cell is win, loss, or neutral. Neutral is a real state —
not every pair should resolve.

## Type-count spike (is 8 too many?)

`tools/battle-matrices-spike.mjs` folds the authored roster into 4 / 5 / 6 types, solves a triangle
web per count against the folded kits, and emits `mock/matrices.js`. Compare all four side by side at
`mock/matrices.html`; any duel window's **types** selector plays the chosen table (`8 · full web`,
`6`, `5`, `4 · cycle`), and the window's table follows the selection.

| types | edges (facts to learn) | triangles | decisive / neutral |
| --- | --- | --- | --- |
| 8 | 25 | 16 | 45% / 55% |
| 6 | 15 | 8 | 50% / 50% |
| 5 | 10 | 3 | 50% / 50% |
| 4 | 6 | 2 | 50% / 50% |

Folds are successive: lightning→fire and elder→spirit (6), then wind→water (5), then the 4-type
cycle (flame / bloom / tide / stone). Every table still satisfies the roster coverage demands with no
unsatisfied set pairs; the 8-type table is exactly the live web. Learnability proxy is the edge count —
8 types is 25 facts, 5 types is 10.


`tools/battle-matchup-solver.mjs` derives the x/y chart from the edge list and checks demands over
the actual roster move-sets (each creature's distinct move types):

- two distinct sets must have at least one decisive edge in either direction — a positive matchup
  always exists between the sets;
- identical sets need an internal edge (some pick beats another pick in the same set);
- identical singleton sets are mirrors a matrix cannot fix; the solver reports them as data issues
  rather than inventing self-loops.

`node tools/battle-wheel-solve.mjs` re-derives the web for the current roster: it seeds the four
triangles, greedily adds the edges that satisfy the most outstanding demands (preferring edges that
close new triangles, then causal preference, then random across restarts), and prints the edge list
to bake into `tools/battle-wheel.mjs`. The generator refuses to build `mock/data.js` if any demand
is unsatisfied, and the data test re-runs the proof over all 64 creatures.

Current result: 25 edges, 16 triangles, 0 unsatisfied demands over 465 unique set pairs —
329 have mutual counterplay, 136 are one-sided (only one side has a winning pick), 0 are dead.
Three mirror data issues remain: wind, water and plant are each carried by two singleton
creatures, so those mirror matchups have no positive pick either way until the creatures get
more moves. The generator refuses to build `mock/data.js` if any demand is unsatisfied, and the
data test re-runs the proof over all 64 creatures.

## Roster context for three-move kits

- Status/utility moves (type `status`/`none`) are excluded from every pick list in this spike.
- usable attack moves per creature: 1 ×4, 2 ×13, 3 ×15, 4 ×32. Seventeen creatures field fewer than
  three attack moves (bell and SquibbleSnail carry one each), so some footers show fewer cells.
- distinct move types per creature (all moves, including status): 1 type ×7, 2 ×37, 3 ×18, 4 ×2.
- move-type carriers: earth 23, plant 23, water 16, elder 16, spirit 15, wind 12, lightning 12, fire 10.
- every creature already has a STAB move (affinity ×1.5 in the power duel).

## Play rules as implemented in the mock

- Both sides pick one of their own creature's moves, hidden; 550 ms reveal.
- **Power spelling** (`mock/duel.html`): win = full hit scaled by the winning move's power
  (4 + 1.2 × power, ×1.5 affinity), loser chips 2 + 0.25 × own power; neutral or mirror = trade chip.
  A ✱ next to the power marks an affinity (STAB) pick. Kept for comparison; the owner's direction is
  no-power.
- **Pure-RPS spelling** (`mock/duel-pure.html`): power, stats and affinity are ignored; the relation
  alone scores — win 14, chip 4, trade 6 both.
- **Creature-bonus spelling** (`mock/duel-bonus.html`) — recommended direction: moves carry no power
  (flat 14 / 4 / 6), and the creature-type matchup scales the damage each side deals — ×1.5 when the
  move's type beats the opposing creature's type, ×0.5 when it loses to it, ×1 otherwise (applies to
  full hits and chips, mirrored for the foe; no immunity, no affinity). ↑ / ↓ mark boosting and weak
  picks; result lines read BONUS WIN/LOSE (×1.5), WEAK WIN/LOSE (×0.5) or RPS WIN/LOSE (base).
- Type badges (white-framed chips) sit on each creature sprite in every window, so both creatures'
  types are visible while choosing.
- All windows share the web, option source and seed, so they can run side by side.
- The AI picks at the same moment you commit and only sees public information: your badges,
  move types and revealed history — never your current pick.
- **Personas** (reused across groups of trainers; selectable in every duel window):
  `random`, `reader` (counters your last revealed type), `frequency` (your most-used),
  `anti-repeat` (expects you to rotate — discounts your last pick), `exploiter` (counts your picks
  made after it read you; challenge areas only). The read-rate slider is the chance the AI acts on
  its model; fallback when no counter exists is uniform random — no boost/utility moves exist in
  this prototype.
- `AI READ` tag remains a debug tell (checkbox); ship decision open.
- Read-rate spike (`node tools/battle-duel-spike.mjs --battles 120`): vs a random player, `reader`
  at 0→100% moves player wins 57%→38%; a full-rate `reader` is exploited by predictable rotation
  (anti-repeat policy: 33%→100% player wins), so persona defaults should sit below ~75% until
  personas mix in fallbacks.
- Switching is the real-game answer to "no counter available"; this 1v1 prototype documents it as
  the next structural mechanic rather than simulating it.
- **Switching** forfeits the round: teams are 3v3, rolled randomly from the battle seed at start
  (rerolling the seed rerolls teams; the spike uses one pinned creature per side to isolate the read
  model). Picking the swap cell sends out the next living member and hands the other side a default
  win on the arrival — flat 14, or 21 in the bonus spelling when the attacker's move type beats it.
  Both sides switching deals no damage. A KO on an incoming swap reverts to the outgoing creature; a
  side loses only when all three members have fainted. The AI switches only when its active kit
  holds no counter for its guess and a living member does; it picks the member holding the counter.
- Uses are not tracked in duel mode yet.

## Open questions

1. One-sided set pairs (no winning pick, e.g. wind/plant vs fire): author moves to remove them, or accept?
2. Edge directions flagged as arbitrary by lore review: `spirit>water`, `spirit>lightning`, `earth>plant`.
3. Preference list for the solver is hand-ordered; confirm the causal stories or re-rank.
4. Three moves per creature: who authors the 44 reworked kits and the coverage balance?

## Type-count team composition census

`node tools/battle-team-permutations.mjs [4|5|6|8]` enumerates every 3-creature team composition from
the folded types and classifies every matchup (run the tool for the full table):

| types | compositions | sequences | distinct triples | matchup pairs | mutual / one-sided / dead | pools (roster) |
| --- | --- | --- | --- | --- | --- | --- |
| 4 | 20 | 64 | 24 | 210 | 156 / 50 / 4 | tide 20, bloom 18, stone 16, flame 10 |
| 5 | 35 | 125 | 60 | 630 | 493 / 132 / 5 | water 20, spirit 13, plant 11, earth 10, fire 10 |
| 6 | 56 | 216 | 120 | 1596 | — | see tool |
| 8 | 120 | 512 | 336 | 7260 | — | see tool |

- Dead pairs are always the mono-type mirrors (one per type): no internal edge can exist, so a team
  of three identical types cannot have a positive matchup against itself. 4 types → 4 dead,
  5 types → 5 dead.
- 4-type dominance: bloom and tide beat two types each and anchor strong comps; flame (beats only
  bloom) is support; perfectly balanced comps are flame+bloom+tide and flame+bloom+stone.
- 5-type dominance: the web is a full tournament (10 edges, every pair decided); water (3 wins) and
  earth (3 wins) anchor, spirit and fire (1 win each) are support; water+earth+plant and
  water+fire+plant are perfectly balanced (adv = dis = 0).
- Roster team counts per composition sum to C(64,3) = 41,664 in every table; the smallest pools are
  mono-flame (120 teams) and mono-earth/mono-fire (120) — no composition is unfillable.

