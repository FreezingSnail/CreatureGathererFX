#!/usr/bin/env bash
# Permanent integration tests for generated-library invariants that live outside
# the packed entry table:
#
#   1. no drift  - generated headers still match their committed src/ copies,
#                  which is what the firmware actually compiles
#   2. blob form - the script text block keeps its little-endian count/offset/
#                  length framing inside the packed image, as drawScriptText()
#                  and ReadFXu16() assume
#
# Entry-by-entry byte placement is covered by tst/generated/generated_libs_test.cpp
# (make test-generated-libs).
#
# Overrides: FX_HEADER, FX_IMAGE.
set -euo pipefail

root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd -P)
header=${FX_HEADER:-$root/src/fxdata.h}
image=${FX_IMAGE:-$root/dist/fxdata-data.bin}

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

passes=0
failures=0

pass() {
    passes=$((passes + 1))
}

fail() {
    failures=$((failures + 1))
    printf 'FAIL %s\n' "$1" >&2
    if [ "$#" -gt 1 ]; then
        printf '     %s\n' "${@:2}" >&2
    fi
}

require_file() {
    test -f "$1" || {
        printf 'generated-libs: missing %s\n%s\n' "$1" "$2" >&2
        exit 1
    }
}

require_file "$header" 'remedy: run make gen'
require_file "$image" 'remedy: run make gen'

# --- src copies of generated headers -----------------------------------------
# make gen writes these into fxdata/generated only. The copies under src/ are
# compiled into the firmware, so drift desynchronizes the firmware from the
# packed data without any build error.
assert_no_drift() {
    local generated=$1 copy=$2
    if [ ! -f "$root/$copy" ]; then
        fail "drift $copy" 'committed copy is missing' "remedy: cp -f $generated $copy"
        return
    fi
    if diff -u "$root/$copy" "$root/$generated" > "$work/drift.diff"; then
        pass
        return
    fi
    fail "drift $copy" \
        "$copy differs from $generated" \
        "remedy: cp -f $generated $copy, then re-check its consumers" \
        "$(cat "$work/drift.diff")"
}

assert_no_drift fxdata/generated/opcodes.hpp src/vm/opcodes.hpp
assert_no_drift fxdata/generated/flags.hpp src/flags/flags.hpp
assert_no_drift fxdata/generated/flag_bit_array.hpp src/flags/flag_bit_array.hpp
assert_no_drift fxdata/generated/flag_bit_array.cpp src/flags/flag_bit_array.cpp

# --- script text block framing ----------------------------------------------
# Written as u16 LE string count, u16 LE offset per string, then each string as
# u16 LE byte length followed by its bytes.
address_of() {
    awk -v want="$1" '
        /^namespace [A-Za-z_][A-Za-z0-9_]*/ { next }
        $1 == "constexpr" && $2 == "uint24_t" && $3 == want {
            value = $5
            sub(/;$/, "", value)
            print value
            found = 1
            exit
        }
        END { exit !found }
    ' "$header"
}

# head closing the pipe early is expected, so pipefail is relaxed per slice.
image_byte() {
    (
        set +o pipefail
        tail -c "+$(($1 + 1))" "$image" | head -c 1 | od -An -tu1 | tr -d ' \n'
    )
}

read_le16() {
    local low high
    low=$(image_byte "$1")
    high=$(image_byte "$(($1 + 1))")
    printf '%d\n' "$((low + (high << 8)))"
}

assert_text_block() {
    local base_hex base
    if ! base_hex=$(address_of raw_map_text); then
        fail 'text block' 'raw_map_text is not published in the header' \
            'remedy: run make gen'
        return
    fi
    base=$((base_hex))

    local count
    count=$(read_le16 "$base")
    if [ "$count" -eq 0 ] || [ "$count" -gt 4096 ]; then
        fail 'text block' "implausible string count $count at raw_map_text" \
            'a byte-swapped count is the usual cause'
        return
    fi
    pass

    local blob_start=$((base + 2 + count * 2))
    local index=0 previous=-1 offset length
    while [ "$index" -lt "$count" ]; do
        offset=$(read_le16 "$((base + 2 + index * 2))")
        if [ "$offset" -le "$previous" ] && [ "$index" -gt 0 ]; then
            fail 'text block' "offset $index ($offset) does not ascend past $previous"
            return
        fi
        length=$(read_le16 "$((blob_start + offset))")
        if [ "$length" -eq 0 ] || [ "$length" -gt 1024 ]; then
            fail 'text block' "string $index at offset $offset has implausible length $length"
            return
        fi
        previous=$offset
        index=$((index + 1))
    done
    pass

    # The last string must end before the next published symbol.
    local blob_end=$((blob_start + previous + 2 + length))
    local next_hex
    if next_hex=$(address_of generator_version); then
        if [ "$blob_end" -gt "$((next_hex))" ]; then
            fail 'text block' \
                "blob ends at $blob_end, past generator_version at $((next_hex))"
            return
        fi
        pass
    fi
}

assert_text_block

printf 'generated-libs-invariants: %d passed, %d failed\n' "$passes" "$failures"
test "$failures" -eq 0 || exit 1
printf 'generated-libs-invariants: PASS\n'
