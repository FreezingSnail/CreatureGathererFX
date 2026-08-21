#!/usr/bin/env bash
# Build the FX assembler's named stock-team descriptor from cgfx-tools output.
set -euo pipefail

fixture=tst/fxdatatest/generated/arena_data.hpp
legacy_skeleton=fxdata/data/teams.txt
arena=fxdata/generated/arena.txt
output=fxdata/generated/teams.txt

for input in "$fixture" "$legacy_skeleton" "$arena"; do
    test -f "$input" || { echo "missing generated team input: $input" >&2; exit 1; }
done

awk '
    FNR == NR {
        if ($0 ~ /^[[:space:]]*\{[[:space:]]*[0-9]+,/) {
            row = $0
            sub(/^[^{]*\{[[:space:]]*/, "", row)
            sub(/[[:space:]]*\}.*/, "", row)
            rows[++count] = row
        }
        next
    }
    /^[[:space:]]*uint24_t teamList\[\] = \{/ { exit }
    /^[[:space:]]*uint8_t [[:alnum:]_]+\[\] = \{/ {
        prefix = $0
        sub(/\{.*/, "", prefix)
        print prefix "{" rows[++row_index] "};"
        next
    }
    { print }
    END {
        if (count == 0 || row_index != count) exit 1
    }
' "$fixture" "$legacy_skeleton" > "$output" || {
    echo "stock-team rows did not match Rust fixture" >&2
    exit 1
}

cat "$arena" >> "$output"
printf '\n} namespace_end\n' >> "$output"
