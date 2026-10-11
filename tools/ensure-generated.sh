#!/usr/bin/env bash
# Reuse packed FX outputs only when authored content, installed tooling, and
# the generation recipe still match the last successful `make gen`.
set -euo pipefail

fail() {
    printf 'generated cache: %s\n' "$1" >&2
    return 1
}

sha256_file() {
    if command -v shasum >/dev/null 2>&1; then
        shasum -a 256 "$1" | awk '{print $1}'
    elif command -v sha256sum >/dev/null 2>&1; then
        sha256sum "$1" | awk '{print $1}'
    else
        fail 'neither shasum nor sha256sum is available'
    fi
}

sha256_stream() {
    if command -v shasum >/dev/null 2>&1; then
        shasum -a 256 | awk '{print $1}'
    elif command -v sha256sum >/dev/null 2>&1; then
        sha256sum | awk '{print $1}'
    else
        fail 'neither shasum nor sha256sum is available'
    fi
}

usage() {
    echo 'usage: ensure-generated.sh --check|--record ROOT STAMP LAYOUT MANIFEST FXDATA_DIST_DIR DIST_DIR FXDATA_BIN FXDATA_DATA_BIN FXDATA_SAVE_BIN' >&2
    echo '       ensure-generated.sh --ensure ROOT STAMP LAYOUT MANIFEST FXDATA_DIST_DIR DIST_DIR FXDATA_BIN FXDATA_DATA_BIN FXDATA_SAVE_BIN -- COMMAND...' >&2
    exit 2
}

mode=${1:-}
test "$#" -ge 10 || usage
shift
root=$1
stamp=$2
layout=$3
manifest=$4
fxdata_dist_dir=$5
dist_dir=$6
fxdata_bin=$7
fxdata_data_bin=$8
fxdata_save_bin=$9
shift 9

root=$(CDPATH= cd -- "$root" && pwd -P)
case $stamp in
    /*) ;;
    *) stamp="$root/$stamp" ;;
esac
cd "$root"

default_paths() {
    test "$layout" = fxlayout.toml &&
    test "$manifest" = fxdata/generated/manifest.json &&
    test "$fxdata_dist_dir" = dist &&
    test "$dist_dir" = dist &&
    test "$fxdata_bin" = dist/fxdata.bin &&
    test "$fxdata_data_bin" = dist/fxdata-data.bin &&
    test "$fxdata_save_bin" = dist/fxdata-save.bin
}

fingerprint() {
    local tool file
    tool=$(command -v cgfx-tools) || return 1
    test -f "$tool" && test -x "$tool" || return 1
    {
        printf 'schema=1\n'
        printf 'tool-path=%s\n' "$tool"
        printf 'tool-sha256=%s\n' "$(sha256_file "$tool")"
        for file in Makefile tools/ensure-generated.sh tools/emit-rust-teams.sh \
            tools/emit-tool-version-stamp.sh tools/record-fxdata-manifest.sh \
            tools/assert-fxdata-manifest.sh tools/fxdata-manifest-lib.sh; do
            test -f "$file" || return 1
            printf '%s\t%s\n' "$file" "$(sha256_file "$file")"
        done
    } | sha256_stream
}

write_stamp() {
    local value tmp
    if ! value=$(fingerprint); then
        fail 'cannot fingerprint cgfx-tools or generation scripts'
        return 1
    fi
    mkdir -p "$(dirname -- "$stamp")"
    tmp="$stamp.tmp.$$"
    trap 'rm -f "$tmp"' EXIT
    printf '%s\n' "$value" >"$tmp"
    if test -f "$stamp" && cmp -s "$tmp" "$stamp"; then
        rm -f "$tmp"
    else
        mv -f "$tmp" "$stamp"
    fi
    trap - EXIT
}

check_fresh() {
    local expected mirror
    default_paths || { fail 'custom artifact paths disable cache reuse'; return 1; }
    test -f "$stamp" || { fail 'cache stamp is missing'; return 1; }
    expected=$(fingerprint) || { fail 'cgfx-tools or generation scripts are unavailable'; return 1; }
    test "$(cat "$stamp")" = "$expected" || { fail 'generator/tool fingerprint changed'; return 1; }

    for file in src/fxdata.h dist/fxdata.bin dist/fxdata-data.bin \
        dist/fxdata-save.bin fxdata/generated/manifest.json; do
        test -f "$file" || { fail "required generated file is missing: $file"; return 1; }
    done
    for mirror in opcodes.hpp flags.hpp flag_bit_array.hpp flag_bit_array.cpp; do
        case $mirror in
            opcodes.hpp) source=src/vm/opcodes.hpp ;;
            flags.hpp) source=src/flags/flags.hpp ;;
            flag_bit_array.hpp) source=src/flags/flag_bit_array.hpp ;;
            flag_bit_array.cpp) source=src/flags/flag_bit_array.cpp ;;
        esac
        test -f "fxdata/generated/$mirror" && test -f "$source" || {
            fail "generated header mirror is missing: $mirror"; return 1;
        }
        cmp -s "fxdata/generated/$mirror" "$source" || {
            fail "generated header mirror differs: $source"; return 1;
        }
    done
    ./tools/assert-fxdata-manifest.sh "$layout" "$manifest" || return 1
}

case $mode in
    --check)
        test "$#" -eq 0 || usage
        check_fresh
        ;;
    --record)
        test "$#" -eq 0 || usage
        write_stamp
        ;;
    --ensure)
        test "${1:-}" = -- || usage
        shift
        test "$#" -gt 0 || usage
        if check_fresh; then
            echo 'generated cache: current; skipping make gen'
        else
            echo 'generated cache: stale; running make gen' >&2
            "$@"
            write_stamp
        fi
        ;;
    *) usage ;;
esac
