#!/bin/sh
# Compare cgfx-tools output with an independently generated fxdata-build.py baseline.
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd -P)
cd "$root"
mkdir -p build
stage=$(cd "$(mktemp -d "build/pack-parity.XXXXXX")" && pwd -P)
tar -cf - --exclude './.git' --exclude './build' --exclude './dist' . | tar -xf - -C "$stage"

(
    cd "$stage/fxdata"
    python3 "$stage/Arduboy-Python-Utilities/fxdata-build.py" "$stage/fxdata/fxdata.txt"
)
mkdir -p "$stage/python"
mv -f "$stage/fxdata/fxdata.bin" "$stage/python/fxdata.bin"
mv -f "$stage/fxdata/fxdata-data.bin" "$stage/python/fxdata-data.bin"
mv -f "$stage/fxdata/fxdata.h" "$stage/python/fxdata.h"

tool=$(./tools/cgfx-tools.sh)
"$tool" --project "$stage/cgfx-project.json" --pack --layout "$stage/fxlayout.toml"

cmp "$stage/python/fxdata.bin" "$stage/dist/fxdata.bin"
cmp "$stage/python/fxdata-data.bin" "$stage/dist/fxdata-data.bin"
normalize_header() {
    sed '/^\/\*\*\*\*/d' "$1"
}
normalize_header "$stage/python/fxdata.h" > "$stage/python/header.normalized"
normalize_header "$stage/src/fxdata.h" > "$stage/cgfx-header.normalized"
cmp "$stage/python/header.normalized" "$stage/cgfx-header.normalized"
printf '%s\n' 'pack parity: PASS'
