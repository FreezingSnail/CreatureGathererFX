# Post-duel growth rearch — brief

Planning input, 2026-10-17. Owner decisions from the post-duel review session.
No production source changes are authorized by this document; implementation
beads carry the work.

## Decisions

- **Stats retire to `hpSeed`.** Duel species identity = types + attack mask + HP
  + art. The five other authored seeds are unread by the shipping duel: every
  stat read outside `.hp` lives behind `#ifndef CGFX_DUEL_BATTLE`
  (`Damage.cpp`, `Ai.cpp`, `Resolve.cpp`, `Effects.cpp`, `BattleSetup.cpp`
  legacy arms). `ogh.7` proved the class empirically (hedge `spcdef` 3→5,
  0/25 pairs changed).
- **The 5-bit attack mask is the progression/customization axis.** Evolution
  unlocks types now (adopt the target species kit); item-driven custom kits are
  a later feature — the persisted `StoreRecord.attackMask` byte is its slot.
- **Evolution**: author the full starter chains (1→2, 4→5, 7→8 at level 16; the
  `evelevel` fields already carry 16/24) and adopt the target species' mask on
  evolution, restoring the instance-mask == species-mask invariant.
- **Legacy surface**: archive the no-macro legacy battle contract run (host
  suites plus the dormant `Ai/Damage/Effects/Resolve` TUs) and the leftover
  Move/Effect/PP surface; `archive/battle-legacy/` is the precedent. Fold
  `CreatureGathererFX-1cge` (dormant `BATTLE_SIMULATOR` blocks) and use `obbp`
  evidence for the duel-unreachable feedback arm.
- **Parked decisions** (recorded, not scheduled): per-level learnset table (only
  if intra-stage type unlocks are wanted; evolution-only was chosen for now);
  one ATK seed scaling the flat 14/4/6 (cheap follow-on if HP-only fights feel
  flat — needs a full balance re-baseline); statuses/effects (retired). See
  `CreatureGathererFX-ogh` epics for the power/speed trials retired here.

## MHS reference (why HP+mask, not stat spreads)

Monster Hunter Stories 2 monsties carry full RPG stat tables — HP, Attack,
Defense, Speed, Crit Rate, Recovery plus 12 element atk/def values per level
(Kiranico per-level tables) — *because MHS kept a scaled damage formula*. Its
readable decision layer is still Power/Speed/Technical head-to-heads plus
element weakness; the producer interviews say monstie stats are deliberately
designed as one strength/one weakness, and the real build depth is genes
(3x3 equippable slots + bingos), not stat spreads. Our duel deleted the damage
formula for 1bpp communicability; the honest MHS analogue for us is kit breadth
(the mask), not revived damage numbers.

## Live vs remnant (evidence)

| Concern | Live model | Remnant | Evidence |
| --- | --- | --- | --- |
| Stats | `statlist.hp` (HP = `2*level + hpSeed*(level/3) + 30`) | `atk/deff/spcatk/spcdef/spd` seeds: authoring, `CreatureData_t` 11 B, `setStats` 6 folds, `stats_t` 6 B | `Creature.cpp`, `BattleSetup.cpp` `copyCreature` duel branch, `ogh.7` |
| HP-only read | `Combatant` duel = 6 B, `BattleState` duel = 46 B asserts | — | `BattleState.hpp` |
| Moves | `attackMask` 5-bit per instance + species default; no power/PP/accuracy | `Move.hpp`, `Effect.hpp`, `MoveIds.hpp`, `PpGlyph` battle uses, `MoveUses.hpp` legacy half, `BattleState.hpp` still includes `Move.hpp` | `moveset-type-mask-brief.md` D6 landed |
| Leveling | exp `8*foeLevel`/ordinary wild win; curve `l^3`, cap `29791@31`; level derived from exp | — | `BattleFlow.cpp`, `LevelCurve.hpp` |
| Evolution | 3 edges (0→1, 3→4, 6→7 @8); keeps exp/slot/mask; recomputes target types+HP; retry via compaction | `evelevel` 16/24 inert on stage-2/3 species (no `evolves_to`); only 3/64 species have edges | `evolution.txt`, `evolution.rs`, `Compaction.cpp:193` |
| Save | 8 B `{id, exp, attackMask, reserved[3], hpEncoded}`; no stats persisted | — | `StoreRecord.hpp` |
| Legacy run | host `make test` runs duel (`CPPFLAGS ?= -I. -DCGFX_DUEL_BATTLE`) | no-macro legacy suites registered in `tst/main.cpp` (`transition`, `battle_test/ai/resolve/faint`, `battle_session/playback/flow`, `battle_item`, `battle_utility`, `battle_slim_session`, `moves_test`) keep the legacy TUs compiling | `tst/main.cpp` guards, `Makefile` L144-190 |

## Reclaim estimates (measurement decides)

| Item | Estimate | Basis |
| --- | --- | --- |
| 5 dead stat seeds, 64 species | ≈ -320 B flash data | `CreatureData_t` 11 B → 6 B |
| `setStats` / `seedToStat` collapse | small code win | 6 folds → 1 |
| Creature trim (`statlist` 6→1-2 B; status/statMods duel-only dead) | ≈ -15..-27 B static SRAM | `Player.party[3]` |
| Legacy suites/TUs/includes | ≈ 0 device B (LTO/GC already excludes); host build/test surface shrinks | `jp8.4.11` symbol audit method |
| Evolution chains + mask adoption | +9 B FX data (3 edges) + small code | `evolution.txt` |

## Bead sequence

1. **Archive legacy contract run** (host/tooling only, 0 device B): move the
   no-macro suites and dormant TUs to `archive/battle-legacy/`, remove their
   registrations, keep every duel path untouched.
2. **Delete dead legacy arms + dormants** (depends on 1): shared sources lose
   their `#ifndef CGFX_DUEL_BATTLE` arms where nothing compiles them anymore;
   Move/Effect/PP leftovers, `BATTLE_SIMULATOR` (`1cge`), feedback-arm evidence
   (`obbp`). Any include that a live caller keeps (e.g. `PpGlyph` through
   `LureZoneView`) stays.
3. **Measured stat collapse** (depends on 2, AVR lane): schema/model/editor
   (sibling `CreatureGathererTools`) + `CreatureData_t` + `Creature`/`setStats`
   + fixtures; whole-image A/B, SRAM, stack, focused device suites.
4. **Evolution as mask progression** (depends on 3, same authored file): chains
   1→2, 4→5, 7→8; mask adoption in `BattleFlow` + `Compaction`; host/device
   evolution tests.

Storage/lane notes: items 3-4 are one AVR/generation lane each (generator
sibling repo + packed `creatureData` bytes); items 1-2 are host-only cleanup.
