#!/bin/sh
set -eu

usage() {
    echo "usage: $0 ARDUINO_LOG FLASH_LIMIT STATIC_RAM_LIMIT TARGET" >&2
    exit 2
}

fail() {
    echo "AVR budget: $*" >&2
    exit 1
}

[ "$#" -eq 4 ] || usage
log=$1
flash_limit=$2
ram_limit=$3
target=$4
case $flash_limit in ''|*[!0-9]*) fail "invalid flash limit '$flash_limit'" ;; esac
case $ram_limit in ''|*[!0-9]*) fail "invalid static-RAM limit '$ram_limit'" ;; esac
[ "${#flash_limit}" -le 5 ] || fail "invalid flash limit '$flash_limit' (ATmega32u4 flash is 29696 B)"
[ "$flash_limit" -le 29696 ] || fail "invalid flash limit '$flash_limit' (ATmega32u4 flash is 29696 B)"
[ "${#ram_limit}" -le 4 ] || fail "invalid static-RAM limit '$ram_limit' (ATmega32u4 SRAM is 2560 B)"
[ "$ram_limit" -le 2560 ] || fail "invalid static-RAM limit '$ram_limit' (ATmega32u4 SRAM is 2560 B)"
[ -f "$log" ] || fail "$target build log not found: $log"

if ! parsed=$(awk '
    function number_after(word,    i, value) {
        for (i = 1; i < NF; ++i) {
            if (tolower($i) == word) {
                value = $(i + 1)
                gsub(/,/, "", value)
                if (value !~ /^[0-9]+$/) return ""
                return value
            }
        }
        return ""
    }
    tolower($0) ~ /sketch uses/ {
        if (++flash_count != 1) bad = 1
        flash = number_after("uses")
        if (flash == "") bad = 1
    }
    tolower($0) ~ /global variables use/ {
        if (++ram_count != 1) bad = 1
        ram = number_after("use")
        if (ram == "") bad = 1
    }
    END {
        if (bad || flash_count != 1 || ram_count != 1) exit 1
        printf "%s %s\n", flash, ram
    }
' "$log"); then
    fail "$target build log is missing, duplicate, or has unparseable flash/RAM measurements"
fi
set -- $parsed
flash=$1
ram=$2
flash_free=$((flash_limit - flash))
ram_free=$((ram_limit - ram))
printf 'AVR_BUDGET_TARGET=%s\nAVR_FLASH_BYTES=%s\nAVR_FLASH_LIMIT=%s\nAVR_FLASH_FREE_BYTES=%s\nAVR_STATIC_RAM_BYTES=%s\nAVR_STATIC_RAM_LIMIT=%s\nAVR_STATIC_RAM_FREE_BYTES=%s\n' \
    "$target" "$flash" "$flash_limit" "$flash_free" "$ram" "$ram_limit" "$ram_free"
[ "$flash" -le "$flash_limit" ] || fail "$target uses $flash B flash; limit is $flash_limit B"
[ "$ram" -le "$ram_limit" ] || fail "$target uses $ram B static RAM; limit is $ram_limit B"
