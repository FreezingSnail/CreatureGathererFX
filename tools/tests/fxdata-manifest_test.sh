#!/usr/bin/env bash
# Permanent fixtures for deterministic FX provenance verification.
set -euo pipefail

root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd -P)
fixtures="$root/tools/tests/fixtures/fxdata-manifest"
assert="$root/tools/assert-fxdata-manifest.sh"
record="$root/tools/record-fxdata-manifest.sh"
source "$root/tools/fxdata-manifest-lib.sh"

prefixed_version_manifest=$(mktemp)
trap 'rm -f "$prefixed_version_manifest"' EXIT
printf '%s\n' '  "tool_version": "cgfx-tools 0.2.0",' > "$prefixed_version_manifest"
test "$(fxdata_tool_version "$prefixed_version_manifest")" = '0.2.0'

verify() {
    "$assert" "$1/fxdata/fxdata.txt" "$1/fxdata/generated/manifest.json"
}

must_fail_with() {
    local fixture=$1 expected=$2 output
    if output=$(verify "$fixture" 2>&1); then
        printf 'expected provenance verification failure: %s\n' "$fixture" >&2
        exit 1
    fi
    printf '%s\n' "$output" | grep -F "$expected" >/dev/null || {
        printf 'missing failure detail for %s: %s\n%s\n' "$fixture" "$expected" "$output" >&2
        exit 1
    }
    printf '%s\n' "$output" | grep -F 'remedy: run make gen' >/dev/null || {
        printf 'missing make gen remediation for %s\n%s\n' "$fixture" "$output" >&2
        exit 1
    }
}

verify "$fixtures/valid"
must_fail_with "$fixtures/changed-input" 'changed inputs: data/source.json'
must_fail_with "$fixtures/missing-output" 'missing outputs output: fxdata/generated/data.bin'
must_fail_with "$fixtures/unexpected-output" 'unexpected outputs artifact: fxdata/generated/unexpected.txt'
must_fail_with "$fixtures/malformed-manifest" 'malformed manifest:'

changed_expected=$(awk -F '"' '/"path":"dist\/fxdata.bin"/ { print $8; exit }' "$fixtures/changed-image/fxdata/generated/manifest.json")
changed_observed=$(fxdata_sha256 "$fixtures/changed-image/dist/fxdata.bin")
must_fail_with "$fixtures/changed-image" 'changed image: dist/fxdata.bin'
must_fail_with "$fixtures/changed-image" "expected $changed_expected, observed $changed_observed"
must_fail_with "$fixtures/unexpected-image" 'unexpected image artifact: dist/fxdata-save.bin'
must_fail_with "$fixtures/orphan-image" 'missing image output: dist/fxdata-save.bin'

skip_output=$(make --no-print-directory -C "$root" verify-generated \
    FX_LAYOUT="tools/tests/fixtures/fxdata-manifest/no-image/fxdata/fxdata.txt" \
    FXDATA_MANIFEST="tools/tests/fixtures/fxdata-manifest/no-image/fxdata/generated/manifest.json" \
    FXDATA_DIST_DIR="tools/tests/fixtures/fxdata-manifest/no-image/dist")
printf '%s\n' "$skip_output" | grep -F 'verify-generated: image not built; run make gen to build packed FX artifacts' >/dev/null

first=$(fxdata_sha256 "$fixtures/deterministic/fxdata/generated/manifest.json")
"$record" "$fixtures/deterministic/fxdata/fxdata.txt" "$fixtures/deterministic/fxdata/generated/manifest.json"
second=$(fxdata_sha256 "$fixtures/deterministic/fxdata/generated/manifest.json")
"$record" "$fixtures/deterministic/fxdata/fxdata.txt" "$fixtures/deterministic/fxdata/generated/manifest.json"
third=$(fxdata_sha256 "$fixtures/deterministic/fxdata/generated/manifest.json")
test "$first" = "$second"
test "$second" = "$third"
printf 'fxdata-manifest: PASS\n'
