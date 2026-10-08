# Battle balance review — 2026-10-07

Recommendation: expand useful choices before raising caps. Keep level 31, byte-sized
stats and ±2 applied stages. Test smoother growth, repair exhausted/immunity kits,
then introduce six moves using existing effects. These are proposals, not gameplay
changes or measured candidate improvements.

## Evidence and limits

Reviewed `build/balance/expanded-64`, `movesets-full-pairwise`,
`movesets-full-random3`, and `movesets-full-tactical`. Latest report creature/move
fixture hashes match current generated fixtures:

- Creature: `9fa17e8ffe3349e3949861a85232efa580b5c19c7064afcc08b5476b03e35718`
- Move: `0d2a8e9b132a04e9a60d9e8f33008438fdec606694644dc659c5f9b5e7248cd6`

Pairwise greedy rates use 128 player-side battles per species (64 opponents,
two trials). Teams use only four equipped/default moves, not the later JSON
`moveList` pools. Opponents always use production tactical AI. Repeated deterministic
trials are not independent human samples. Random teams mix growth stages and the
original weak starting kits with the stronger expansion kits; their results are
team associations, not individual contribution measurements.

| Species | Earlier greedy 1v1 | Current greedy 1v1 | Interpretation |
| --- | ---: | ---: | --- |
| NibWeevil | 26/128 (20.3%) | 26/128 (20.3%) | More moves did not fix the matchup problem |
| StaticMite | 38/128 (29.7%) | 60/128 (46.9%) | Coil is the clearest useful kit change |
| ShardWisp | 80/128 (62.5%) | 84/128 (65.6%) | Smite helps some matchups; modest overall change |
| GeodeGuard | 84/128 (65.6%) | 82/128 (64.1%) | Ironbody gives identity, not a demonstrated general buff |
| SaplingSage | 42/128 (32.8%) | 42/128 (32.8%) | Recovery alone does not solve the offensive kit |
| Shieldshoe | 118/128 (92.2%) | 118/128 (92.2%) | Do not strengthen its stats based on weak species elsewhere |
| CinderCocoon | 114/128 (89.1%) | 112/128 (87.5%) | Already strong in this screen |

Current random-3v3 player wins: greedy 157/300; random 104/300; tactical 158/300.
Mean turns: 16.99, 19.79, 16.85 respectively. There are three mixed-policy
timeouts and zero tactical timeouts. Tactical SaplingSage teams won 8/21 versus
7/21 under greedy; one extra win does not establish a healing benefit.

All 16 current pairwise timeouts are the ordered Reliquary/OrbitRelic combinations,
including their mirrors. Both have finite elder attacks and unlimited spirit attacks;
elder defenders are immune to spirit. Adding unlimited **Burst**, already in their
pools, is the first candidate repair. Replacing ElderBurst preserves ElderSlam as
their finite finisher. This does not fix all water immunity matchups.

## Formula audit

Inspected `src/creature/{Creature.cpp,LevelCurve.hpp}`, `src/lib/{Stats.hpp,
StatModifier.hpp,Type.hpp,Type.cpp,Move.hpp}`, `src/engine/battle/{Damage.cpp,
Effects.cpp,MoveUses.hpp,Resolve.cpp,Ai.cpp,BattleSetup.cpp}`, and `StoreRecord.hpp`.

