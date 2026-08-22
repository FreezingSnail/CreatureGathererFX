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
printf '%s\n' '  "tool_version": "cgfx-tools 0.1.0",' > "$prefixed_version_manifest"
test "$(fxdata_tool_version "$prefixed_version_manifest")" = '0.1.0'

verify() {
    "$assert" "$1/fxdata/fxdata.txt" "$1/fxdata/generated/manifest.json"
}

must_fail() {
    if verify "$1" >/dev/null 2>&1; then
        printf 'expected provenance verification failure: %s\n' "$1" >&2
        exit 1
    fi
}

verify "$fixtures/valid"
must_fail "$fixtures/changed-input"
must_fail "$fixtures/missing-output"
must_fail "$fixtures/unexpected-output"
must_fail "$fixtures/malformed-manifest"

first=$(fxdata_sha256 "$fixtures/deterministic/fxdata/generated/manifest.json")
"$record" "$fixtures/deterministic/fxdata/fxdata.txt" "$fixtures/deterministic/fxdata/generated/manifest.json"
second=$(fxdata_sha256 "$fixtures/deterministic/fxdata/generated/manifest.json")
"$record" "$fixtures/deterministic/fxdata/fxdata.txt" "$fixtures/deterministic/fxdata/generated/manifest.json"
third=$(fxdata_sha256 "$fixtures/deterministic/fxdata/generated/manifest.json")
test "$first" = "$second"
test "$second" = "$third"
printf 'fxdata-manifest: PASS\n'
