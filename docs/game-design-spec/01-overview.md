# 1. Overview, pillars, non-goals

CreatureGathererFX is a Pokémon-style creature collecting demake compressed onto a 128×64 monochrome screen with roughly 23 KB of usable program flash. The design deliberately trades depth for readability: five creature types in a complete rock-paper-scissors web, one flat damage table, hidden simultaneous picks, and a single persistent save on the FX cartridge. Gathering is the second core verb: lure zones turn ordinary wild battles into capture ("gather") encounters with a progress clock.

## Design pillars

- **Legible at 128×64.** Every battle decision must be explainable by the type chart and the pick row; no hidden damage formula, no PP ledgers, no stat-stage homework.
- **One flat table.** Damage is 14 win / 4 chip / 6 trade, scaled only by type matchup (×1.5 / ×0.5).
- **Gather, don't just fight.** Lures create dedicated zones; gatherable wild battles bank progress toward capture instead of a knockout.
- **Persistent collection.** 64 stable instance slots on cartridge flash, journaled and compacted against power loss, each instance carrying its own species, experience, HP and attack kit.
- **Authored data, validated in host tools.** Content constraints (evolution edges, encounter tables, script profiles) fail generation or tests, not the device.

## Engineering priorities

- Correctness of requested outcomes and mechanics comes first; measured smallest firmware second.
- Whole-image flash/RAM measurement decides; per-symbol sizes are only leads.
- Metadata reads (FX/OLED share SPI) happen on transitions only.
- Trust authored/internal invariants; no runtime defenses for impossible states.
- Tests prove mechanics and hardware outcomes, live permanently beside the code.

## Non-goals

- No audio yet (feasibility epic `7v13` only).
- No multiplayer/link features.
- No stat spreads, statuses, PP, accuracy or turn order — removed with the duel <span class="tag ret">RETIRED</span>.
- No animation framework; battles are staged single-frame updates with timed phases.
