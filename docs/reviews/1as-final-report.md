# Remaining firmware simplification spikes: final research checkpoint

All 17 `CreatureGathererFX-1as.*` child beads and P0 `CreatureGathererFX-8fu` are closed with per-item reports and isolated patches under `/private/tmp/cgfx-1as-spike/artifacts`. This closes the research epic; it does not adopt the candidate or claim a passing full gate.

The fresh frozen shipping control measured **23,488 B flash / 1,747 B static SRAM**. P0 `.8fu` corrects the SaveController flag-array extent and contributes −14 B flash / 0 static. The accepted 1as chain adds −1,474 B / 0 static. Final retained candidate: **22,000 B flash / 1,662 B static**, **−1,488 B flash / −85 B static** against the frozen control; flash free is **7,696 B** of 29,696, static free is 898 B of 2,560 (498 B under the project's 2,160 B static build ceiling).

| Item | Disposition | Independent flash/static delta | Cumulative flash/static delta |
|---|---|---:|---:|
| 1as.1 narrow damage arithmetic | Retain | −88 / 0 B | −88 / 0 B |
| 1as.2 omit unused shipping consequence writes | Retain | −174 / 0 B | −174 / 0 B |
| 1as.3 remove unused flash page-write member | Retain | −76 / 0 B | −76 / 0 B |
| 1as.4 remove duplicate post-state captures | Retain | −8 / 0 B | −8 / 0 B |
| 1as.5 remove NEGATIVE raster mode | Reject | 0 / 0 B | 0 / 0 B |
| 1as.6 perform journal initialization at boot replay | Retain | −240 / 0 B | −240 / 0 B |
| 1as.7 stream checkpoint checksum and validation | Retain | −6 / 0 B | −6 / 0 B |
| 1as.8 carry AI scan results | Reject | +44 / 0 B | +52 / 0 B |
| 1as.9 omit dialog queue in compact profile | Retain | −90 / −85 B | −90 / −85 B |
| 1as.10 compact shipping ActionResult | Retain | −404 / 0 B | −392 / 0 B |
| 1as.11 one-pass compact warp VM | Retain | −254 / 0 B | −262 / 0 B |
| 1as.12 narrow tile property cache | Reject | +28 / −1 B | +24 / −22 B |
| 1as.13 validate encounters and remove duplicate select scans | Retain | −34 / 0 B | −34 / 0 B |
| 1as.14 advance packed plant stages byte-wise | Retain | −56 / 0 B | −56 / 0 B |
| 1as.15 specialize AVR snapshot encoding | Reject | +42 / 0 B | +38 / 0 B |
| 1as.16 remove redundant journal blank decode check | Retain | −16 / 0 B | −34 / 0 B |
| 1as.17 carry validated PP into attack spending | Retain | −24 / 0 B | −14 / 0 B |

The cumulative shipping path uses `CGFX_BATTLE_NO_FACTS`, `CGFX_BATTLE_COMPACT_RESULT` (which requires no-facts), and `CGFX_COMPACT_WARP_VM` with the established shipping flags. The compact tile-property variant is rejected and excluded. If the retained chain is later considered for adoption, review/apply only the retained cumulative patches, keep those profile flags wired explicitly, and validate the source/packed encounter checks from `.13`. No production source was changed during this work.

Known blockers remain separate from the research results: the paired stack suite still fails `save compaction completes got=2 want=12` (3/1), despite 525–547 B painted headroom in `.17` tests; stale legacy `test_save` API expectations and 1x8 battle-session expectations also remain. No full-gate pass is claimed. The corrected P0 `.8fu` device/native evidence and Sol approval are in `8fu-report.md`; save-wire and compact VM/result changes have their individual Sol approvals in their reports/review notes. Rejected variants remain available as evidence but are not in the candidate.

Artifacts: all per-spike reports, independent/cumulative patches, and selected ELFs are in `/private/tmp/cgfx-1as-spike/artifacts`; this final checkpoint does not supersede their exact commands, hashes, and test results.
