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

## The solver guarantee

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

- equipped moves per creature: 1 ×3, 2 ×12, 3 ×3, 4 ×46. Fifteen creatures field fewer than three
  moves (bell fields two: wisper + Sharpen; SquibbleSnail one), so some duel footers show fewer than
  three cells. The mock shows the real kits; normalizing kits is an authoring pass.
- distinct move types per creature: 1 type ×7, 2 ×37, 3 ×18, 4 ×2. 44 creatures need an authoring
  pass to field three distinct-type moves.
- move-type carriers: earth 23, plant 23, water 16, elder 16, spirit 15, wind 12, lightning 12, fire 10.
- every creature already has a STAB move (affinity ×1.5 in the duel).

## Play rules as implemented in the mock

- Both sides pick one of their own creature's moves, hidden; 550 ms reveal.
- **Power spelling** (`mock/duel.html`): win = full hit scaled by the winning move's power
  (4 + 1.2 × power, ×1.5 affinity), loser chips 2 + 0.25 × own power; neutral or mirror = trade chip.
- **Pure-RPS spelling** (`mock/duel-pure.html`): power, stats and affinity are ignored; the relation
  alone scores — win 14, chip 4, trade 6 both.
- Both windows share the web, option source and seed, so they can run side by side.
- The AI counters your last revealed type at the read-rate slider when a counter exists in its own moves
  (power spelling then prefers the strongest counter).
- Uses are not tracked in duel mode yet.

## Open questions

1. One-sided set pairs (no winning pick, e.g. wind/plant vs fire): author moves to remove them, or accept?
2. Edge directions flagged as arbitrary by lore review: `spirit>water`, `spirit>lightning`, `earth>plant`.
3. Preference list for the solver is hand-ordered; confirm the causal stories or re-rank.
4. Three moves per creature: who authors the 44 reworked kits and the coverage balance?
