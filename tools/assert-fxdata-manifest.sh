#!/usr/bin/env bash
# Verify cgfx-tools artifacts and every fxdata.txt input before packing.
set -euo pipefail

fxdata_file=${1:-fxdata/fxdata.txt}
manifest=${2:-fxdata/generated/manifest.json}

fail() {
    printf 'pack: %s\n' "$1" >&2
    exit 1
}

manifest_has_input() {
    printf '%s\n' "$manifest_inputs" | grep -Fqx "$1"
}

test -f "$fxdata_file" || fail "missing FX definition: $fxdata_file"
test -f "$manifest" || fail "missing generated manifest: $manifest"

manifest_paths=$(awk -F '"' '
    /"artifacts"/ { in_artifacts = 1; next }
    in_artifacts && /^[[:space:]]*\]/ { exit }
    in_artifacts && /^[[:space:]]*"path"[[:space:]]*:/ { print $4 }
' "$manifest")
test -n "$manifest_paths" || fail "manifest has no artifacts: $manifest"
manifest_inputs=$(awk -F '"' '
    /"fxdata_inputs"/ { in_inputs = 1; next }
    in_inputs && /^[[:space:]]*\]/ { exit }
    in_inputs && /^[[:space:]]*"/ { print $2 }
' "$manifest")
test -n "$manifest_inputs" || fail "manifest has no FX inputs: $manifest"

raw_entries=$(awk '
    /^[[:space:]]*raw_t[[:space:]]+/ {
        symbol = $0
        sub(/^[[:space:]]*raw_t[[:space:]]+/, "", symbol)
        sub(/[[:space:]]*=.*/, "", symbol)
        path = $0
        sub(/^[^"]*"/, "", path)
        sub(/".*/, "", path)
        print symbol "\t" path
    }
' "$fxdata_file")

while IFS= read -r artifact; do
    test -n "$artifact" || continue
    if [[ $artifact = /* ]]; then
        payload=$artifact
    else
        payload="fxdata/generated/$artifact"
    fi
    if [[ ! -f $payload ]]; then
        symbol=$(printf '%s\n' "$raw_entries" | awk -F '\t' -v artifact="$artifact" '
            $2 == "generated/" artifact || $2 == artifact { print $1; exit }
        ')
        if [[ -n $symbol ]]; then
            fail "missing fxdata symbol '$symbol': $payload (listed by $manifest)"
        fi
        fail "missing manifest artifact: $payload (listed by $manifest)"
    fi
done <<< "$manifest_paths"

while IFS= read -r include; do
    test -n "$include" || continue
    manifest_has_input "$include" || fail "missing fxdata include '$include' in manifest: $manifest"
    test -f "fxdata/$include" || fail "missing fxdata include '$include': fxdata/$include"
done < <(awk '
    /^[[:space:]]*include[[:space:]]+"/ {
        include = $0
        sub(/^[^"]*"/, "", include)
        sub(/".*/, "", include)
        print include
    }
' "$fxdata_file")

while IFS=$'\t' read -r symbol payload; do
    test -n "$symbol" || continue
    manifest_has_input "$payload" || fail "missing fxdata symbol '$symbol' in manifest: $manifest"
    test -f "fxdata/$payload" || fail "missing fxdata symbol '$symbol': fxdata/$payload"
done <<< "$raw_entries"
