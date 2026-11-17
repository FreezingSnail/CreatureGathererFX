# Generator release checklist

Use this only for a **generator/release pin**. A playable slice does not require a new
`cgfx-tools` release; keep release engineering in its own bounded bead and timebox it.

## Before tagging

1. Select the exact clean source revision. Do not tag a dirty tree or a revision whose committed
   model predates the game's authored schema. If the current artifacts came from a local build,
   prove a clean build of the selected revision can parse this repository first.
2. In a clean worktree of that revision, confirm the package version and run:
   `cargo test --workspace` (or the selected scope) and
   `cargo build --locked --release -p cgfx-core --bin cgfx-tools`.
3. Record the local binary's `--version` and SHA-256. Do not publish a locally built asset as a
   release asset; release provenance must come from the release workflow or an explicitly approved
   one-off.

## Publish

1. Tag the selected commit and push the tag. Confirm the release workflow's macOS runner is a
   supported arm64 image, and that the publish job checks out the repository before creating the
   release (the v0.3.0 workflow needed both fixes).
2. Wait for both `cgfx-tools-macos-arm64` and `cgfx-tools-linux-x64` assets plus `.sha256` files.
   Verify the macOS asset's checksum and `--version`; record the release URL, tag commit and both
   checksums. A skipped publish job is not a release.

## Pin and verify in the game

1. Update `tools/toolchain.lock` (`RELEASE_TAG` + both SHA-256 values), CI download paths, the
   documented Ardens path and the design-spec tool identity. Regenerate the spec HTML.
2. Put the released binary first on `PATH` and run `make release-check`. It covers doctor, clean
   generation, generated-artifact checks, manifest, generated libraries, packed parity and spec
   sync. If the release identity changes, regenerate with it and deliberately rebaseline packed
   parity; do not hand-edit generated files.
3. Run the required device checkpoint (`make fxtest-headless` for the full suite set, or the
   documented CI-equivalent job). A local gate alone does not prove release reproducibility.
4. Retain the release URL, commit, checksums, `make release-check` output and device evidence.

## Stop rule

If the release is blocked (runner, provenance, checksum, or clean-generation mismatch), do not block
the playable slice on it. Close the slice on its green local gate, keep a separate release bead with
the exact blockage and evidence, and return to it only when the release path is actionable.
