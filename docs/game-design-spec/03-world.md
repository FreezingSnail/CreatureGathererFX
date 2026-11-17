# 3. Core loop, world and encounters

## Core gameplay loop

```
 walk the area ──► face a deposit tile + A ──► deposit a lure
      │                                            │
      │ step onto an encounter tile (10% roll)     ▼
      ▼                                     active lure zone (rate 1/2/4)
 wild duel begins ◄──────────────┐              │
      │                          │              │
      ├─ WIN (ordinary wild) ─► exp to living party,  material drop
      ├─ WIN (gatherable) ────► GATHER progress banks toward capture
      │                          │
      │                          └─ progress reaches need ─► GATHERED: instance written to collection (64 slots)
      ├─ ESCAPE (wild only) ──► return to world on the same tile
      ├─ LOSE / DRAW ─────────► defeat recovery: party healed, returned to start
      └─ FLED (gather clock ran out) ─► battle ends, no capture
```

Growth runs on the same terminal edge as rewards: ordinary wild wins grant experience to every living party member, which can raise levels, grow max HP, and trigger authored evolution. The collection is the end game: gathered instances keep their species, level, attack kit and damaged HP state across resets.

## World and exploration

| Aspect | Design |
|---|---|
| Map | 256×256 tiles, authored as `maps/world_map.json`, packed into the FX image (tile 16×16 px) |
| View | Fixed player-centered camera; 8×4-tile (128×64 px) viewport; player drawn at screen (56, 24) |
| Chunks | 8×4-tile chunks (2048 total); tile property window caches a 9×5 area; script slot = 128 B per chunk |
| Player | Masked 16×16 chibi, 4 directions × (idle + 2 walk frames), packed as append-only FX field |
| Movement | One tile per 16 frames; priority LEFT > RIGHT > UP > DOWN; no diagonals; walk animation from the 16-px step |
| Collision | Destination tile must be in bounds, non-empty (GID ≠ 0) and carry `PROP_WALKABLE`; properties authored in `maps/tiles.tsx` (walkable / water / encounter / spare / plantable) |
| Interactions | Face tile + A: deposit lure (`PROP_SPARE`), plant-claim (`PROP_PLANTABLE`), rest heal (authored rest tile), all coordinate/data driven |

## World scripts and warps <span class="tag live">LIVE</span>

The shipping profile is the compact warp VM: one forward pass over a 128-byte slot per chunk, big-endian operands, only `If` / `TpIf` / `End`. A build validator checks flag capacity, condition values, branch encoding, forward-jump command boundaries and map coordinates before firmware compilation; zero-filled slots leave state unchanged. Three warps are authored for the first beta (door/teleport pair plus a flag-gated door). Popup text, `Msg`/`TMsg`/`SMsg` and the `GiveItem` opcode exist only under the opt-in full VM profile; the generated-content guard rejects full-only content in default builds.

> **Current content status.** The authored first-beta area (`jp8.11.25`, closed 2026-10-19) is packed, not a test layout: every one of the 93 walkable cells in x8..23, y0..11 carries the encounter property, six active zone defs (chunks 1, 2, 33, 34, 65, 66) cover those cells with the five authored tables, the deposit box sits at (9,2), the plant plot at (14,2), and the warp pair plus flag-gated door close the area. A fresh boot can plant, deposit a lure, meet a wild encounter, rest and reach the fixed boss with no demo flags. The two scripted NPC markers remain placeholders; trainer dialogue stays outside the compact VM profile.

## Encounters

Wild battles trigger from world steps: after plants tick and lure decay, the step commit checks the destination tile. If the tile carries `PROP_ENCOUNTER`, a **10% roll** starts an encounter. Selection is deterministic per zone:

- Zone definitions are packed per chunk: base table, table count, a 4-way lure-type nibble map, and a signed level offset.
- An active lure zone selects a table by its type nibble (types 0–3 exposed); inert zones use the base table.
- A slot is rolled from 10 entries; the wild level is `clamp(floor(party average level) + zone offset, table floor, table ceil)`.
- The encounter catalog is **32 species** in shipping (ids 0–31); ids 32–63 are the opt-in expansion roster used by test builds only.

### First-beta tables <span class="tag cont">CONTRACT</span>

`data/beta-area.toml` pins exactly five 10-slot tables (base + spirit/water/wind/earth lure groups), levels 2–5, allowed species `[0, 3, 6, 9, 10, 12, 13, 14]`; `fxdata/data/encounters.txt` packs them in contract order behind six authored zone defs (lure nibbles 1–4, inert 0/255 sentinels). Placeholder species (21/22) and expansion ids are excluded.

| Table | Lure | Slots (species ids) |
|---|---|---|
| base | — | 0, 3, 6, 9, 10, 12, 13, 14, 0, 6 |
| spirit | type 0 | 9 × 6 heavy, 0 × 2, 6, 13, 3, 14 |
| water | type 1 | 6 × 6 heavy, 3, 9, 14, 0 |
| wind | type 2 | 0 × 4, 13 × 4, 9, 6 |
| earth | type 3 | 3 × 4, 14 × 4, 9, 6 |
