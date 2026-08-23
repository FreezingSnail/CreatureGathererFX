# CreatureGathererFX

Creature collecting demake game for the Arduboy.

## Generate and validate

Run `make gen` to generate game data and package the FX image with `cgfx-tools`. The current layout still contains `source = { legacy = ... }` entries; cgfx-tools 0.2.0 resolves those through its transitional `fxdata-build.py` bridge, so Python 3 and the vendored bridge input remain required until native symbol builders replace those entries.

Run `make check` for generation, host tests, and FX tests when [Ardens](https://github.com/tiberiusbrown/Ardens) is available. Set `ARDENS=/path/to/Ardens` to enable the device-test stage; without it, `fxtest` reports a skip.

`tools/cgfx-tools.sh` resolves the data generator. It uses an executable set through `CGFX_TOOLS_BIN`, otherwise builds a sibling `../CreatureGathererTools` checkout, otherwise downloads the locked and checksummed release. Run `./tools/cgfx-tools.sh` to inspect the selected executable. Do not remove `Arduboy-Python-Utilities/fxdata-build.py` or `fxdata/fxdata.txt` until that transitional bridge is retired.
