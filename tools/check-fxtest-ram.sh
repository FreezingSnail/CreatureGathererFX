#!/bin/sh
set -eu

usage() {
    echo "usage: $0 ELF AVR_SIZE RAM_LIMIT SUITE" >&2
    exit 2
}

fail() {
    echo "fxtest RAM: $*" >&2
    exit 1
}

[ "$#" -eq 4 ] || usage
elf=$1
size_tool=$2
limit=$3
suite=$4

case $limit in
    ''|*[!0-9]*) fail "invalid RAM limit '$limit'" ;;
esac
[ "${#limit}" -le 4 ] || fail "invalid RAM limit '$limit' (ATmega32u4 SRAM is 2560 B)"
[ "$limit" -le 2560 ] || fail "invalid RAM limit '$limit' (ATmega32u4 SRAM is 2560 B)"
[ -f "$elf" ] || fail "$suite ELF not found: $elf"
command -v "$size_tool" >/dev/null 2>&1 || fail "avr-size not found: $size_tool"

if ! sections=$("$size_tool" -A "$elf" 2>&1); then
    fail "$suite avr-size failed: $sections"
fi
if ! parsed=$(printf '%s\n' "$sections" | awk '
    $1 == ".data" {
        if (++data_count != 1 || $2 !~ /^[0-9]+$/) bad = 1
        data = $2
    }
    $1 == ".bss" {
        if (++bss_count != 1 || $2 !~ /^[0-9]+$/) bad = 1
        bss = $2
    }
    END {
        if (bad || data_count != 1 || bss_count != 1) exit 1
        printf "%s %s\n", data, bss
    }
'); then
    fail "$suite ELF has missing, duplicate, or invalid .data/.bss size sections"
fi
set -- $parsed
data=$1
bss=$2
used=$((data + bss))
free=$((limit - used))
printf 'FXTEST_RAM_SUITE=%s\nFXTEST_RAM_DATA_BYTES=%s\nFXTEST_RAM_BSS_BYTES=%s\nFXTEST_RAM_USED_BYTES=%s\nFXTEST_RAM_LIMIT=%s\nFXTEST_RAM_FREE_BYTES=%s\n' \
    "$suite" "$data" "$bss" "$used" "$limit" "$free"
[ "$used" -le "$limit" ] || fail "$suite uses $used B static RAM; limit is $limit B"