| Rule | Current behavior | Recommendation |
| --- | --- | --- |
| Stat growth | `2*L + seed*floor(L/3)`; HP adds 30 | Test `2*L + floor(seed*L/3)` with wide intermediate arithmetic. Current growth erases all seed differences at levels 1–2 and jumps every third level. Smooth growth restores early roles. |
| Base stat bounds | Six runtime stats are `uint8_t`; seeds stored as bytes, current seeds reach 15 | Keep runtime 255 maximum, explicitly validate/clamp calculations instead of wrapping. Do not raise seeds globally. With current growth at L31, non-HP seeds must be ≤19 and HP seeds ≤16 to fit; these are arithmetic limits, not balance targets. |
| HP | `growth+30`, then byte storage | Keep +30 for the first growth experiment. Growth/HP changes affect pacing; separate them from damage changes. Current largest HP seed is 14. |
| Experience | Cumulative `L³`, table levels 1–31, two-byte experience | Keep 31 now. L40 costs 64,000 and fits experience; L41 costs 68,921 and does not. A later L40 design needs new growth/HP bounds, not just more thresholds. No battle XP award or evolution application was found in the reviewed runtime source. |
| Damage | `floor(floor(staged(power*A)/max(1,staged(floor(D/2))))/2)`, then type/status/STAB multiplier, min 1 unless immune, max 255 | Test a simpler ratio `floor(power*staged(A)/max(1,staged(D)))` as a separate experiment. Halving defense before division introduces extra truncation, especially for tiny stats. Avoid another global damage reduction: high-level battles already stall. |
| Same-type bonus | ×2 combined with type/status multipliers; combined scale clamped to ¼…4×, immunity remains zero | Keep baseline first. Compare ×1.5 only if low-level burst remains too high after smoothing; that requires a separate arithmetic design. At the current 4× ceiling, some boosts have no additional damage benefit. |
| Type chart | Plant, lightning and elder attack rows are identical; fire differs from these only against lightning. Water is immune to plant/fire/elder; elder immune to spirit | Authored/runtime parity was fixed by closed bead 62o. Balance is still questionable. First test plant→water 0→½ independently; zero damage is a major reason extra plant moves cannot rescue NibWeevil. Broader type differentiation is a separate chart experiment. |
| Applied stages | ±2; multipliers at −2,−1,0,+1,+2 are ½,⅔,1,1½,2 | Keep ±2. Raising it promotes longer setup and greater snowballing. Packed sign/magnitude fields can represent ±3; ±4 or Pokémon ±6 need a representation redesign. Damage helper’s ±4 table does not make ±4 an applied cap. |
| Speed/priority | Staged speed; player wins ties; switch/gather/item/escape priority precedes attacks | Keep priorities; measure seat bias before changing ties. At L1–2 all unmodified speeds match, making current early-level screening heavily player-favored. |
| Move uses | Effectless power <10 unlimited; power ≥10 or any effect has two uses; zero-power stat moves have three | Keep limits initially. Every tested kit needs an unlimited attack that can damage its intended matchups. Reaching a wider pool does not automatically solve an equipped-kit deadlock. |
| Regeneration/drain | Regen `floor(maxHP/8)` for three ticks; sap `max(1,floor(maxHP/16))` without an expiry timer | Keep regen. Test sap expiry at three ticks as its own control experiment; persistent damage is valuable and should not be handed to every plant by default. Regen may tick on its application turn. |
| Status capacity | Two effect slots; switching clears statuses; stat stages persist | Retain capacity. Elemental status scaling follows the creature’s types, not only the chosen move’s element. Check dual-type saturation before giving more charged attacks. |
| Accuracy/effects | No accuracy or critical rolls; valid move effects currently roll at 100% | Do not balance high power by authored accuracy: it is not executed. Prefer adjusting power/uses. More RNG is an optional feature with a firmware cost. |
| Pin/confusion | Pin skips with probability ⅓; confusion self-hits with probability ¼, three ticks each | Retain duration. Confusion uses the selected move against self and no secondary effects/PP spend; it is not a fixed-power neutral hit. Avoid more high-power confusion moves until tested. |
| Gather/escape | Need `clamp(8+floor(L/2),8,24)`; gather adds tier rate; flee after six end turns; legal wild escape succeeds | Audit tier availability: needed actions are `ceil(need/rate)`. At L31 need=23, so rate 4 needs six actions; lower rates cannot finish inside that window. Combat-control samples do not measure gathering usefulness. |
| AI utility | Recognizes heal, attack/special attack buff, physical defense buff and speed debuff; usually one setup stage | New special-defense/defense-breaking moves will be usable by humans but not properly evaluated by current tactical selection. Extend host candidate evaluation before concluding those moves are weak. |

### Additional level screen

Ran the unchanged production engine with current fixtures, all 64 ordered pairs,
one trial, tactical player policy, seed 20261007, 100-turn cap:

| Level | Battles | Player wins | Mean turns including timeouts | Timeouts |
| --- | ---: | ---: | ---: | ---: |
| 1 | 4,096 | 2,568 | 1.916 | 0 |
| 10 | 4,096 | 2,107 | 4.431 | 4 |
| 31 | 4,096 | 2,045 | 13.366 | 81 |

