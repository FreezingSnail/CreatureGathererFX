#!/bin/sh
# Verify native cgfx-tools packing preserves the committed FX image baseline.
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd -P)
cd "$root"
expected=d8af838ee9fa759fbdc14cb8eeceb7df06f246857cf92d58f33e000cf4d0ac01

make --no-print-directory pack
actual=$(shasum -a 256 dist/fxdata.bin | awk '{print $1}')
test "$actual" = "$expected" || {
    printf 'pack parity: expected %s, observed %s\n' "$expected" "$actual" >&2
    exit 1
}
printf '%s\n' 'pack parity: PASS'
