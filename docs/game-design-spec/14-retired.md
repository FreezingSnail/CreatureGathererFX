# 14. Retired systems, decision log and open questions

## Retired systems

| Item | Status | Notes |
|---|---|---|
| Ordered-turn battle engine (PP, accuracy, stat stages, statuses, effects, turn order, damage formula) | <span class="tag ret">RETIRED</span> | Source archived under `archive/battle-legacy/`; host-only legacy suites keep it compiling today — being archived to `archive/` in the rearch wave |
| Packed `Move`/`MoveBitSet`/`Effect` movesets; `moves.json`; move power | <span class="tag ret">RETIRED</span> | Replaced by the 5-bit type mask; leftover types are being cleaned up (`rhs7.2`) |
| Five secondary stat seeds (atk/def/spcAtk/spcDef/spd) | <span class="tag ret">RETIRED</span> <span class="tag plan">PLAN rhs7.3</span> | Provably unread in the duel; measured inert in trials |
| Stances variant (4-type cycle demo) | <span class="tag ret">RETIRED</span> | Mock-only spelling; not shipped |
| Superseded sprite sheets (creature/field/menu/fonts/letters) | <span class="tag ret">RETIRED</span> | 11 sheets satisfied-broken; removal pending `u0ez` |
| Power/speed balance trials (`ogh.10/.11/.12/.13`) | <span class="tag ret">RETIRED</span> | Closed 2026-10-17 — targets no longer exist in the engine |

## Decision log

| Date | Decision |
|---|---|
| 2026-10-10 | Player-choice resource checkpoint recorded (23,488 B flash / 1,747 B static / 509 B stack headroom); budget rules formalized. |
| 2026-10-15 | Duel brief + 5-bit mask contract: five types, hidden simultaneous picks, flat 14/4/6; movesets collapse to a type mask; save keeps an 8-byte record with a 1-byte mask. |
| 2026-10-16 | Legacy ordered-turn engine and its batch simulator retired; duel parity kernel + mock become the balance toolchain; fxtest device gate rebuilt (persistent per-profile stages, full wildcard ~75 s). |
| 2026-10-17 | Post-duel growth rearch: stats retire to HP; the mask is the progression axis; evolution adopts the target kit and extends to full starter chains; legacy contract run archived; per-level learnset and an attack seed parked (`docs/duel-growth-rearch-brief.md`, epic `rhs7`). |
| 2026-10-17 | Spec review vs open beads: four planning holes filed (`jp8.11.25`–`.28`); release/full-64 policy noted on `jp8.11.8`; this document split into editable markdown sources. |

## Open questions

- **Per-level learnset table** — only if intra-stage type unlocks are wanted; evolution-only progression was chosen for now (authoring format ~3 B/species packed).
- **One attack seed** — scaling the flat 14/4/6 by ±1–2 if HP-only fights feel flat; requires AI/mock/parity updates and a full balance re-baseline.
- **Zero-win compositions** — the paired 3v3 sweep still shows 7/25 fully-decided matchups (mono/single-kit comps); an owner decision on composition rules (e.g., team-building constraints) rather than stat tweaks.
- **Type skew** — water is ~42% of attacks in the sweep; encounter-table and mask authoring can rebalance availability without engine changes.
- **Boss behavior** — species 31 ardu at level 8, trainer rules, victory flag `flag_beta_complete`; exact trigger/ending presentation owned by `jp8.11.13`.
- **Items in battle** — heal/cure/charge data exists; the picker page and action wiring are open (`jp8.5.19`).
- **Release/full-64 policy** — `storeRemove` has no caller; `jp8.11.8` carries the freeze note (release action or explicit no-release-for-beta).
