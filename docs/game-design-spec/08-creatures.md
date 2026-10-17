# 8. Creatures and content roster

- **64 authored species** (ids 0–63): 32 shipping catalog + 32 expansion. Shipping encounters and the save format use the 32-catalog; expansion ids appear only in opt-in test builds.
- Each species record: id, up to two types (authored 8-type), evolution level, six stat seeds (retiring to hp), default attack mask.
- Art: 48×48 front/back masked sprites per species plus world scenery and fonts.
- Battle labels use the shipped 5×6 font with authored width metrics.

| Id | Species | Types | Default kit (folded) | Evolution |
|---|---|---|---|---|
| 0 | SquibbleSnail | wind (→water) | water | → 1 @ 8 |
| 1 | SquableSnail | wind | water | <span class="tag plan">PLANNED</span> → 2 @ 16 |
| 2 | ScrambleSnail | wind / plant | water + plant | terminal |
| 3 | SkitterCrab | earth | earth | → 4 @ 8 |
| 4 | ScatterCrab | earth | earth + fire | <span class="tag plan">PLANNED</span> → 5 @ 16 |
| 5 | ShatterCrab | earth / lightning (→fire) | earth + fire | terminal |
| 6 | squid | water | water | → 7 @ 8 |
| 7 | bigsquid | water | water | <span class="tag plan">PLANNED</span> → 8 @ 16 |
| 8 | BiggestSquid | water / fire | water + fire | terminal |
| 31 | ardu | — | boss kit | first-beta boss (planned) |

Other first-beta wild species: 9 bell, 10 ember, 12 hedge, 13 cloud, 14 rock. Full roster data lives in `data/json/creatures.json` and generated headers; see the pipeline section.
