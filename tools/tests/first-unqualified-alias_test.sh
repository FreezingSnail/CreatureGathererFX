#!/usr/bin/env bash
# Pin the legacy resolver rule against the current packed image: a bare symbol
# takes the first declaration in layout order. MenuStrings deliberately leaves
# these names unresolved in its namespace and must use MenuFXData, not globals.
set -euo pipefail

root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd -P)
header=${FX_HEADER:-$root/src/fxdata.h}
image=${FX_IMAGE:-$root/dist/fxdata-data.bin}
expectations=${FX_ALIAS_EXPECTATIONS:-$root/tools/tests/fixtures/first-unqualified-aliases.tsv}
menu_strings_address=$(awk '
    /^namespace MenuStrings$/ { in_table = 1; next }
    in_table && $1 == "constexpr" && $2 == "uint24_t" && $3 == "MenuStrings" {
        value = $5
        sub(/;$/, "", value)
        print value
        found = 1
        exit
    }
    END { exit !found }
' "$header")

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

while IFS=$'\t' read -r index leaf target competing; do
    case "$index" in ''|'#'*) continue ;; esac
    rows=$((rows + 1))
    actual_target=$(menu_address "$leaf" || true)
    actual_competing=$(global_address "$leaf" || true)
    table_offset=$((menu_strings_address + index * 3))
    resolved=$(read_be24 "$table_offset" || true)

    if [ -z "$actual_target" ] || [ -z "$actual_competing" ] ||
       [ "$actual_target" = "$actual_competing" ] || [ "$resolved" != "$actual_target" ]; then
        fail "$leaf: expected MenuStrings[$index] to resolve first declaration $target; competing $competing; published $target=${actual_target:-missing}, $competing=${actual_competing:-missing}, MenuStrings[$index]=${resolved:-missing}"
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
