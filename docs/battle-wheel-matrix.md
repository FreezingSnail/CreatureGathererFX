# Eight-type battle wheel (matchup matrix proposal)

Status: spike proposal for the simultaneous-pick duel, 2026-10-14. Source of truth is
`WHEEL` in `tools/battle-design-spike-data.mjs`, generated into `mock/data.js`; playable in
`mock/duel.html`. Not adopted into firmware or the shipping damage chart.

## The ring

```
wind > earth > lightning > water > fire > plant > spirit > elder > wind
```

Each type beats exactly the next one clockwise and loses to the previous one. Every other pair is
neutral. The full 8×8 matrix is therefore one diagram: eight icons in a ring with one arrow each.

| you | beats | loses to | neutral vs |
| --- | --- | --- | --- |
| wind | earth | elder | lightning, water, fire, plant, spirit |
| earth | lightning | wind | water, fire, plant, spirit, elder |
| lightning | water | earth | fire, plant, spirit, elder, wind |
| water | fire | lightning | plant, spirit, elder, wind, earth |
| fire | plant | water | spirit, elder, wind, earth, lightning |
| plant | spirit | fire | elder, wind, earth, lightning, water |
| spirit | elder | plant | wind, earth, lightning, water, fire |
| elder | wind | spirit | earth, lightning, water, fire, plant |

## Why a ring

- 64 cells collapse to 8 edges; learnable from one screen. Applies to the hidden simultaneous pick
  where both sides know each other's type but not the pick.
- No immunity cells: every move stays a live pick. A 0× cell in a hidden-pick game is a trap
  option that punishes reading without offering counterplay.
- Each matchup is decisive in exactly one direction, so "who wins the clash" is always well
  defined and mirror picks are neutral trades.
- Firmware-cheap: the ring is 8 successor indices (~8 B) versus the 81-cell packed chart (21 B).
  A lookup is `(defender == next[attacker]) ? win : (attacker == next[defender]) ? loss : neutral`.

## Intuition audit

Solid links (6): wind>earth (erosion), earth>lightning (grounding), lightning>water (conduction),
water>fire (dousing), fire>plant (burning), plant>spirit (nature entangles spirits).

Weakest links (flag for the owner): spirit>elder, elder>wind. If these read badly in play, options:
swap the two mystical types into a different ring slot, or accept them as authored lore taught by
the ring diagram.

## Notes for the three-move creature design

Current equipped moves (level-8 generated data, 2026-10-14):

- distinct move types per creature: 1 type ×7, 2 ×37, 3 ×18, 4 ×2 creatures. A "three moves with
  three distinct types" rule needs an authoring pass for 44 of 64 creatures.
- 7 creatures carry fewer than three moves at all.
- move-type coverage across the roster: earth 23, plant 23, water 16, elder 16, spirit 15, wind 12,
  lightning 12, fire 10.
- Duplicate-type kits exist (ember: two fire moves). The mock shows them as-is so the noise is
  visible during playtests.
- Every creature already has at least one STAB move (affinity ×1.5 in the duel).

## Duels mechanics as implemented in the mock

- Both sides pick one of their own creature's equipped moves, hidden.
- Reveal after 550 ms. Win = full hit based on the winning move's power (×1.5 affinity), loser
  deals chip; neutral or mirror = trade chip both ways. Uses are not tracked in duel mode yet.
- The AI counters your last revealed type at the read-rate slider, otherwise picks a stronger move.

## Open questions

1. Do spirit>elder and elder>wind survive play? Flagged as arbitrary.
2. Ring-only (6 neutral cells per row) or add a second win/loss edge per type for richer matchups?
3. Three moves per creature: who authors the 44 reworked kits, and how is type coverage balanced
   across the roster (fire is the thinnest at 10 carriers)?
4. Should duplicate-type moves be deduped to the strongest per type, or kept as authored?
