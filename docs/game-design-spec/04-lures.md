# 4. Lures, zones and gathering

## Lure items

Lures are the primary resource. Item id = `type × 3 + tier` (8 authored types × 3 tiers = 24 ids). Tier sets the zone charge and the gather rate:

| Tier | Name | Charge (steps) | Gather rate |
|---|---|---|---|
| 0 | crude | 16 | 1 / round |
| 1 | fine | 32 | 2 / round |
| 2 | pure | 64 | 4 / round |

Types: spirit, water, wind, earth, fire, lightning, plant, elder. Encounter tables distinguish only the four low groups (spirit / water / wind / earth); higher types are future area content.

## Depositing and zone life

- Face a deposit tile (`PROP_SPARE`), press A, pick a lure from the 5-row picker. One lure is consumed.
- A zone belongs to the chunk where it was deposited, carries the lure's type and tier, and burns 1 charge per committed world step.
- Leaving to a chunk without an authored zone clears the lure; expiry clears it and shows the "Lure Gone" feedback.
- While active, wild encounters on that chunk are **gatherable** and use the lure-type table.

## Gathering in battle

- Gatherable battles expose a **GATHER** action (wild only; trainer/boss battles refuse it).
- Each gather round adds the tier rate (1/2/4) to progress; the need is `clamp(8 + wildLevel / 2, 8, 24)` → levels 2–5 give 9–10.
- Fleeing is automatic at 6 gather rounds; a KO on either active pauses the countdown (progress is retained).
- Reaching the need captures the foe at its current HP into the collection; a full 64-slot store refuses losslessly.
- The renewable math (authoring contract): tier-1 = 2 progress × 6 rounds = 12 ≥ need 10, so a fresh player can capture without KO risk or first-battle luck.

> **Renewable loop status.** Plant-claim tiles currently grant a lure directly; the staged planting/harvest model is planned (`jp8.11.5`) and the authored plant source is generated but not yet read by firmware. See the Plants section.
