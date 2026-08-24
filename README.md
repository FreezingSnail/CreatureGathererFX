# CreatureGathererFX

Creature collecting demake game for the Arduboy.

## Generate and validate

Run `make gen` to generate game data, native sprite/font sources, and package the FX image with
`cgfx-tools`; it requires no Python bridge. `make gen-sprites` explicitly regenerates those
sprite/font sources through `cgfx-tools`.

Run `make check` for generation, host tests, and FX tests when [Ardens](https://github.com/tiberiusbrown/Ardens)
is available. `fxtest` delegates only to the `fxtest-headless` serial harness: every concrete
`tst/fxdatatest/*.ino` suite (including future save suites) must emit an exact `P` or `F` serial
marker. Set `ARDENS=/path/to/Ardens` to enable this device-test stage; without it, it reports a
skip. An Ardens binary without `captureserial` is blocked rather than treated as a pass (the
installed 0.24.4 bundle is incompatible). CI builds the pinned headless Ardens source with
`captureserial`; use that runner for device-suite execution rather than a graphical/manual Ardens
path.

`tools/cgfx-tools.sh` resolves the data generator. It uses an executable set through
`CGFX_TOOLS_BIN`, otherwise builds a sibling `../CreatureGathererTools` checkout, otherwise
downloads the locked and checksummed release. Run `./tools/cgfx-tools.sh` to inspect the selected
executable.

## Tooling

CreatureGathererFX uses the native Rust `cgfx-tools` binary from the sibling
[CreatureGathererTools](../CreatureGathererTools) repository. Its reusable core is
`CreatureGathererTools/crates/core` (package `cgfx-core`); the `cgfx-tools` binary runs the same
core without the desktop UI. The native pipeline replaces the retired standalone data converters
and Python FX-data bridge. A Python installation is not required for generation or packing.

### Tool resolution

Always invoke the resolver instead of assuming a binary location:

```sh
./tools/cgfx-tools.sh
```

Resolution order:

1. If `CGFX_TOOLS_BIN` is set, it must name an executable file; the resolver returns its absolute path.
2. If `../CreatureGathererTools` exists, the resolver builds
   `cargo build --release -p cgfx-core --bin cgfx-tools` there and uses `target/release/cgfx-tools`.
   This local checkout is intentionally unpinned and is useful during tool development.
3. Otherwise, the resolver downloads the host asset for the locked release in `tools/toolchain.lock`,
   verifies its SHA-256 checksum, caches it under `.cache/cgfx-tools/`, and returns the cache path.
   The lock currently defines macOS arm64 and Linux x64 assets; unsupported hosts fail.

A failed resolution is an error, not a fallback to an old converter. For a local tool, either export
`CGFX_TOOLS_BIN=/absolute/path/to/cgfx-tools` or keep the sibling checkout at
`../CreatureGathererTools`. CI sets the locked tool explicitly where reproducibility matters.

### Native generation and pack pipeline

`make gen` performs the complete project pipeline:

1. `cgfx-tools --project cgfx-project.json` reads the canonical JSON/asset inputs and writes
   generated creature, move, map/script, team, fixture, and related headers/binaries under
   `fxdata/generated/` (plus device-test fixtures under `tst/fxdatatest/generated/`).
2. Native CSV modes build the remaining generated inputs:
   `--arena-csv data/arena.csv --arena-output fxdata/generated` and
   `--type-table-csv data/typetable.csv --type-table-output fxdata/generated`. The step then copies
   the generated headers the firmware compiles into place: `opcodes.hpp` → `src/vm/`, and
   `flags.hpp` / `flag_bit_array.{hpp,cpp}` → `src/flags/`. These were hand copies before, which is
   how a stale opcode table (missing `SMsg = 2`) and stale flag ids once shipped unnoticed.
3. `cgfx-tools --project cgfx-project.json --emit-fixtures` and the version-stamp helper refresh
   generated device fixtures and `tool_version.bin`.
4. `make gen-sprites` runs `cgfx-tools --sprite-config fxsprites.toml` to regenerate
   native-compatible sprite/font C sources.
5. The pack step stages the project, then runs
   `cgfx-tools --project ... --pack --layout ...`. It writes `src/fxdata.h` and the FX image
   artifacts in `dist/`.

The generation inputs remain repository-relative. Do not hand-edit generated files; change the
JSON/CSV/PNG/TOML source and run the appropriate target. `make gen` includes `gen-sprites`; use
`make gen-sprites` when only sprite/font source regeneration is needed.

### `fxlayout.toml`

`fxlayout.toml` is an ordered flash layout, not a general-purpose build script. Paths resolve
relative to the layout file. It contains:

- Repeated `[[entry]]` tables. Each entry has a `name`, optional `namespace`, optional `align`, and
  one `source`. Declaration order is significant: later typed symbols can refer to offsets of
  earlier entries, and the resulting order is mirrored in `src/fxdata.h`.
- A `[save]` table with the reserved save-sector count.
- Repeated `[[save.entry]]` tables naming save-region offsets. Save offsets must be sector-aligned
  and remain within the reserved region.

Supported native entry sources:

- `raw = "path"` copies a binary file byte-for-byte.
- `carray = { path = "..." }` extracts all compatible C-array bytes; adding `symbol = "..."`
  selects one named `uint8_t` array from a legacy-compatible generated source such as `Sprites.txt`.
- `symbol = { path = "...", symbol = "..." }` parses a typed C declaration (`uint8_t`,
  `uint24_t`, or `uint32_t`) and resolves symbolic initializers against earlier layout fields.
  Namespaces become qualified header names while preserving the first unqualified alias behavior
  required by the game.
- `image = { path = "...", width = ..., height = ..., shades = ..., spacing = ... }` uses
  the native sprite encoder for a PNG sheet and emits its image dimensions, frame data, and field
  metadata. This is how the `fontTrimmed` image is packed.
- `builder = "key"` is available to `cgfx-core` for bytes supplied by a project builder; the
  current project layout uses generated files, C arrays, typed symbols, and images instead.

Sprite and font conversion is native too. `fxsprites.toml` lists ordered PNG sprite sets and writes
legacy-compatible `fxdata/Sprites.txt` C arrays. Its `[strings]` section reads
`data/text/strings.txt`, renders strings from the configured font atlas, and writes
`fxdata/generated/Sprites.txt`; this preserves existing symbols while removing the Python
conversion step. `fxlayout.toml` then selects those sprite/font symbols through `carray` or packs
a PNG font sheet through `image`.

### Packed artifacts and provenance

The packer emits:

- `src/fxdata.h`: generated field offsets, namespaces, image metadata, data-page constants, and
  save-region constants consumed by the sketch.
- `dist/fxdata-data.bin`: unpadded data payload.
- `dist/fxdata-save.bin`: reserved save payload, erased/padded to the configured save sectors when
  `[save]` is present.
- `dist/fxdata.bin`: device/development image containing page-padded data followed by the
  page/sector-padded save region.
- `fxdata/generated/manifest.json`: deterministic provenance record for the layout target,
  `cgfx-tools` version, discovered input/output paths, and SHA-256 checksums.

For this project, `[save]` reserves two sectors. `save_main` starts at offset `0` and `save_log` at
offset `4096`; the generated save image is therefore two sectors (`8192` bytes), and the combined
device image carries that region after the data image. Keep save offsets stable: changing them
changes the generated header and the on-device format.

`make pack` regenerates the packed artifacts. `make verify-generated` is non-mutating: it checks the
manifest against current inputs and generated outputs (and checks the image when present), rejecting
changed, missing, unexpected, or unrecorded artifacts with a `make gen` remedy. `make test-manifest`
runs permanent provenance fixtures. `make test-pack-parity` repacks with native `cgfx-tools` and
compares `dist/fxdata.bin` with the committed SHA-256 baseline.

### Generated-library tests

`make test-generated-libs` asserts that the generated libraries, the published header, and the
packed image agree. Two parts:

- `tst/generated/generated_libs_test.cpp` walks every `fxlayout.toml` entry and checks that its
  source bytes sit in `dist/fxdata-data.bin` at exactly the address `src/fxdata.h` publishes:
  `carray` initializers literally, typed `symbol` tables big-endian (the order
  `FX::readIndexedUInt24` reassembles), `raw` entries byte-for-byte. It decodes the sources with its
  own parser rather than reusing the packer's encoders, so a codegen or packer regression shows up
  as a mismatch instead of two implementations agreeing on the same mistake. It also mirrors the
  packer's legacy rule that an unqualified reference resolves to the **first** declaration of that
  name even when that declaration is namespaced — which is why the `MenuStrings` table's
  `attackText` means `MenuFXData::attackText`. Image entries are covered by the pack-parity SHA
  baseline instead.
- `tools/tests/generated-libs_test.sh` gates generated headers against their committed `src/`
  copies and checks the script text block's little-endian count/offset/length framing inside the
  packed image.

On the device side, `tst/fxdatatest/test_tables.ino` pins the FX read path itself: `uint24_t`
address tables read back as the addresses `fxdata.h` publishes (covering entries above `0x010000`),
indexed byte reads agreeing with `readDataBytes`, the text block decoding little-endian through
`ReadFXu16`, and the ArduboyFX `readIndexedUInt32` three-byte-stride defect
(`seekDataArray(address, index, 0, sizeof(uint24_t))`), so a library upgrade that changes it fails
loudly.

When writing new FX device assertions, never pass a `__uint24` through `uint32_t`: AVR-GCC leaves
the fourth byte undefined and the garbage looks exactly like a byte-order bug. Compare at 24 bits,
or `memcpy` the three bytes out of the object.

### Command reference

```sh
make gen                 # data + native sprites/fonts + fixtures + packed FX image
make gen-sprites         # native sprite/font C sources only
make verify-generated    # verify generated artifacts and provenance; no regeneration
make test-manifest       # manifest/provenance contract fixtures
make test-generated-libs # generated libs vs published header vs packed image
make test-pack-parity   # native pack byte-parity against the committed baseline
make test                # host C++ tests
make testvm              # ScriptVM host tests
make check               # gen, host tests, VM tests, manifest, generated libs, optional fxtest
```

`make gen` and `make pack` resolve `cgfx-tools` through `tools/cgfx-tools.sh`. To force a known
executable in a reproducible invocation:

```sh
CGFX_TOOLS_BIN=/absolute/path/to/cgfx-tools make gen
```

### Strict headless FX tests

`make fxtest` is an alias for `make fxtest-headless`; it never launches graphical Ardens. With
`ARDENS` unset, the target reports a skip. With `ARDENS` set, preflight requires an executable, a
generated FX image, and the `captureserial` capability. A binary that lacks `captureserial` is
**blocked**, not passed; the installed 0.24.4 bundle is therefore incompatible.

The harness compiles every `tst/fxdatatest/*.ino` suite, runs each through
`ARDENS captureserial=... fxport=d1 display=ssd1306`, and accepts only an exact normalized serial
`P` or `F` marker. Empty output, a failing marker, a non-zero Ardens exit, or any other output is
failure. This applies to existing and future suites, including save tests.

The CI workflow builds the pinned Ardens source with SDL/player/flashcart disabled, verifies the
resulting headless binary, runs `CGFX_TOOLS_BIN="$(./tools/cgfx-tools.sh)" make gen`, then gates the
job on `make fxtest-headless`. Use that headless serial runner for device-suite execution; do not
substitute a graphical/manual Ardens path.