These support a pacing problem across the existing range, not a need for more
levels. At equal level, attack/defense ratios barely grow while HP grows markedly;
finite attacks run out in longer fights. Seed 15 HP would wrap at L33 (261→5);
the actual current HP seed maximum 14 wraps at L36 (270→14). Non-HP seed 15 wraps
at L39 (273→17). Raising the cap without changing arithmetic is unsafe.

## Proposed move expansion

Add these six first; names/powers remain candidate values. All accuracy 100;
all use existing effect enums. No new effect dispatcher, animation or RAM fields.

| Move | Type/category | Power/effect | Uses | Need and thematic owners |
| --- | --- | --- | --- | --- |
| Sprout | Plant/special | 5 / none | Unlimited | Special plants currently rely on finite Pollen. NibWeevil, flowers, spores, ChimeSeed, sages; add selectively to older Hedge/Suculent/Cactus/Dragon pools. |
| Echo | Spirit/special | 5 / none | Unlimited | Special spirits have only finite Smite; Thought/Wisper are physical. Mushrooms, bells, sages, relics; Bells/Item/Skull where fitting. |
| Cinder | Fire/physical | 7 / none | Unlimited | Torch is finite and Stoke has an effect. CoalGrub/FurnaceBeetle and selected older fire bugs/worms gain a physical fallback. |
| Ward | Status | 0 / SPCDUP | 3 | Missing special-defense setup. GeodeGuard, Conelet, Reliquary, SaplingSage, Rock/Item2. |
| Fray | Status | 0 / SPCDDWN | 3 | Missing special-defense breaking. SporeKeeper, NectarBloom, ShardWisp, BoughBell; selected spirit/plant originals. |
| Shellcrack | Status | 0 / DEFDWN | 3 | Missing physical defense breaking. GranaryWeevil, Horseshoe, ReefReaper, AncientJaw, FurnaceBeetle; crab/elder originals. |

Later candidates only if the six-move pass exposes a need: **Sapdust** (status,
SPCADWN) for mushrooms/flowers; **Pebble** (earth/physical, power 5, no effect) as
a gentle early physical attack. Dirtfall already covers unlimited earth physical
damage, so Pebble is a progression choice rather than a missing combat role.
Keep Pollen/PollenBurst, Coil, Sharpen, Ironbody, Deepthought, Sweep and Rejuvenate.
Use existing Swiften/Terrorize as optional pool choices; prior tests do not justify
equipping them universally. Display spelling cleanup is separate from semantic IDs.

## Pool structure

Target 6–8 choices for simple species, 8–10 for specialists, and 10–12 for a
deliberately versatile final form. Keep four equipped moves. Pool size need not
increase on every evolution, but later forms should retain useful earlier moves.
Choose a reliable fallback, two distinct attacking options, one role tool, and
two to four alternatives. Limit off-type coverage to one or two concept-backed
options. Do not give every dual-type species every move in both type catalogs.

The following are complete proposed expansion pools, including candidate new
moves. They replace the blanket type-derived pools if adopted; none are implemented.
Coverage against every immunity is not promised: test the chart candidate and
equipped kits together after separate measurements. All names use readable display
spelling. Optional physical/special alternatives allow different four-move builds.

