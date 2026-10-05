#!/usr/bin/env bash
# Verify arena catalog inputs, generated outputs, and test-only fixture staging
# are represented in the normal generated-artifact provenance.
set -euo pipefail

root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd -P)
cd "$root"
manifest=fxdata/generated/manifest.json
test -f "$manifest" || { printf 'arena demo data: missing manifest; run make gen\n' >&2; exit 1; }

for input in 'data/arena-demo.toml' 'images/ArduFont_5x6.png'; do
    grep -Fq '"path":"'"$input"'"' "$manifest" || {
        printf 'arena demo data: manifest omits input %s\n' "$input" >&2
        exit 1
    }
done

for output in 'fxdata/generated/arena_demo.txt' \
              'fxdata/generated/arena_demo_ids.hpp' \
              'tst/fxdatatest/generated/arena_demo_data.hpp'; do
    test -f "$output" || { printf 'arena demo data: missing %s; run make gen\n' "$output" >&2; exit 1; }
    grep -Fq '"path":"'"$output"'"' "$manifest" || {
        printf 'arena demo data: manifest omits generated output %s\n' "$output" >&2
        exit 1
    }
done

if grep -Eq 'ArenaDemoFixture|PROGMEM[^\n]*arena_demo' src/fxdata.h; then
    printf 'arena demo data: production FX address header contains a resident demo table\n' >&2
    exit 1
fi

printf 'arena-demo-data: provenance and production table placement PASS\n'
