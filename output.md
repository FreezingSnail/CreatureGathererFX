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
