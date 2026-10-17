# 5. Battle: the five-type duel

The shipped battle is a hidden-simultaneous-pick duel over a five-type complete tournament. Both sides commit a hidden action; the round resolves atomically; the round feedback reveals both picks and the damage. No turn order, no PP, no accuracy, no statuses.

## Type web

<figure>
<svg width="360" height="320" viewBox="0 0 360 320" role="img" aria-label="Five-type duel web">
<defs><marker id="arr" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="6" markerHeight="6" orient="auto-start-reverse"><path d="M0,0 L10,5 L0,10 z" fill="#bbb"/></marker></defs>
<!-- ring edges (attacker -> defender): spirit->fire->plant->water->earth->spirit -->
<line x1="180" y1="60" x2="270" y2="118" stroke="#bbb" stroke-width="1.6" marker-end="url(#arr)"/>
<line x1="270" y1="118" x2="232" y2="235" stroke="#bbb" stroke-width="1.6" marker-end="url(#arr)"/>
<line x1="232" y1="235" x2="108" y2="235" stroke="#bbb" stroke-width="1.6" marker-end="url(#arr)"/>
<line x1="108" y1="235" x2="70" y2="118" stroke="#bbb" stroke-width="1.6" marker-end="url(#arr)"/>
<line x1="70" y1="118" x2="180" y2="60" stroke="#bbb" stroke-width="1.6" marker-end="url(#arr)"/>
<!-- star edges: water->spirit, water->fire, earth->fire, earth->plant, plant->spirit -->
<line x1="108" y1="235" x2="180" y2="60" stroke="#888" stroke-width="1.6" marker-end="url(#arr)"/>
<line x1="108" y1="235" x2="270" y2="118" stroke="#888" stroke-width="1.6" marker-end="url(#arr)"/>
<line x1="70" y1="118" x2="270" y2="118" stroke="#888" stroke-width="1.6" marker-end="url(#arr)"/>
<line x1="70" y1="118" x2="232" y2="235" stroke="#888" stroke-width="1.6" marker-end="url(#arr)"/>
<line x1="232" y1="235" x2="180" y2="60" stroke="#888" stroke-width="1.6" marker-end="url(#arr)"/>
<circle cx="180" cy="60" r="17" fill="#202020" stroke="#fff"/><text x="180" y="65" fill="#fff" font-size="12" text-anchor="middle">SPR</text>
<circle cx="270" cy="118" r="17" fill="#202020" stroke="#fff"/><text x="270" y="123" fill="#fff" font-size="12" text-anchor="middle">FIR</text>
<circle cx="232" cy="235" r="17" fill="#202020" stroke="#fff"/><text x="232" y="240" fill="#fff" font-size="12" text-anchor="middle">PLA</text>
<circle cx="108" cy="235" r="17" fill="#202020" stroke="#fff"/><text x="108" y="240" fill="#fff" font-size="12" text-anchor="middle">WAT</text>
<circle cx="70" cy="118" r="17" fill="#202020" stroke="#fff"/><text x="70" y="123" fill="#fff" font-size="12" text-anchor="middle">ERT</text>
<text x="180" y="300" fill="#999" font-size="12" text-anchor="middle">Every arrow points from the winning type to the losing type. 10 edges; each pair has exactly one winner.</text>
</svg>
<figcaption>Authored 8 types fold to 5: wind→water, lightning→fire, elder→spirit.</figcaption>
</figure>

## Round resolution

| Case | Result |
|---|---|
| Attack vs attack, one beats the other | Winner deals **14**, loser chips **4** |
| Attack vs attack, mirror or neutral | Both deal **6** (trade) |
| Exactly one side switches | The non-switching side lands a default win (**14**) against the incoming creature |
| Both switch | No damage |
| Exactly one side waits | The other lands a default win (**14**) |
| Both wait / unusable kit | No damage |

Every attack then multiplies its base by its own matchup against the defender: **×1.5** if the move type beats any of the defender's types, **×0.5** if it loses to any, otherwise ×1 (advantage checked first; no stacking of dual weaknesses). Fixed-point tenths, rounded up, minimum 1. Both damages are computed against the same pre-hit state and applied together, so simultaneous KOs are exact; if both sides are wiped the duel is a DRAW. A KO on a member that was swapped in reverts to the (living) outgoing member; otherwise the first living member promotes.

## Attack kits

A creature's moveset is a **5-bit attack mask**: one bit per duel type. The pick row shows one option per set bit, in ascending type order. Mask 0 means no usable attack (the pick row degenerates to WAIT). Each instance carries its own mask in the save record; species data carries the default kit. Trainer presets author masks per member.

## Opponent AI (personas)

| Persona | Model | Default read rate |
|---|---|---|
| random | Never reads; still prefers its best creature-matchup tier | 0% |
| reader | Counters your last revealed type | 25% |
| frequency | Counters your most-used revealed type | 50% |
| anti-repeat | Modal type with the newest pick discounted | 50% |

On a reading turn: pick a countering attack if the active kit has one; otherwise switch to a living bench member that counters; otherwise attack uniformly. On a non-reading turn: prefer the best creature-matchup tier, then uniform. Read rates are capped below 75% because a fully reading opponent is exploitable by predictable rotation. The AI reads only revealed history and public state — never the hidden pick. Wild battles use random/0; trainer personas in the current build are a temporary `trainerId % 3` fixture (reader 25 / frequency 50 / anti-repeat 50) — the authored `data/duel.toml` persona table is generated but not yet consumed by firmware (`jp8.11.28`).

## Session flow and timing <span class="tag live">LIVE</span>

- Choice: the pick row (attack cells, SWAP, WAIT) plus an action page (GATHER / ESCAPE) when legal. The opponent's hidden pick is committed on Choice entry from the shared RNG.
- Conceal phase: **29 frames** (~558 ms at 52 fps) between commitment and reveal.
- Reveal + resolution: one atomic round; result dwell runs **68 frames** (~1308 ms); A accelerates feedback.
- HP ownership: the session writes damage back to `player.creatureHPs` by original party slot at the terminal edge; the world battle syncs HP both ways.
- Terminals: WIN / LOSE / DRAW / GATHERED / ESCAPED / FLED. LOSE and DRAW run defeat recovery (party healed, player returned to the authored start). ESCAPE is immediate on selection for wild battles.

## Rewards on the terminal edge

- **Ordinary wild win:** every living occupied party member gains `8 × foe level` exp; the foe species' authored material drops once (saturating bag).
- **Gatherable win (captured):** the foe instance is written to the 64-slot collection with species, current exp, attack mask and HP-encoded state; no exp and no material.
- **Trainer/boss win (planned for beta):** authored victory flag gates the ending; no repeated reward.
- Gathered/Fled/Escaped/Lose/Draw award nothing.
