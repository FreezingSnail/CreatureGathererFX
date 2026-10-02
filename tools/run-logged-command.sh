#!/bin/sh
# Preserve complete gate diagnostics while printing only useful result lines.
set -eu
log=$1
shift
mkdir -p "$(dirname "$log")"
printf 'gate: running %s; log: %s\n' "$*" "$log"
if "$@" >"$log" 2>&1; then
    awk '/Total Passed:|Total Failed:|: PASS$|PASSED=|stack headroom=|^RAM_(FLASH_BYTES|STATIC_BYTES|FREE_BYTES)=/ { print }' "$log"
    printf 'gate: PASS; log: %s\n' "$log"
else
    status=$?
    printf 'gate: FAIL (exit %s); last 80 lines of %s:\n' "$status" "$log" >&2
    tail -n 80 "$log" >&2
    exit "$status"
fi
