# 9. Items, economy and plants

## Items

| Item class | Ids / capacity | Behavior |
|---|---|---|
| Lures | 24 ids (`type×3 + tier`) | Deposited in world zones; consumed on deposit; gather rate by tier |
| Consumables | 8 ids | tonic/salve/elixir (heal 16/32/64), remedy/herb/nectar (cure), essence/crystal (charge) — battle item UI planned |
| Materials | derived per species | Ordinary wild KO drops the species' authored material (one tier-2 lure of its folded type group, count 1) exactly once; saturates at 99 |
| Key items | 8 bits, byte-packed | Unlocked by full-VM `GiveItem`; separate namespace from world flags |

- The bag is one Player-owned inventory (24 lure counts + 8 consumable counts, saturating at 99) and is the same instance used by world deposits and battle.
- First-beta heal rewards: item 0/1, count 1 each, awarded on ordinary wild wins in the area.
- The bag UI contract is a non-wrapping 4-row list; the world menu (BAG / COLLECTION / SAVE) is <span class="tag plan">PLANNED</span> via `jp8.11.23` (menu/SAVE) and `jp8.11.8` (collection view); the battle item page is `jp8.5.19`.

## Plants

- Model: 32 plant indices, each with a 2-bit stage (0–3) packed into 8 bytes; two plant pairs of (3-bit type + active flag) per byte.
- Growth: a 128-step world ticker increments every stage (mod 4) for each committed step; the ticker and stage bytes persist in the save snapshot.
- Planned staged loop (`jp8.11.5`): face a plantable tile, A plants free (stage 1); stage 3 (mature) + A grants one tier-1 lure and resets to stage 1; maturity is held rather than overflowing a full bag.
- Today the plant-claim tile grants one lure directly as a placeholder, and the authored plant source (type 0 / tier 1) is generated but not yet read by firmware.