| Creature | Proposed pool | Intended distinction |
| --- | --- | --- |
| NibWeevil | Sprout, Root, Seedfall, Pollen, Deepthought, Rejuvenate | Six choices; special seed gatherer, not a generic poison/healing wall |
| HuskWeevil | Sprout, Root, Seedfall, Dirtburst, Dirtfall, Slipfall, Ironbody, Shellcrack | Husk armor and digging |
| GranaryWeevil | Sprout, Root, Seedfall, Dirtburst, Dirtfall, Slipfall, Pollen, Sharpen, Shellcrack | Physical harvest striker |
| DewBud | Sprout, Splash, Squirt, Current, Pollen, Root, Seedfall, Rejuvenate | Dew recovery; preserve physical option |
| NectarBloom | Sprout, Splash, Squirt, Soak, Pollen, PollenBurst, Deepthought, Fray, Rejuvenate, Seedfall | Special bloom/control; retain earlier recovery |
| Sporeling | Sprout, Echo, Thought, Root, Seedfall, Pollen, Blast | Small mixed spore kit |
| SporeKeeper | Sprout, Echo, Thought, Root, Seedfall, Pollen, PollenBurst, Ironbody, Fray, Blast | Spore endurance/debuff, retains Blast |
| MossMole | Dirtburst, Dirtfall, Slipfall, Root, Seedfall, Shatter, Ironbody, Rejuvenate | Digging physical utility |
| Conelet | Splash, Squirt, Current, Dirtburst, Dirtfall, Slipfall, Ironbody, Ward | Shell with two defense builds |
| SpireSquid | Splash, Squirt, Soak, Deluge, Burst, ElderBurst, Deepthought, Current | Special ancient-water caster |
| Horseshoe | Splash, Current, Dirtburst, Dirtfall, Slipfall, Shatter, Ironbody, Shellcrack | Armored physical aquatic |
| Shieldshoe | Dirtburst, Dirtfall, Slipfall, Shatter, Burst, ElderBurst, Ironbody, Shellcrack, Splash, Current | Armored elder; water options retain aquatic family identity |
| SiltSkorp | Splash, Current, Dirtburst, Dirtfall, Slipfall, Soak, Shatter, Swiften | Silt control rather than another generic wall |
| ReefReaper | Splash, Current, Soak, Deluge, Burst, ElderBurst, ElderSlam, Shellcrack | Reef offense; physical/special choices |
| Platefin | Splash, Current, Dirtburst, Dirtfall, Slipfall, Shatter, Ironbody, Shellcrack | Plate defense and physical pressure |
| AncientJaw | Splash, Current, Soak, Burst, Slam, ElderSlam, ElderBurst, Sharpen, Shellcrack | Physical jaw; reliable Current/Burst |
| CoalGrub | Cinder, Burn, Melt, Stoke, Dirtburst, Dirtfall, Slipfall | Fire grub with physical fallback |
| CinderCocoon | Cinder, Burn, Melt, Stoke, Dirtburst, Dirtfall, Slipfall, Ironbody, Deepthought | Shell or special setup; retain family moves |
| FurnaceBeetle | Cinder, Burn, Melt, Torch, Stoke, Dirtburst, Dirtfall, Slipfall, Shatter, Sharpen, Shellcrack | Physical furnace, no automatic defense buff |
| Flintling | Dirtburst, Dirtfall, Slipfall, Bolt, Zap, Plasma, Sharpen | Mixed flint offense |
| GeodeGuard | Dirtburst, Dirtfall, Bolt, Zap, Plasma, Ironbody, Ward, Deepthought | Special wall with choices, not three defenses equipped |
| Tuftling | Breeze, Blow, Bellow, Sprout, Root, Seedfall, Pollen, Swiften | Special wind/plant scout |
| GustTuft | Breeze, Blow, Bellow, Torrent, Sprout, Root, Seedfall, Pollen, Cyclone, Sharpen, Swiften | Physical gust/control; retain early options |
| StaticMite | Bolt, Zap, Coil, Root, Seedfall, Sprout, Pollen, Sharpen | Retain proven Coil kit; optional plant special route |
| ChimeSeed | Thought, Wisper, Echo, Sprout, Root, Seedfall, Pollen, Ward | Gentle spirit seed/support |
| BoughBell | Thought, Wisper, Echo, Sprout, Root, Seedfall, Pollen, PollenBurst, Deepthought, Fray, Ward | Special bell/debuff; retain Ward |
| ShardWisp | Thought, Wisper, Echo, Smite, Dirtburst, Dirtfall, Shatter, Fray | Relic damage; Smite remains |
| Reliquary | Thought, Echo, Smite, Blast, Burst, ElderBurst, ElderSlam, Ward | Mixed relic with unlimited elder fallback |
| SaplingSage | Sprout, Root, Seedfall, Burst, ElderBurst, Pollen, Rejuvenate, Deepthought, Ward | Special grove sustain |
| GroveKeeper | Sprout, Root, Seedfall, Burst, ElderBurst, Pollen, PollenBurst, Rejuvenate, Deepthought, Ward, Ironbody | Mature grove with build choices, preserves recovery |
| FossilBloom | Sprout, Root, Seedfall, Pollen, Burst, ElderBurst, Slam, ElderSlam, Ironbody, Shellcrack | Fossil physical armor breaking |
| OrbitRelic | Thought, Echo, Smite, Blast, Burst, ElderBurst, Slam, ElderSlam, Deepthought | Mixed orbital offense; Burst avoids spirit-only fallback |

