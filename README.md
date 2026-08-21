# CreatureGathererFX

Creature collecting demake game for the Arduboy.

## Generate and validate

Run `make gen` to generate game data and package the FX image. Game data is produced by `cgfx-tools`; retained Python tooling only produces sprite and string-image assets in `fxdata/generated`.

Run `make check` for generation, host tests, and FX tests when [Ardens](https://github.com/tiberiusbrown/Ardens) is available. Set `ARDENS=/path/to/Ardens` to enable the device-test stage; without it, `fxtest` reports a skip.

`tools/cgfx-tools.sh` resolves the data generator. It uses an executable set through `CGFX_TOOLS_BIN`, otherwise builds a sibling `../CreatureGathererTools` checkout, otherwise downloads the locked and checksummed release. Run `./tools/cgfx-tools.sh` to show the selected executable.
