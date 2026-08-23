#!/usr/bin/env bash
# Shared deterministic provenance helpers for FX generation scripts.

fxdata_manifest_fail() {
    printf 'generated artifacts: %s\nremedy: run make gen\n' "$1" >&2
    exit 1
}

fxdata_sha256() {
    if command -v shasum >/dev/null 2>&1; then
        shasum -a 256 "$1" | awk '{print $1}'
    elif command -v sha256sum >/dev/null 2>&1; then
        sha256sum "$1" | awk '{print $1}'
    else
        fxdata_manifest_fail 'neither shasum nor sha256sum is available'
    fi
}

fxdata_manifest_root() {
    if test "$(basename "$1")" = fxlayout.toml; then
        (CDPATH= cd -- "$(dirname -- "$1")" && pwd -P)
    else
        (CDPATH= cd -- "$(dirname -- "$1")/.." && pwd -P)
    fi
}

fxdata_relative_path() {
    local root=$1 path=$2
    case $path in
        "$root"/*) printf '%s\n' "${path#"$root"/}" ;;
        *) test -f "$root/$path" || fxdata_manifest_fail "path outside project root: $path"; printf '%s\n' "$path" ;;
    esac
}

fxdata_collect_inputs() {
    local root=$1 definition=${2:-fxlayout.toml} path
    if test "$(basename "$definition")" = fxlayout.toml; then
        for path in cgfx-project.json fxlayout.toml fxsprites.toml tools/cgfx-tools.sh tools/toolchain.lock; do
            test -f "$root/$path" && printf '%s\n' "$path"
        done
    else
        for path in cgfx-project.json fxdata/fxdata.txt tools/cgfx-tools.sh tools/toolchain.lock \
            Arduboy-Python-Utilities/fxdata-build.py; do
            test -f "$root/$path" && printf '%s\n' "$path"
        done
    fi
    for path in data maps images Font4x6 ArduboyFXFonts; do
        test -d "$root/$path" || continue
        (cd "$root" && find "$path" -type f -print | LC_ALL=C sort)
    done
}

fxdata_fxdata_payloads() {
    local fxdata_file=$1
    awk '
        /^[[:space:]]*include[[:space:]]+"/ || /^[[:space:]]*raw_t[[:space:]]+/ {
            path = $0
            sub(/^[^"]*"/, "", path)
            sub(/".*/, "", path)
            print path
        }
    ' "$fxdata_file"
}

fxdata_collect_outputs() {
    local root=$1 definition=$2 path
    if test "$(basename "$definition")" = fxlayout.toml; then
        test -d "$root/fxdata" && (
            cd "$root" && find fxdata -type f ! -name 'manifest.json*' -print | LC_ALL=C sort
        )
        test -d "$root/tst/fxdatatest/generated" && (
            cd "$root" && find tst/fxdatatest/generated -type f -print | LC_ALL=C sort
        )
        return
    fi
    test -d "$root/fxdata/generated" && (
        cd "$root" && find fxdata/generated -type f ! -name 'manifest.json*' -print | LC_ALL=C sort
    )
    while IFS= read -r path; do
        test -n "$path" || continue
        test -f "$root/fxdata/$path" || fxdata_manifest_fail "missing packaged FX output: fxdata/$path"
        printf 'fxdata/%s\n' "$path"
    done < <(fxdata_fxdata_payloads "$definition")
    test -d "$root/tst/fxdatatest/generated" && (
        cd "$root" && find tst/fxdatatest/generated -type f -print | LC_ALL=C sort
    )
}

fxdata_collect_image_outputs() {
    local root=$1 path
    for path in src/fxdata.h dist/fxdata-data.bin dist/fxdata.bin dist/fxdata-save.bin; do
        test -f "$root/$path" && printf '%s\n' "$path"
    done
}

fxdata_sorted_unique() {
    LC_ALL=C sort -u
}

fxdata_tool_version() {
    local manifest=$1 version
    version=$(awk -F '"' '/^[[:space:]]*"tool_version"[[:space:]]*:/ { print $4; exit }' "$manifest")
    if test -z "$version"; then
        version=$(awk -F '"' '/^[[:space:]]*"generator"[[:space:]]*:/ { if ($6 == "cgfx-tools" && $8 == "version") { print $10; exit } }' "$manifest")
    fi
    test -n "$version" || fxdata_manifest_fail "manifest has no cgfx-tools version: $manifest"
    # cgfx-tools emits "cgfx-tools <version>"; record only its version token.
    case $version in
        'cgfx-tools '*) version=${version#'cgfx-tools '} ;;
    esac
    case $version in
        *[!A-Za-z0-9._+-]*) fxdata_manifest_fail "unsupported cgfx-tools version: $version" ;;
    esac
    printf '%s\n' "$version"
}
