#!/bin/sh
# Optional, explicit Arduino setup. Default invocation is deliberately non-mutating.
set -eu

if [ "${SETUP_APPLY:-}" != 1 ]; then
    printf '%s\n' \
        'tools/setup.sh: no changes made.' \
        'Review the commands, then rerun with SETUP_APPLY=1 to install cores/libraries and edit Arduino CLI configuration.' >&2
    exit 2
fi

git submodule update --init --recursive
arduino-cli config init --additional-urls https://arduboy.github.io/board-support/package_arduboy_index.json
arduino-cli config add board_manager.additional_urls https://raw.githubusercontent.com/MrBlinky/Arduboy-homemade-package/master/package_arduboy_homemade_index.json
arduino-cli core update-index
arduino-cli core install arduino:avr
arduino-cli core install arduboy-homemade:avr
arduino-cli lib install Arduboy2
arduino-cli lib install ArdBitmap
arduino-cli compile --fqbn "arduboy-homemade:avr:arduboy" . --output-dir .
