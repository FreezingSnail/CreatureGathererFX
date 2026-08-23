#!/usr/bin/env bash
# Verify deterministic cgfx-tools provenance before consuming FX artifacts.
set -euo pipefail

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)
# shellcheck source=fxdata-manifest-lib.sh
source "$script_dir/fxdata-manifest-lib.sh"

skip_image=false
if test "${1:-}" = '--skip-image'; then
    skip_image=true
    shift
fi

fxdata_file=${1:-fxlayout.toml}
manifest=${2:-fxdata/generated/manifest.json}
fxdata_file=$(CDPATH= cd -- "$(dirname -- "$fxdata_file")" && printf '%s/%s\n' "$PWD" "$(basename -- "$fxdata_file")")
test -f "$fxdata_file" || fxdata_manifest_fail "missing FX definition: $fxdata_file"
test -f "$manifest" || fxdata_manifest_fail "missing generated manifest: $manifest"
root=$(fxdata_manifest_root "$fxdata_file")
target=$(fxdata_relative_path "$root" "$fxdata_file")
parsed="$manifest.parsed.$$"
expected="$manifest.expected.$$"
trap 'rm -f "$parsed" "$expected"' EXIT

awk -v target="$target" '
function bad(message) { print message > "/dev/stderr"; exit 1 }
function trim(value) { sub(/^[[:space:]]+/, "", value); sub(/[[:space:]]+$/, "", value); return value }
BEGIN { state = 0; count["inputs"] = 0; count["outputs"] = 0; count["image"] = 0 }
{
    line = trim($0)
    if (state == 0) { if (line != "{") bad("expected opening object"); state = 1; next }
    if (state == 1) { if (line != "\"schema_version\":1,") bad("expected schema_version 1"); state = 2; next }
    if (state == 2) {
        if (line !~ /^"generator":\{"name":"cgfx-tools","version":"[A-Za-z0-9._+-]+"\},$/) bad("invalid generator")
        state = 3; next
    }
    if (state == 3) { if (line != "\"target\":\"" target "\",") bad("invalid target"); state = 4; next }
    if (state == 4) { if (line != "\"inputs\":[") bad("expected inputs"); section = "inputs"; state = 5; next }
    if (state == 6) { if (line != "\"outputs\":[") bad("expected outputs"); section = "outputs"; state = 5; next }
    if (state == 7) { if (line != "\"image\":[") bad("expected image"); section = "image"; state = 5; next }
    if (state == 5) {
        if (line == "]" || line == "],") {
            if ((section != "image" && count[section] == 0) || trailing_comma) bad("invalid " section)
            if ((section == "inputs" || section == "outputs") && line != "],") bad(section " missing separator")
            if (section == "image" && line != "]") bad("image has separator")
            state = section == "inputs" ? 6 : (section == "outputs" ? 7 : 8)
            next
        }
        trailing_comma = $0 ~ /,$/
        entry = $0
        if (trailing_comma) sub(/,$/, "", entry)
        pattern = "^    \\{\\\"path\\\":\\\"[^\\\"]+\\\",\\\"sha256\\\":\\\"[a-f0-9]+\\\"\\}$"
        if (entry !~ pattern) bad("invalid " section " entry")
        if (count[section] > 0 && !previous_comma) bad("missing " section " separator")
        path = entry; sub(/^    \{"path":"/, "", path); sub(/","sha256".*/, "", path)
        sha = entry; sub(/^.*"sha256":"/, "", sha); sub(/".*/, "", sha)
        if (length(sha) != 64) bad("invalid " section " checksum")
        print section "\t" path "\t" sha
        count[section]++; previous_comma = trailing_comma; next
    }
    if (state == 8) { if (line != "}") bad("expected closing object"); state = 9; next }
    bad("unexpected content")
}
END { if (state != 9) exit 1 }
' "$manifest" > "$parsed" || fxdata_manifest_fail "malformed manifest: $manifest"

if "$skip_image"; then
    awk -F '\t' '$1 != "image"' "$parsed" > "$parsed.filtered"
    mv -f "$parsed.filtered" "$parsed"
fi

while IFS=$'\t' read -r section path hash; do
    test -n "$path" || continue
    test -f "$root/$path" || fxdata_manifest_fail "missing $section output: $path (expected sha256 $hash)"
    observed=$(fxdata_sha256 "$root/$path")
    test "$observed" = "$hash" || fxdata_manifest_fail "changed $section: $path (expected $hash, observed $observed)"
done < "$parsed"

{
    while IFS= read -r path; do
        printf 'inputs\t%s\t%s\n' "$path" "$(fxdata_sha256 "$root/$path")"
    done < <(fxdata_collect_inputs "$root" "$fxdata_file" | fxdata_sorted_unique)
    while IFS= read -r path; do
        printf 'outputs\t%s\t%s\n' "$path" "$(fxdata_sha256 "$root/$path")"
    done < <(fxdata_collect_outputs "$root" "$fxdata_file" | fxdata_sorted_unique)
    if ! "$skip_image"; then
        while IFS= read -r path; do
            printf 'image\t%s\t%s\n' "$path" "$(fxdata_sha256 "$root/$path")"
        done < <(fxdata_collect_image_outputs "$root" | fxdata_sorted_unique)
    fi
} | LC_ALL=C sort > "$expected"
LC_ALL=C sort "$parsed" > "$parsed.sorted"
mv -f "$parsed.sorted" "$parsed"
if ! cmp -s "$parsed" "$expected"; then
    missing=$(comm -23 "$expected" "$parsed" | head -n 1 || true)
    extra=$(comm -13 "$expected" "$parsed" | head -n 1 || true)
    if test -n "$missing"; then
        IFS=$'\t' read -r section path hash <<< "$missing"
        fxdata_manifest_fail "unexpected $section artifact: $path (observed sha256 $hash; missing manifest entry)"
    fi
    IFS=$'\t' read -r section path hash <<< "$extra"
    fxdata_manifest_fail "unexpected manifest $section entry: $path (expected sha256 $hash; no matching artifact)"
fi
