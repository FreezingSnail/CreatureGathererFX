#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd -P)
cd "$ROOT"
fixtures=tools/tests/fixtures/avr-build-budget
build=build/avr-build-budget-test
mkdir -p "$build"

below=$(tools/check-avr-build-budget.sh "$fixtures/pass.log" 1001 1201 FX)
printf '%s\n' "$below" | grep -Fxq 'AVR_FLASH_FREE_BYTES=1'
printf '%s\n' "$below" | grep -Fxq 'AVR_STATIC_RAM_FREE_BYTES=1'
equal=$(tools/check-avr-build-budget.sh "$fixtures/pass.log" 1000 1200 FX)
printf '%s\n' "$equal" | grep -Fxq 'AVR_FLASH_FREE_BYTES=0'
printf '%s\n' "$equal" | grep -Fxq 'AVR_STATIC_RAM_FREE_BYTES=0'

expect_failure() {
    label=$1
    shift
    log="$build/$label.log"
    if "$@" >"$log" 2>&1; then
        echo "AVR budget checker unexpectedly passed: $label" >&2
        exit 1
    fi
}

expect_failure flash-over tools/check-avr-build-budget.sh "$fixtures/pass.log" 999 1200 FX
grep -Fq 'uses 1000 B flash; limit is 999 B' "$build/flash-over.log"
expect_failure ram-over tools/check-avr-build-budget.sh "$fixtures/pass.log" 1000 1199 FX
grep -Fq 'uses 1200 B static RAM; limit is 1199 B' "$build/ram-over.log"
expect_failure invalid-flash tools/check-avr-build-budget.sh "$fixtures/pass.log" 1x 1200 FX
grep -Fq 'invalid flash limit' "$build/invalid-flash.log"
expect_failure excessive-flash tools/check-avr-build-budget.sh "$fixtures/pass.log" 30000 1200 FX
grep -Fq 'ATmega32u4 flash is 29696 B' "$build/excessive-flash.log"
expect_failure missing-log tools/check-avr-build-budget.sh "$build/missing.log" 1000 1200 FX
grep -Fq 'build log not found' "$build/missing-log.log"
expect_failure missing-measurement tools/check-avr-build-budget.sh "$fixtures/missing.log" 1000 1200 FX
grep -Fq 'missing, duplicate, or has unparseable' "$build/missing-measurement.log"
expect_failure duplicate-measurement tools/check-avr-build-budget.sh "$fixtures/duplicate.log" 1000 1200 FX
grep -Fq 'missing, duplicate, or has unparseable' "$build/duplicate-measurement.log"
expect_failure invalid-measurement tools/check-avr-build-budget.sh "$fixtures/invalid.log" 1000 1200 FX
grep -Fq 'missing, duplicate, or has unparseable' "$build/invalid-measurement.log"

echo 'AVR build budget: PASS'