Original roster also needs curation rather than keeping 29/31 options just because
those lists predate the expansion. Suggested family direction:

| Original creatures | Pool direction |
| --- | --- |
| Squibble/Squable/ScrambleSnail | Wind basics → Torrent/Cyclone; later Root/Seedfall, one water option |
| Skitter/Scatter/ShatterCrab | Dirtfall/Dirtburst, Shellcrack/Ironbody; lightning only with the storm concept |
| Squid/BigSquid/BiggestSquid | Splash/Current → Soak/Deluge; fire only in the final water/fire form |
| Bell, Item, Item2 | Physical bell gets Thought/Blast/Sharpen; special relic gets Echo/Smite/Ward; cap Item/Item2 at about 10–12 thematic choices |
| Ember, Flickerfly/Flitfly, Wiggle/Waggleworm | Preserve physical/special choices: Cinder for physical fire, Burn for special; wind for flying insects, earth for worms |
| Circuit, Zip/Zap | Bolt/Zap/Coil plus one role tool; water only for Zap's dual type/concept |
| Hedge, Suculent/Cactus | Sprout/Pollen vs Root/Seedfall; recovery for hedge, earth/defense tools for cactus |
| Cloud, Billow/Howl | Breeze/Blow/Bellow/Torrent plus Sweep; lightning for storm concepts |
| Skimskate/Skimray | Current/Splash plus plant options for ray; recovery or control rather than every water move |
| Rock | Dirtburst/Dirtfall/Ironbody/Ward plus one relic spirit option |
| Dragon, Skull, Ardu | Burst plus finite elder finisher, dual-type physical/special choices, one utility, one justified extra coverage; 10–12 maximum |

The four repeating expansion stat templates rotate across 32 entries. Several
visual evolution families do not grow monotonically; e.g. DewBud HP seed 11 becomes
NectarBloom 8, and SpireSquid/AncientJaw trade special attack for physical attack.
Make these explicit sidegrades or tune family roles before assuming a level-cap
increase will create progression. Do not inflate every creature to fix template reuse.

## Recommended experiments and budget

1. Equip Burst for Reliquary/OrbitRelic; compare identical pairwise matrices and
   timeout traces. Keep this existing-move repair distinct from new content.
2. Test smooth growth at L1,2,3,10,20,31. Compare early roles, attack/defense
   matchups, battle length and HP bounds. Keep +30 HP and damage baseline.
3. Test plant→water ½ independently against baseline. Review winners/losers;
   preserve the authored/runtime table contract and all dual-type compositions.
4. Spike Sprout and Echo, then the remaining four candidates. Sweep curated
   four-move builds, not randomly enlarged pools; evaluate physical, special,
   setup and sustain builds separately, including capped-PP immunity cases.
5. After kits settle, compare the cleaner damage ratio and any sap-duration change
   separately. Repeat matched-stage cohorts at multiple levels and multiple seeds;
   add gathering trials and seat comparisons. Suggested screening flags: any
   persistent timeout, near-zero useful attacks, and whether role tools ever improve
   outcomes without overcentralizing them. Do not force baby/final forms to 50%.

New move records and names can live in FX data, but generator mappings/menu ID
limits must be inspected before declaring firmware cost zero. JSON pool metadata
currently is not packed or consumed by runtime learning. Pack selected pool IDs
on FX if learning is implemented; load on transitions, keep four equipped slots,
and avoid a per-creature RAM pool. Six moves have a 24-byte raw move-record payload
(four bytes each), before names, address tables, alignment and mapping costs.
Whole-image measurements, not that payload estimate, determine actual cost.

Reference shipping figures from the last completed gate: 27,480 B firmware,
1,787 B static RAM, 773 B free; device stack headroom 423 B; packed cart 700,160 B.
No firmware/data changes were made in this review. No new resource delta is claimed.

New review commands (repeat with L=1,10,31 and a distinct directory per level):

```sh
build/tools/battle-sim/battle-sim --mode pairwise --species-count 64 --level 31 --trials 1 --seed 20261007 --policy tactical --max-turns 100 --output-dir build/balance/review-level31
```

The roster HTML now has a plain heading and correctly uses a 0–15 seed display
scale. It still displays current implemented content, not these proposed pools.
