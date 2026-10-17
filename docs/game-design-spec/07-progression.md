# 7. Progression: levels, evolution and stats

## Experience and levels

- Curve: cumulative exp for level *L* is *L³* (level 1 = 1 exp), stored as a 32-entry u16 table; cap 29,791 at level 31. Level is derived from exp — it is never stored.
- Gain: `8 × foe level` per ordinary wild win, to every living occupied party member, saturating at the level-31 threshold.
- Level-up effect: max HP is recomputed from species seed and level; a living member keeps its missing HP (HP grows by the max-HP delta, capped at the new maximum).

## Stats model <span class="tag plan">PLANNED</span>

Authoring currently carries six stat seeds per species (atk, def, spcAtk, spcDef, hp, speed), but the duel reads only **hp**: max HP = `2×level + hpSeed×(level/3) + 30`. The five other seeds are unread in shipping and their trials were measured inert (e.g., a special-defense seed change moved 0/25 matchup pairs). Owner decision (2026-10-17): retire the five dead seeds, keep HP as the only stat, and make the **attack mask the progression/customization axis** — evolution unlocks types now, item-driven custom kits later. One attack-scaling seed and a per-level learnset table remain parked options (see the retired/open section).

## Evolution

- Edges are explicit authored `from → to @ level` mappings; id adjacency is never inferred. The generator validates duplicates, self/missing targets, thresholds, kit presence and cycles before packing.
- Live beta edges: `0→1`, `3→4`, `6→7`, each at level 8 (the starter trio).
- On the same wild-win terminal edge as exp: an instance at or over its edge threshold replaces its species in place, preserving exp, stable slot and current mask, and recomputing types/HP at the unchanged derived level; a living member keeps its missing HP.
- A refused save write leaves the old species in place — the evolution stays derivable from exp and retries on the next eligible transition. The collection path can evolve boxed instances the same way.
- <span class="tag plan">PLANNED</span> Growth rearch (`rhs7`): author the full starter chains (`1→2`, `4→5`, `7→8` at level 16) and adopt the target species' kit on evolution, which restores the "instance kit == species kit" invariant and turns stage transitions into type unlocks (crab gains fire at 8; snail gains plant and squid gains fire at 16).
