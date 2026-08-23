#!/usr/bin/env bash
# Record deterministic cgfx-tools provenance for all FX inputs and outputs.
set -euo pipefail

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)
# shellcheck source=fxdata-manifest-lib.sh
source "$script_dir/fxdata-manifest-lib.sh"

fxdata_file=${1:-fxdata/fxdata.txt}
manifest=${2:-fxdata/generated/manifest.json}
test -f "$fxdata_file" || fxdata_manifest_fail "missing FX definition: $fxdata_file"
test -f "$manifest" || fxdata_manifest_fail "missing cgfx-tools manifest: $manifest"

root=$(fxdata_manifest_root "$fxdata_file")
version=$(fxdata_tool_version "$manifest")
tmp="$manifest.tmp.$$"
trap 'rm -f "$tmp"' EXIT

write_entries() {
    local label=$1 collector=$2 required=${3:-true} first=1 path hash
    printf '  "%s":[\n' "$label"
    while IFS= read -r path; do
        test -n "$path" || continue
        test -f "$root/$path" || fxdata_manifest_fail "missing $label path: $path"
        hash=$(fxdata_sha256 "$root/$path")
        if test "$first" -eq 0; then printf ',\n'; fi
        printf '    {"path":"%s","sha256":"%s"}' "$path" "$hash"
        first=0
    done < <("$collector" "$root" "$fxdata_file" | fxdata_sorted_unique)
    if test "$first" -eq 1 && test "$required" != false; then
        fxdata_manifest_fail "no $label discovered"
    fi
    printf '\n  ]'
}

{
    printf '{\n'
    printf '  "schema_version":1,\n'
    printf '  "generator":{"name":"cgfx-tools","version":"%s"},\n' "$version"
    printf '  "target":"fxdata/fxdata.txt",\n'
    write_entries inputs fxdata_collect_inputs
    printf ',\n'
    write_entries outputs fxdata_collect_outputs
    printf ',\n'
    write_entries image fxdata_collect_image_outputs false
    printf '\n}\n'
} > "$tmp"
mv -f "$tmp" "$manifest"
trap - EXIT
