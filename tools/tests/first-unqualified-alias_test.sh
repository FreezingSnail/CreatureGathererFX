#!/usr/bin/env bash
# Pin the legacy resolver rule independently from regenerated fxdata.h:
# a bare symbol takes the first declaration in layout order.  MenuStrings is
# deliberately unresolved in its own namespace and therefore must use the
# earlier MenuFXData declarations rather than the later global declarations.
set -euo pipefail

root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd -P)
header=${FX_HEADER:-$root/src/fxdata.h}
image=${FX_IMAGE:-$root/dist/fxdata-data.bin}
expectations=${FX_ALIAS_EXPECTATIONS:-$root/tools/tests/fixtures/first-unqualified-aliases.tsv}
# This is the committed pre-alias-resolution table location, deliberately not
# read from the regenerated header.
menu_strings_address=0x04EA95

failures=0
passes=0
rows=0

fail() {
    failures=$((failures + 1))
    printf 'FAIL first-unqualified-alias %s\n' "$1" >&2
}

require_file() {
    test -f "$1" || {
        printf 'first-unqualified-alias: missing %s\nremedy: run make gen\n' "$1" >&2
        exit 2
    }
}

menu_address() {
    awk -v want="$1" '
        /^namespace MenuFXData$/ { in_menu = 1; next }
        in_menu && /^}/ { in_menu = 0; next }
        in_menu && $1 == "constexpr" && $2 == "uint24_t" && $3 == want {
            value = $5
            sub(/;$/, "", value)
            print value
            found = 1
            exit
        }
        END { exit !found }
    ' "$header"
}

global_address() {
    awk -v want="$1" '
        /^constexpr uint24_t/ && $2 == "uint24_t" && $3 == want {
            value = $5
            sub(/;$/, "", value)
            print value
            found = 1
            exit
        }
        END { exit !found }
    ' "$header"
}

read_be24() {
    local offset=$1 bytes
    bytes=$(dd if="$image" bs=1 skip="$offset" count=3 2>/dev/null | od -An -tu1)
    set -- $bytes
    test "$#" -eq 3 || return 1
    printf '0x%02X%02X%02X\n' "$1" "$2" "$3"
}

require_file "$header"
require_file "$image"
require_file "$expectations"

while IFS=$'\t' read -r index leaf target expected competing competitor; do
    case "$index" in ''|'#'*) continue ;; esac
    rows=$((rows + 1))
    actual_target=$(menu_address "$leaf" || true)
    actual_competing=$(global_address "$leaf" || true)
    table_offset=$((menu_strings_address + index * 3))
    resolved=$(read_be24 "$table_offset" || true)

    if [ "$actual_target" != "$expected" ] || [ "$actual_competing" != "$competitor" ] || [ "$resolved" != "$expected" ]; then
        fail "$leaf: expected $target=$expected, $competing=$competitor; published $target=${actual_target:-missing}, $competing=${actual_competing:-missing}; MenuStrings[$index]=${resolved:-missing}"
        continue
    fi
    passes=$((passes + 1))
done < "$expectations"

if [ "$passes" -ne 28 ]; then
    fail "fixture has $passes passing rows; expected exactly 28 colliding leaves"
fi

printf 'first-unqualified-alias: %d passed, %d failed\n' "$passes" "$failures"
test "$failures" -eq 0
printf 'first-unqualified-alias: PASS\n'
