#!/usr/bin/env bash
# Add the complete fxdata.txt input set to the cgfx-tools manifest.
set -euo pipefail

fxdata_file=${1:-fxdata/fxdata.txt}
manifest=${2:-fxdata/generated/manifest.json}

test -f "$fxdata_file" || { echo "manifest: missing FX definition: $fxdata_file" >&2; exit 1; }
test -f "$manifest" || { echo "manifest: missing generated manifest: $manifest" >&2; exit 1; }

inputs=$(awk '
    /^[[:space:]]*include[[:space:]]+"/ {
        path = $0
        sub(/^[^"]*"/, "", path)
        sub(/".*/, "", path)
        print path
    }
    /^[[:space:]]*raw_t[[:space:]]+/ {
        path = $0
        sub(/^[^"]*"/, "", path)
        sub(/".*/, "", path)
        print path
    }
' "$fxdata_file" | awk '!seen[$0]++')
test -n "$inputs" || { echo "manifest: no FX inputs found in $fxdata_file" >&2; exit 1; }

tmp="$manifest.tmp.$$"
trap 'rm -f "$tmp"' EXIT
if grep -q '"fxdata_inputs"' "$manifest"; then
    awk '/^[[:space:]]*"fxdata_inputs"[[:space:]]*:/ { exit } { print }' "$manifest" |
        sed '$ s/,$//' > "$tmp"
else
    sed '$d' "$manifest" > "$tmp"
fi
printf ',\n  "fxdata_inputs": [\n' >> "$tmp"
first=1
while IFS= read -r input; do
    if [[ $first -eq 0 ]]; then
        printf ',\n' >> "$tmp"
    fi
    printf '    "%s"' "$input" >> "$tmp"
    first=0
done <<< "$inputs"
printf '\n  ]\n}\n' >> "$tmp"
mv -f "$tmp" "$manifest"
trap - EXIT
