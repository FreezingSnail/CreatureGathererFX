#!/usr/bin/env bash
# Authoritative device-suite and resource summary.
# Usage: tools/suite-report.sh [check.log]
# With no argument, use the newest final-gate check.log in this checkout.
set -euo pipefail

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P)
log=${1:-}
if test -z "$log"; then
    log=$(ls -t "$root"/build/reports/rhs7/final-gate-*/check.log \
        "$root"/build/final-gate/check.log \
        "$root"/build/rhs7-lane/final-gate/check.log 2>/dev/null | head -n 1 || true)
fi

suite_count=$(find "$root/tst/fxdatatest" -maxdepth 1 -type f -name 'test_*.ino' -print | wc -l | tr -d ' ')
printf 'device suite files: %s\n' "$suite_count"

if test -z "$log" || test ! -f "$log"; then
    printf 'gate log: none found; pass a check.log path or run make final-gate\n'
    exit 0
fi

passes=$(grep -cE '^test_[A-Za-z0-9_]+: PASS$' "$log" || true)
fails=$(grep -cE '^test_[A-Za-z0-9_]+: FAIL' "$log" || true)
printf 'gate log: %s\n' "$log"
printf 'device PASS: %s\n' "$passes"
printf 'device FAIL: %s\n' "$fails"
if test "$fails" -ne 0; then
    printf 'failures:\n'
    grep -E '^test_[A-Za-z0-9_]+: FAIL' "$log" || true
fi
grep -E '^stack headroom=' "$log" | tail -n 1 || true

ram_log=${log%/check.log}/ram.log
if test -f "$ram_log"; then
    printf 'shipping RAM:\n'
    grep -E '^(RAM_FLASH_BYTES|RAM_STATIC_BYTES|RAM_FREE_BYTES)=' "$ram_log" || true
fi
