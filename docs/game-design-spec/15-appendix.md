# 15. Appendix: type tables, curve and quick numbers

## Authored types (8) and the duel fold

| Authored id | Name | Duel fold |
|---|---|---|
| 0 | spirit | spirit |
| 1 | water | water |
| 2 | wind | water |
| 3 | earth | earth |
| 4 | fire | fire |
| 5 | lightning | fire |
| 6 | plant | plant |
| 7 | elder | spirit |

## Attack vs single-type defender (attacker row)

| | SPR | WAT | ERT | FIR | PLA |
|---|---|---|---|---|---|
| SPR | T | L | L | W | L |
| WAT | W | T | W | W | L |
| ERT | W | L | T | W | W |
| FIR | L | L | L | T | W |
| PLA | W | W | L | L | T |

<span class="w">W</span> = 14, <span class="l">L</span> = 4, <span class="t">T</span> = 6 before the creature matchup multiplier (×1.5 / ×0.5 / ×1).

## Experience curve (cumulative, selected levels)

| Level | 1 | 3 | 5 | 8 | 16 | 24 | 31 |
|---|---|---|---|---|---|---|---|
| Exp | 1 | 27 | 125 | 512 | 4,096 | 13,824 | 29,791 |

Exp = L³. Max HP at a level = `2×L + hpSeed×(L/3) + 30`; starter seeds (hp 3–4) give 46–56 HP at level 8 for typical wild creatures.

## Quick numbers

| Constant | Value |
|---|---|
| Damage win / chip / trade | 14 / 4 / 6 |
| Matchup modifiers | ×1.5 / ×1.0 / ×0.5 (tenths 15/10/5) |
| Party size / team pips | 3 per side |
| Pick options cap | 5 attack types (+ SWAP/WAIT cells) |
| AI history cap | 8 revealed player types |
| Conceal / result dwell | 29 / 68 frames at 52 fps |
| Encounter step chance | 10% on encounter tiles |
| Gather need | clamp(8 + level/2, 8, 24) |
| Gather flee window | 6 rounds (paused on KO rounds) |
| Wild win exp | 8 × foe level |
| Level cap / exp cap | 31 / 29,791 |
| Lure charge / rate by tier | 16/32/64 steps; 1/2/4 per gather round |
| Item saturation | 99 per stack |
| Collection capacity | 64 instances × 8 B records |
| Save sectors | 8 × 4,096 B (main, journal, store A/B, 4 reserved) |
| Map / chunk / tile | 256×256 tiles; 8×4 chunks; 16 px |
| Movement step | 1 tile per 16 frames |
| Script slot | 128 B per chunk, If/TpIf/End |
