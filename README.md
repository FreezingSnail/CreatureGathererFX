# CreatureGathererFX

Creature collecting demake game for the Arduboy.

The game uses native `Arduboy2Base` 1bpp rendering at 52 fps. Each loop updates and renders once,
then presents the frame with `FX::display(CLEAR_BUFFER)`.

Workflow conventions distilled from past waves live in `docs/dev-flow.md`; agent instructions live
in `AGENTS.md`.

The first-beta first-session script and its minimal (no-dialogue) affordance set are pinned in
`docs/first-beta-onboarding-brief.md`.

## Generate and validate

Run `make gen` to generate game data, native sprite/font sources, and package the FX image with
`cgfx-tools`; it requires no Python bridge. `make gen-sprites` explicitly regenerates those
sprite/font sources through `cgfx-tools`. The opt-in `arena-demo` and `lure-demo` targets use
`gen-if-needed` to reuse generated outputs only when authored inputs, packed artifacts, generator
tooling, and generated-header mirrors still match; explicit `make gen` remains forceful.

Run `make check` for generation, host tests, and FX tests when [Ardens](https://github.com/tiberiusbrown/Ardens)
is available. `fxtest` delegates only to the `fxtest-headless` serial harness: every concrete
`tst/fxdatatest/*.ino` suite (including future save suites) must emit an exact `P` or `F` serial
marker. Set `ARDENS=/path/to/Ardens` to enable this device-test stage; without it, it reports a
skip. An Ardens binary without `captureserial` is blocked rather than treated as a pass (the
installed 0.24.4 bundle is incompatible). CI builds the pinned headless Ardens source with
`captureserial`; use that runner for device-suite execution rather than a graphical/manual Ardens
path.

`cgfx-tools` must be available on `PATH`. Run `make doctor` to verify the selected executable.

Run `make ram` to build the FX sketch and print stable flash/static-RAM totals, free static RAM, and
the 15 largest `.data`/`.bss` symbols. `RAM_STATIC_BYTES` comes from the linker section total and
is the static RAM budget figure; the symbol list attributes named records and can sum lower because
it omits unnamed linker bookkeeping. Override `RAM_ELF`, `AVR_SIZE`, or `AVR_NM` when inspecting a
specific ELF or using tools outside the Arduino AVR-GCC package.

`make build` and `make mini` enforce the physical application flash limit of 29,696 B and a 2,160 B
static RAM. `make check` runs the host-side parser tests and the FX build gate. Override
`AVR_FLASH_BUDGET` or `AVR_STATIC_RAM_BUDGET` to test a different ceiling. Each device-test ELF is
also checked against `FXTEST_RAM_BUDGET` (default 2,160 B `.data` + `.bss`) before Ardens runs;
that static allowance is separate from the painted `test_stack` headroom gate.

Shipping FX and Mini builds disable the USB/CDC application stack to recover flash and static RAM.
The device-test sketches keep the stock USB entry point because their exact serial `P`/`F` result
depends on it. Automatic uploader reset is unavailable while the game is running. To recover for
upload, connect USB, hold DOWN while resetting the Arduboy, then start the upload while the
bootloader is active; shipping startup also checks DOWN and transfers to the bootloader. If that
path does not enter the bootloader, double-tap reset and start the upload promptly.

## Music (AUDIO=1)

Music is off by default: `AUDIO=0` (the default) compiles no engine code and costs no flash or
static RAM, so shipping images are byte-identical with or without this feature present. Build the
soundtrack with `AUDIO=1 make build` or inspect it with `AUDIO=1 make ram`. `AUDIO=1 make
battle-test` starts the trainer demo with the combat theme. The runtime is FX-only;
`make mini AUDIO=1` fails intentionally, because the 11 theme streams live in the packed FX image
(`dist/fxdata.bin`, 42,584 B total, appended after the existing data; see `data/music/README.md`
and `fxlayout.toml`).

Theme routing is selected by game state (`src/engine/ModeState.cpp` and
`src/engine/battle/BattleFlow.cpp`):

| State | Theme |
| --- | --- |
| World / battle exit | `overworld` |
| Battle entry | `combat` |
| Win or Gathered result | `victory` cue, 104 frames (~2 s), then `overworld` |
| Lose or Draw result | `defeat` cue, 104 frames (~2 s), then `overworld` |

The remaining catalog entries (`opening`, `village`, `forest`, `coast`, `marsh`, `ruins`,
`exploration`) are packed for future areas; no area state selects them yet. `overworld` is the
current world theme everywhere. There are no sound effects.

Playback pauses (and mutes) while a save is written and resumes afterwards, so save timing is
unchanged. Music data reads use the separate audio read category and never consume the per-frame
metadata/render read budget. Compact streaming needs one foreground service call per frame after
`FX::display`; that call also counts down the bounded result cue.

Measured with Arduino CLI 1.2.0 / AVR-GCC 7.3.0-atmel3.6.1-arduino7, `-mrelax`, LTO:

| Build | Flash | Static RAM |
| --- | --- | --- |
| `AUDIO=0 make ram` (default) | 23,252 B | 1,665 B |
| `AUDIO=1 make ram` (pre-routing, 9ha9.3) | 27,204 B | 1,895 B |
| `AUDIO=1 make ram` (routing, current) | 27,524 B | 1,898 B |

So the whole soundtrack runtime is +4,272 B flash / +233 B static RAM over the default image, of
which state routing is +320 B flash / +3 B static. The theme table itself is read from PROGMEM on
each theme change, not every frame. The `AUDIO=1` device suite (`test_readcounter` +
`test_stack`) reports 0 underruns, 0 stream errors, a maximum service gap of 2, and 373 B painted
stack headroom.

## Lure world scenery (opt-in)

`make lure-demo ARDENS=/path/to/Ardens` uses the clear, sparse environment scenery
in the existing lure clearing.32px environment artwork is baked into16px render
cells; sparse white ground, connected foliage and the original16px player preserve
the current movement scale. The box, plant, collision and encounter/lure data stay
at their existing locations. Normal shipping world art is unchanged.
See [native previews and art notes](docs/world-environment-32-spike.html#lure-world).

## World script profile

Shipping world interactions use the compact `If`, `TpIf`, and `End` profile
for the three authored warps. Default builds select `CGFX_COMPACT_WARP_VM`: a
single forward pass over each 128-byte slot with big-endian operands and live
player-position checks. The build validator checks flag capacity, conditions,
branch encoding, forward jumps onto command boundaries, and map coordinates
before firmware compilation. Zero-filled slots leave gameplay state unchanged.
Default world interactions omit popup handling because this profile cannot queue
messages. Tactical battle AI, battle menus, and battle rendering use the same paths.

Replace `CGFX_COMPACT_WARP_VM` with `CGFX_FULL_WORLD_VM` in the shipping C++ flags to select the full world
interpreter and popup behavior. `ScriptVm::initVM/run` remain available to callers;
the full interpreter retains its runtime bounds checks. The full profile also
executes the generated `GiveItem` opcode (9) with three `kind,id,count` operands
into the one shared Player bag: kind 0 lures and kind 1 consumables add through
`item::inventoryAdd` (saturating at 99), kind 2 unlocks a Player key bit through
`item::keyItemUnlock` without an inventory row. The generated-content guard
rejects full-only commands or script text in a default build, including commands
inside skipped branches. Choose the full profile when authoring that content.

## Player artwork and walking

The world player uses masked16×16 chibi sprites facing up,right,down,left.
Each direction has an idle pose and two walking poses, selected from the existing
16-pixel movement step. White outlines separate the player from scenery;
transparent pixels preserve the map beneath. No additional animation state is
stored in RAM. The authored source is `images/Playerchibi_16x16.png`; `make gen`
packs it as the append-only `worldPlayerSprites` FX field.

## Trainer battle demo (opt-in)

Battles use the reviewed native48×48 front/back sprites and compact two-row menu by default.
Playback feedback uses white text on black. The compact HUD labels FOE (bar only) and YOU (current/max HP and bar), with
3×5 numbers and thin tracks above the feedback area.

After `make gen`, build the developer demo with the `CGFX_TRAINER_DEMO` flag. It starts a three-on-three
trainer battle with the named `opening` player and trainer preset, using the normal battle controls.
Add `-DCGFX_TRAINER_DEMO_SWITCH_DRILL` to choose the alternate `switch_drill` matchup. A reset
restarts the selected matchup; ordinary builds omit this bootstrap.
Add `-DCGFX_TRAINER_DEMO_UTILITY` with `CGFX_TRAINER_DEMO` to try Bell (Sharpen),
Rock (Ironbody), and Hedge (Rejuvenate/Pollen). Selected move PP appears at the top
of the move menu; `*` means unlimited. Battles still have one active creature per side.
To preview the gathering expansion roster, run `make battle-test BATTLE_TEST_EXPANSION=1`
with `ARDENS=/path/to/Ardens`. This isolated build uses creatures 32–63 for its player and trainer
parties; the normal encounter catalog and save format remain at 32 creatures.

```sh
make build BUILD_DIR=build/trainer-demo \
  AVR_SHIPPING_CPP_FLAGS='-mrelax -mcall-prologues -fno-move-loop-invariants -mstrict-X -DCGFX_SHIPPING_NO_USB -DCGFX_TRAINER_DEMO'
mkdir -p build/trainer-demo/isolated
cp -f dist/fxdata.bin build/trainer-demo/isolated/fxdata.bin
cp -f dist/fxdata-save.bin build/trainer-demo/isolated/fxdata-save.bin
/path/to/Ardens fxport=d1 display=ssd1306 \
  file=build/trainer-demo/CreatureGathererFX.ino.hex \
  file=build/trainer-demo/isolated/fxdata.bin \
  save=build/trainer-demo/isolated/fxdata-save.bin
```

The demo bypasses save loading and does not auto-save. The copied cart and save files keep this run
separate from any personal Ardens save. Press A to select and accelerate feedback, B to back out of
optional submenus, and the directions to navigate. Trainer battles refuse Gather and Escape.

## Arena demo (opt-in)

The arena demo launches the current opt-in **RPS duel** engine from the existing arena bootstrap:
`arena-demo` builds with the full shipping flags plus `-DCGFX_ARENA_DEMO` and
`-DCGFX_DUEL_BATTLE`. Team selection, isolated FX data/save launch, and all override knobs
(`ARENA_DEMO_BUILD_DIR`, `ARENA_DEMO_CART_DIR`, `ARENA_DEMO_CPP_FLAGS`, `ARENA_DEMO_FLASH_BUDGET`)
are unchanged. `make arena-test` is an alias for the same launch.

Build and launch the arena demo with:

```sh
make arena-demo ARDENS=/path/to/Ardens
# or the alias:
make arena-test ARDENS=/path/to/Ardens
```

To compile the arena+duel firmware without launching Ardens:

```sh
make ram BUILD_DIR=build \
  AVR_SHIPPING_CPP_FLAGS='-mrelax -DCGFX_SHIPPING_NO_USB -fno-inline-small-functions -DCGFX_BATTLE_NO_FACTS -DCGFX_BATTLE_COMPACT_RESULT -DCGFX_COMPACT_WARP_VM -DCGFX_ARENA_DEMO -DCGFX_DUEL_BATTLE' \
  AVR_FLASH_BUDGET=29184
```

To build the distributable Arduboy package (no Ardens required):

```sh
make arena-package
```

It builds the same arena firmware and writes `dist/CreatureGathererFX-ArenaDemo.arduboy`: a `zip`
holding `info.json` (schemaVersion 3), `CreatureGathererFX.ino.hex`, and `fxdata.bin`. The archive
is reproducible — fixed member order, normalised mtimes, and `zip -X`.

The combat itself is the duel: hidden simultaneous picks, the 5-type relation, 14/4/6 damage with
the x1.5/x0.5 creature-type matchup, switch/KO/draw, WAIT, and the fixture persona. The arena menus
are the current (pre-compact) screens; only the battle engine changed.

The target runs `make gen-if-needed`, builds into `build/arena-demo` using the opt-in
`CGFX_ARENA_DEMO` define and a 29,184-byte arena flash ceiling, then launches Ardens with isolated FX data and save
files. Override the output path, isolated cart path, or flags with `ARENA_DEMO_BUILD_DIR`,
`ARENA_DEMO_CART_DIR`, `ARENA_DEMO_CPP_FLAGS`, and `ARENA_DEMO_FLASH_BUDGET`. It requires Ardens
plus the normal `cgfx-tools`, Arduino CLI, and AVR size tools used by `make gen` and `make ram`.

The target launches Ardens with the split data and save images from the isolated cart directory.
To launch that already-built demo again without rebuilding, run:

```sh
mkdir -p build/arena-demo/isolated
cp -f dist/fxdata-data.bin build/arena-demo/isolated/fxdata-data.bin
cp -f dist/fxdata-save.bin build/arena-demo/isolated/fxdata-save.bin
/path/to/Ardens fxport=d1 display=ssd1306 \
  file=build/arena-demo/CreatureGathererFX.ino.hex \
  file=build/arena-demo/isolated/fxdata-data.bin \
  save=build/arena-demo/isolated/fxdata-save.bin
```

`dist/fxdata.bin` is the flashable FX cart image; Ardens takes its FX data and save regions as
separate inputs as shown above. The demo draws a random match every time: boot rolls one of the
three level-31 premade teams (Blitz, Bulwark, Utility) and one of the five premade opponents
(Starter, Speed, Fortress, Tricks, Champion) from the shared game RNG — seeded once in setup from
the hardware entropy source — and starts the duel immediately, with no team picker. After terminal
battle feedback, `arena::finishBattle` re-rolls both sides and starts the next match, so the demo
plays a continuous random 3v3. Every match starts with fresh HP, status, modifiers, and move uses.
The demo bypasses player save loading and writing and does not add arena records, rewards, or
progression.

Measured arena firmware: 23,382 B flash against the 29,184 B arena ceiling and 1,765 B static RAM
against the 2,160 B limit.

## Overworld wild-battle demo (opt-in)

Build with `CGFX_WILD_DEMO` instead of `CGFX_TRAINER_DEMO`. It applies the named `opening`
player preset, bypasses loading and writing a user save, and starts at the normal overworld entry
location. Move once with the directions to start a wild battle through the regular world-step,
encounter selection, `BattleSession`, and playback controller paths. The production map carries
encounter tiles across the authored first-beta area, so an ordinary boot already rolls wild
encounters; the demo's one-time first-step trigger stays as its opt-in shortcut. Reset to restart
the demo.

```sh
make build BUILD_DIR=build/wild-demo \
  AVR_SHIPPING_CPP_FLAGS='-mrelax -mcall-prologues -fno-move-loop-invariants -mstrict-X -DCGFX_SHIPPING_NO_USB -DCGFX_WILD_DEMO'
mkdir -p build/wild-demo/isolated
cp -f dist/fxdata.bin build/wild-demo/isolated/fxdata.bin
cp -f dist/fxdata-save.bin build/wild-demo/isolated/fxdata-save.bin
/path/to/Ardens fxport=d1 display=ssd1306 \
  file=build/wild-demo/CreatureGathererFX.ino.hex \
  file=build/wild-demo/isolated/fxdata.bin \
  save=build/wild-demo/isolated/fxdata-save.bin
```

Use the normal controls to attack, switch, wait through faint feedback, select Gather (refused in
this ordinary wild encounter), and escape. Battle HP returns to the player party when the battle
exits. The isolated save file prevents this run from using or updating a personal Ardens save.

## Tooling

CreatureGathererFX uses the native Rust `cgfx-tools` binary from
[CreatureGathererTools](../CreatureGathererTools). Its reusable core is
`CreatureGathererTools/crates/core` (package `cgfx-core`); the `cgfx-tools` binary runs the same
core without the desktop UI. The native pipeline replaces the retired standalone data converters
and Python FX-data bridge. A Python installation is not required for generation or packing.

### Tool installation

Install `cgfx-tools`, then expose its bin directory through `PATH`:

```sh
(cd ../CreatureGathererTools && ./install.sh)
export PATH="$HOME/Applications/CreatureGathererTools/bin:$PATH"
make doctor
```

`make` invokes `cgfx-tools` directly. A missing or incompatible executable is an error, not a
fallback to an old converter. CI downloads the locked release and adds it to `PATH` before generation.

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
relative to the layout file. The current layout has 20 `[[entry]]` tables: 13 file-level expansion
entries followed by the image and raw artifacts, plus a `[save]` table and two `[[save.entry]]`
tables. Save offsets must be sector-aligned and remain within the reserved region.

Most C-style data files use file-level expansion:

```toml
[[entry]]
source = { expand = { path = "fxdata/data/menu.txt" } }
```

`expand` restores the pre-migration include behavior: it reads every supported declaration from the
source file and makes each declaration an FX field. Its declaration/source order is the within-file
ordering ABI; fields are emitted in exact source order, so do not reorder declarations merely for
formatting. Later declarations can resolve offsets of earlier fields, and the final order is
mirrored in `src/fxdata.h`. Names and namespaces come from the C source (for example,
`namespace MenuFXData { ... }`), rather than from the TOML entry. Expansion accepts mixed
`uint8_t`, `uint24_t`, and `uint32_t` declarations: `uint8_t` byte initializers are packed directly,
while typed `uint24_t`/`uint32_t` initializers resolve symbolic offsets in declaration order.

An `expand` entry must have only its `source`: `name`, `namespace`, and `align` are forbidden because
the source declarations own those properties and their placement. Use `exclude` when one declaration
requires deliberate hand placement, then add a normal named entry for it; for example,
`source = { expand = { path = "fxdata/data/menu.txt", exclude = ["attackText"] } }` excludes the
real `MenuFXData::attackText` declaration from that file's expansion. The loader rejects unknown
keys or source kinds, sources with zero declarations, unterminated declarations or namespaces,
exclusions that do not name a source declaration, and duplicate qualified field names.

Unqualified symbolic initializers retain the legacy compatibility rule: they resolve to the **first**
declaration with that name, even when it is namespaced. Thus `MenuStrings`'s `attackText` refers to
`MenuFXData::attackText`. This is a permanent ABI rule, guarded by
`tst/generated/generated_libs_test.cpp`; do not replace it with last-declaration or scope-dependent
resolution.

Supported native sources, including the concrete entries that remain in the 20-entry layout:

- `raw = "path"` copies a binary file byte-for-byte.
- `carray = { path = "..." }` extracts compatible C-array bytes; adding `symbol = "..."` selects
  one named `uint8_t` array from a legacy-compatible source such as `fxdata/Sprites.txt`.
- `symbol = { path = "...", symbol = "..." }` packs one typed `uint8_t`, `uint24_t`, or `uint32_t`
  C declaration and resolves its symbolic initializers against earlier layout fields. It remains for
  explicit, hand-placed declarations; its optional TOML `namespace` becomes a qualified header name.
- `image = { path = "...", width = ..., height = ..., shades = ..., spacing = ... }` uses the
  native sprite encoder for a PNG sheet and emits image dimensions, frame data, and field metadata.
  The current layout uses it for `images/ArduFontTrimmed_5x6.png` as `fontTrimmed`.
- `builder = "key"` is available to `cgfx-core` for bytes supplied by a project builder; the current
  layout does not use it.

Sprite and font conversion is native too. `fxsprites.toml` lists ordered PNG sprite sets and writes
legacy-compatible `fxdata/Sprites.txt` C arrays. Its `[strings]` section reads
`data/text/strings.txt`, renders strings from the configured font atlas, and writes
`fxdata/generated/Sprites.txt`; the corresponding `expand` entries preserve their pre-migration
include behavior while removing the Python conversion step.

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

For this project, `[save]` reserves eight sectors (`32768` bytes). `save_main` starts at offset `0`,
`save_log` at `4096`, and the two store sectors at `8192` and `12288`. Four named reserved sectors
occupy offsets `16384` through `28672`. The combined device image carries this region after the
data image. Keep save offsets stable: changing them changes the generated header and the on-device
format.

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

`make gen` and `make pack` invoke `cgfx-tools` from `PATH`. To use a specific installation,
place its bin directory first in `PATH`:

```sh
PATH=/absolute/path/to/bin:$PATH make gen
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
resulting headless binary, runs `make gen` with its pinned `cgfx-tools` installed on `PATH`, then
gates the job on `make fxtest-headless`. Use that headless serial runner for device-suite execution;
do not substitute a graphical/manual Ardens path.
