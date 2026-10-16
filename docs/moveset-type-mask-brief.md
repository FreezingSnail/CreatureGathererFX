# Duel moveset simplification — 5-bit type-mask brief

Planning input, 2026-10-15. Owner direction: a creature's moveset becomes a **5-bit
type mask** (one bit per duel type), and the old moveset machinery is removed.
Follow-on to epic `CreatureGathererFX-d73i`. No production source changes are
authorized by this document; implementation children land after approval.

## Why

The duel resolved the moveset to *type* only: the pick row, kernel and AI read
`foldType(moveTypes[slot])`; `moveIds[slot]` is consulted only to detect an empty
slot. Nothing in the duel reads a move's name, power, physical/special flag,
accuracy or effect. Yet the interface still carries all of it:

- `data/json/moves.json` — a global 45-move table (`id,name,type,power,physical,accuracy,effect`).
- `data/json/creatures.json` — per creature both `move1..4` (equipped ids) **and** an
  inline `moveList` duplicating the global table (edited by the Tauri `CreatureEditor`).
- Generated `fxdata/data/movelists.txt` (packed 32-bit `Move`s) + `duelMoveTypes[45]`.
- Device `Combatant.moveTypes[4]` + `moveIds[4]`, `Creature.moves[4]` + `moveList[4]`,
  packed `Move`/`MoveBitSet`, PP/`moveUsesSpent` semantics.

With five duel types, "which attacks does this creature know" is one `uint8_t`.
The device already proves the shape: `duelAttackMask(Creature)` folds a creature's
authored slots into exactly such a mask for the bench. This redesign makes that mask
the only moveset representation.

## Target representation

- **Creature moveset = `uint8_t attackMask`**, bits 0..4 = duel types in the pinned
  fold order `[spirit, water, earth, fire, plant]` (`data/duel.toml` `fold.order`).
  `0` = no usable attack (WAIT).
- **Authored data** (`creatures.json`): replace `move1..4` **and** `moveList` with one
  field, e.g. `"moves": ["spirit", "water"]`, listing the known attack types. Authoring
  stays in the authored 8-type names (consistent with `type1`/`type2`); the generator
  folds to the 5-bit mask. (Decision D1: names list vs raw mask int.)
- **Generator** emits a per-creature `attackMask[]`; no `moves.json`, no
  `movelists.txt`, no `duelMoveTypes[]`, no `Move` packing.
- **Device**: `Combatant.attackMask` (1 B) replaces `moveTypes[4]` + `moveIds[4]`;
  `Creature.attackMask` replaces `moves[4]` + `moveList[4]`; `DuelView` iterates mask
  bits; `duelAttackMask` collapses to the field read.
- **Save** (`StoreRecord`): `moves[4]` (4 B) → `attackMask` (1 B). One byte suffices.
- **Editor**: `CreatureEditor` move list → five type checkboxes.

## What is deleted

- `data/json/moves.json`; `creatures.json` `moveList`/`move1..4`; generated
  `movelists.txt`, `duelMoveTypes[]`.
- Device `src/lib/Move.hpp` (`Move`, `MoveBitSet`, accuracy/effect packing), `Effect.hpp`
  use from movesets, `Creature::loadMoves/setMove/getMove/moveTypeBonus`,
  `CreatureData_t.move1..4` (→ one mask byte), `CreatureSeed.moves` (uint32 8-bit move
  ids → mask byte).
- Duel-build `moveUsesSpent` aliasing (the mask is now first-class; keep or drop the PP
  name).
- `moves_json` config key in `cgfx-project.json` (and the generator `Config`).

## Ripple map

| Area | Files |
| --- | --- |
| Data | `data/json/creatures.json`, `data/json/moves.json` (delete), `cgfx-project.json` |
| Generated | `fxdata/data/movelists.txt`, `duel_data.hpp`, creature fixture, `src/fxdata.h`, `fxlayout.toml` (moveList symbol) |
| Generator (sibling) | `config.rs`, `model.rs`, `store.rs`, `builder.rs`, `duel.rs`, `fixtures.rs`, `lib.rs`, editor `ui/src/*` |
| Device battle | `BattleState.hpp`, `BattleSetup.{cpp,hpp}`, `DuelView.hpp`, `BattleSession.{cpp,hpp}`, `DuelKernel.hpp`, `Ai.cpp` |
| Device creature/save | `creature/Creature.{hpp,cpp}`, `lib/DataTypes.hpp`, `save/StoreRecord.hpp`, `save/Compaction.cpp`, `player/Player.cpp` |
| Arena | `engine/arena/ArenaDemo.cpp` |
| Tests | `tst/duel_*`, `tst/fxdatatest/{save,world_player,battle_duel,duel_ui,duel_resources,stack}_test*`, `tst/battle_flow_test.hpp`, `tst/encounter_test.hpp` |
| Parity/sim | `tools/battle-duel-spike.mjs`, `tools/duel-parity/*`, `sim-test`, `mock/*` |

