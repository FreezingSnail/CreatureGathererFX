# CreatureGathererFX — session reports (active wave)

The completed first-beta implementation and v0.3.0 release wave is archived at
`docs/reports-archive/2026-11-16-first-beta.md`. It contains the per-bead evidence, the review/
repair cycle, the release pin and the final gate.

## 2026-11-16 first-beta closeout

- Implementation commits: `c1c5019` (agent roles/reviewer contract), `f9af724` (first-beta slice +
  cgfx-tools v0.3.0 pin).
- Final gate PASS on the pinned release: `build/reports/rhs7/final-gate-jp8-11-16-v030/` —
  27,520 B flash / 1,782 B static / 622 B stack; all device suites pass.
- Epic `CreatureGathererFX-jp8.11` closed; only pre-existing art studies/scratch generators remain
  untracked.

## lckq — process hardening

Added `docs/release-checklist.md`; `make release-check` (doctor/gen/verify-generated/manifest/
generated-libs/parity/spec); `make fxtest-compile-all`; `tools/suite-report.sh` + `make suite-report`;
release/shared-test/reporting rules in `docs/dev-flow.md`, `docs/agent-roles.md` and `AGENTS.md`;
rotated the prior 832-line report to `docs/reports-archive/2026-11-16-first-beta.md`; set
`core.hooksPath=.beads/hooks` (pre-commit executable). `make fxtest-compile-all` compiled all suites;
`make release-check` PASS with the pinned `0.3.0+gfe328be2` binary; `make suite-report` reports
35/35 device suites, 0 failures, stack 622 B, shipping 27,520/1,782/778. No shipping change.

## 5wxa — menu/collection stack proof

Extended `tst/fxdatatest/stack_test.hpp` to paint the bag page, collection root/list/party page,
swap apply and the duel Heal page through the shipping menu seams. Menu-chain low-water headroom
**271 B** (>=219, below the 300 B target); save chain 359 B, mode 439 B. Test-stack suite flash
27,086 (2,610 free), static 1,998/2,160 (162 free); shipping unchanged 27,520/1,782/622. The suite
now fails if the menu chain or save chain drops below 219 B. `make test` and the focused
`fxtest-spike test_stack` pass; full compile-all is green.

## wc89 — align the world player sprite with its map tile

The idle sprite was drawn at screen `(56,24)` while the player's map cell (viewport tile 3,2) was
drawn at `(48,32)`, so the 16x16 sprite straddled four tiles by 8 px on each axis. `draw.h` now
defines the shared `PLAYER_VIEW_TILE_X/Y` anchor and uses it for both the map viewport and
`PLAYER_DRAW_X/Y`; the sprite occupies its own cell at `(48,32)`. The focused player pixel proof
now expects pages 4-5 / columns 48-63 and rejects pixels outside that cell.

Focused `test_world_player` 60/0; final gate PASS in
`build/reports/rhs7/final-gate-wc89/`; shipping unchanged at 27,520 B flash / 1,782 B static /
622 B stack. The camera remains tile-anchored at viewport cell 3,2 rather than half-tile centered;
if the owner wants the sprite centered, that is a separate camera/window change.

## nsv1 — 8-step plant growth in `make dev`

`make dev` now passes `AVR_SHIPPING_CPP_FLAGS="$(DEV_CPP_FLAGS)"`, where
`DEV_CPP_FLAGS = shipping flags + -DCGFX_DEV`; `PlantGameState::GROWTH_STEPS` is 8 under
`CGFX_DEV` and 128 otherwise. Shipping, `run`, build, mini, demos and device-test profiles are
unchanged. The host/device growth-boundary tests now derive their tick counts from
`GROWTH_STEPS`, so the default 128 and the dev 8 both validate the same code.

Default `make test`: 244,009 passed / 0 failed; `CGFX_DEV` host run: 244,007 passed / 0 failed;
dev AVR build succeeded with `-DCGFX_DEV`. Final gate PASS in
`build/reports/rhs7/final-gate-nsv1/`; 35/35 device suites; shipping unchanged at
27,520 B flash / 1,782 B static / 622 B stack.
