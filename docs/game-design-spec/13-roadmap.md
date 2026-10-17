# 13. Roadmap

## First beta — one complete saved area (`jp8.11`)

Shipped: authored beta start + preset, exact wild-win exp and bounded levels, material/heal reward mappings, evolution on the authored beta edges, lure tier rates through the real encounter path, gather commit to the collection, first-beta contract tests.

Remaining open: renewable planting/harvest (`.5`), four-row collection view (`.8`), party-slot replacement without healing (`.9`), fixed boss + persistent ending (`.13`), real-control loop proof (`.15`, blocked by map authoring), explicit SAVE/recoverable failure (`.23`), release gate (`.16`). Planning items added 2026-10-17: first-beta map authoring — encounters, deposits, warps, gating (`.25`); pacing and lure-economy numbers (`.26`); first-session onboarding without dialogue (`.27`); authored trainer-persona wiring or removal (`.28`).

## Growth rearch (`rhs7`, 2026-10-17)

1. Archive the legacy battle contract run (host suites + dormant legacy TUs).
2. Delete dead legacy arms and the dormant Move/Effect/PP surface from shared sources.
3. Measured stat collapse: retire five dead stat seeds to hp (≈ −320 B flash data, SRAM down).
4. Evolution as mask progression: full starter chains + target-kit adoption.

## Post-beta (`jp8.12`, P3)

- Fishing and fossil acquisition design; seven more themed areas; broader growth, shops and a larger arena.

## Long-term M0–M6 arc (historical contract)

M0 overworld spine → M0.5 lure prototype → M1a flash store + SaveFile v2 → M1b items → M2 gathering sources → M3 lure zones → M4 ownership/growth (32-box collection) → M5 fishing → M6 eight areas, boss gates, shops, arena. The first-beta epic is the current slice of that arc.

## Parallel workstreams

- **Resource maintenance** (`qu9`): enum-size audit, snapshot-buffer removal feasibility, budget reserve rules.
- **Audio feasibility** (`7v13`, P2): FX-streamed music prototype (~+3 KB flash class), gated on whole-image measurement and stack budget.
- **Asset disposition** (`u0ez`): remove 11 superseded legacy sprite sheets once fixtures are reworked.
- **Balance evidence** (`ogh`): held-out trials on live levers (e.g., Rock HP seed); power/speed trials closed as retired-engine no-ops.