## Save format

`StoreRecord` is an 8-byte wire ABI. Plan: keep 8 bytes; replace `moves[4]` with a
1-byte `attackMask` and reclaim the spare. This collides with the in-flight
`jp8.11.14` WIP, which repurposes the `reserved` byte as `hpEncoded`. **Decision D2**:
sequence the mask change after that WIP lands, or fold both into one layout change.
Existing v3 saves store move ids; a fresh boot default or an id→type fold is needed
for migration. **Decision D3**: migrate, version-bump, or treat pre-existing records
as "full mask of own authored types".

## Open decisions

- **D1** authored schema: type-name list vs raw mask int (recommend names list).
- **D2** save layout sequencing vs the `jp8.11.14` WIP.
- **D3** existing-save migration.
- **D4** whether `moves.json` / move names are needed anywhere else (assume no).
- **D5** cap: with 5 bits a creature may know all five; pick row shows up to 5 options
  (currently 4). Confirm the UI/pick-row limit.

## Frozen contract (approved 2026-10-15)

- **Mask encoding.** `uint8_t attackMask`, bit *i* = duel type *i* in the pinned fold
  order `[spirit=0, water=1, earth=2, fire=3, plant=4]`. `0` = no usable attack (WAIT).
  Built at generation from authored 8-type names via the existing fold map.
- **D1 data.** `creatures.json` drops `move1..move4` and `moveList`; adds
  `"moves": ["spirit","water"]` (authored 8-type names, duplicates folded, `[]` allowed).
  `data/json/moves.json` deleted; `moves_json` removed from `cgfx-project.json` and the
  generator `Config`.
- **Generated.** Per-creature `uint8_t attackMask[]` replaces `movelists.txt` and
  `duelMoveTypes[]`; the `fxlayout.toml` `moveList` symbol is replaced by `attackMask`.
- **Device.** `battle::Combatant.attackMask` (1 B) replaces `moveTypes[4]` + `moveIds[4]`
  (duel build); `Creature.attackMask` replaces `moves[4]` + `moveList[4]`;
  `CreatureData_t.move1..4` (4 B) → `attackMask` (1 B); `CreatureSeed.moves` `uint32` →
  `attackMask`. `DuelView` enumerates mask bits in ascending type order; `duelAttackMask`
  becomes a field read; the `moveUsesSpent` bench-mask aliasing is dropped. AVR asserts
  for `Combatant`/`BattleState` are updated to the new sizes.
- **D2/D3 save.** `StoreRecord` keeps its 8-byte wire size; `moves[4]` → `attackMask`
  (1 B). The layout change lands **after** the in-flight `jp8.11.14` WIP (which owns the
  `reserved`/`hpEncoded` byte) as one coordinated layout change. Pre-existing records
  without a mask seed from the species' authored mask; no per-record id→type table.
- **D4** `moves.json` has no other consumer; deleted.
- **D5 pick row.** Up to **5** attack options (was 4) plus SWAP/WAIT; `Cells` arrays and
  the renderer sized for 5.
- **D6 (newly surfaced).** "Old moveset code" is removed: packed `Move`/`MoveBitSet`,
  `Effect` move effects, `Creature::loadMoves/setMove/getMove/moveTypeBonus`, PP. The
  legacy no-macro battle build's packed-move suites are retired with this epic (the
  engine already lives in `archive/battle-legacy/`); world/VM/save coverage is preserved.
  Flag for veto if the legacy battle build should instead keep generating move data.

## Proposed bead sequence

1. **Freeze contract** (design): schema, mask encoding, save layout, D1–D5.
2. **Generator + data** (sibling): `moves` type list → `attackMask[]`; delete
   `moves.json`/`movelists.txt`/`duelMoveTypes`; native tests.
3. **Device battle**: `Combatant.attackMask`; `DuelView`/`BattleSession`/`DuelKernel`/
   `BattleSetup` on the mask; drop packed `Move` from the duel path.
4. **Device creature/save**: `Creature.attackMask`, `StoreRecord`, `Compaction`,
   `CreatureData_t`, `CreatureSeed`; migration per D2/D3.
5. **Editor** (sibling): five type checkboxes.
6. **Parity/sim/mock + fixtures**: regenerate and re-baseline.
7. **Gate**: focused duel+save device suites, then one integrated final gate.

Each child is one implement/measure/report loop with an AVR whole-image delta; the
duel gate (legacy vs duel per-suite profiles) is reused.
