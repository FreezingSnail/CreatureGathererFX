#!/bin/sh
# Verify the hand-expanded migration fixture and expand fixture keep the exact
# cgfx-core field ABI, then preserve the committed FX image baseline.
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd -P)
cd "$root"
fixtures=$root/tools/tests/fixtures
old_layout=$fixtures/fxlayout-old-304.toml
new_layout=$fixtures/fxlayout-expand-20.toml
perturbed_layout=$fixtures/fxlayout-expand-20-perturbed.toml
# Baseline includes the eight-sector save region declared by fxlayout.toml.
# Restored TSX-authored walkability; GIDs, field addresses and save bytes agree
# with the prior baseline. Semantic map checks run in test-generated-libs.
# Utility prototype adds move names, kits and trainer basic attacks.
# Runtime/authored matchup chart reconciliation updates the packed lookup table.
expected=1b09633a00e4d64a26413d67fd62d04c600df3e1b9623d6b8dbe796d1ccd91a3

for layout in "$old_layout" "$new_layout" "$perturbed_layout"; do
    test -f "$layout" || {
        printf 'layout equivalence: missing fixture %s\n' "$layout" >&2
        exit 2
    }
done
test "$(grep -c '^\[\[entry\]\]' "$old_layout")" -eq 317 || {
    printf 'layout equivalence: old fixture must contain 317 entries\n' >&2
    exit 2
}
test "$(grep -c '^\[\[entry\]\]' "$new_layout")" -eq 20 || {
    printf 'layout equivalence: expand fixture must contain 20 entries\n' >&2
    exit 2
}

# Rebuild pack inputs before comparison; both plans below execute in this exact
# cgfx-core binary, directly over the project-relative permanent fixtures.
make --no-print-directory pack
tool=cgfx-tools
"$tool" --assert-layout-equivalent "$old_layout" "$new_layout"

# Permanent negative proof: removing one early declaration must name the first
# divergent field and publish both compared addresses rather than a hash only.
if diagnostic=$("$tool" --assert-layout-equivalent "$old_layout" "$perturbed_layout" 2>&1); then
    printf 'layout equivalence: perturbed fixture unexpectedly matched\n' >&2
    exit 1
fi
printf '%s\n' "$diagnostic" | grep -Fq 'layout equivalence: field 0 differs' || {
    printf 'layout equivalence: perturbation omitted first differing field\n%s\n' "$diagnostic" >&2
    exit 1
}
printf '%s\n' "$diagnostic" | grep -Eq 'old: name="zoneDefs".*addr=0x[0-9A-F]{6}' || {
    printf 'layout equivalence: perturbation omitted old field address\n%s\n' "$diagnostic" >&2
    exit 1
}
printf '%s\n' "$diagnostic" | grep -Eq 'new: name="opponent_seeds".*addr=0x[0-9A-F]{6}' || {
    printf 'layout equivalence: perturbation omitted new field address\n%s\n' "$diagnostic" >&2
    exit 1
}
printf '%s\n' 'layout equivalence: perturbation diagnostic: PASS'

actual=$(shasum -a 256 dist/fxdata.bin | awk '{print $1}')
test "$actual" = "$expected" || {
    printf 'pack parity: expected %s, observed %s\n' "$expected" "$actual" >&2
    exit 1
}
printf '%s\n' 'pack parity: PASS'
