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
expected=d9946b91aae1371e59f22bcb91b19e036a6a61f36c5316948e3c82b8b7274352

for layout in "$old_layout" "$new_layout" "$perturbed_layout"; do
    test -f "$layout" || {
        printf 'layout equivalence: missing fixture %s\n' "$layout" >&2
        exit 2
    }
done
test "$(grep -c '^\[\[entry\]\]' "$old_layout")" -eq 304 || {
    printf 'layout equivalence: old fixture must contain 304 entries\n' >&2
    exit 2
}
test "$(grep -c '^\[\[entry\]\]' "$new_layout")" -eq 20 || {
    printf 'layout equivalence: expand fixture must contain 20 entries\n' >&2
    exit 2
}

# Rebuild pack inputs before comparison; both plans below execute in this exact
# cgfx-core binary, directly over the project-relative permanent fixtures.
make --no-print-directory pack
tool=$(./tools/cgfx-tools.sh)
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
printf '%s\n' "$diagnostic" | grep -Eq 'old: name="encounterRates".*addr=0x[0-9A-F]{6}' || {
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
