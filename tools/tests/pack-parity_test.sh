#!/bin/sh
# Verify native cgfx-tools packing preserves the committed FX image baseline.
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd -P)
cd "$root"
expected=d9946b91aae1371e59f22bcb91b19e036a6a61f36c5316948e3c82b8b7274352

make --no-print-directory pack
actual=$(shasum -a 256 dist/fxdata.bin | awk '{print $1}')
test "$actual" = "$expected" || {
    printf 'pack parity: expected %s, observed %s\n' "$expected" "$actual" >&2
    exit 1
}
printf '%s\n' 'pack parity: PASS'
