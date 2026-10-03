#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd -P)
cd "$ROOT"
fixtures=tools/tests/fixtures/fxtest-ram
build=build/fxtest-ram-test
mkdir -p "$build"
: >"$build/fake.elf"
SIZE_FIXTURE="$ROOT/$fixtures/size-below.txt"
export SIZE_FIXTURE
tool="$ROOT/$fixtures/fake-avr-size.sh"

below=$(tools/check-fxtest-ram.sh "$build/fake.elf" "$tool" 101 below)
printf '%s\n' "$below" | grep -Fxq 'FXTEST_RAM_USED_BYTES=100'
printf '%s\n' "$below" | grep -Fxq 'FXTEST_RAM_FREE_BYTES=1'
equal=$(tools/check-fxtest-ram.sh "$build/fake.elf" "$tool" 100 equal)
printf '%s\n' "$equal" | grep -Fxq 'FXTEST_RAM_FREE_BYTES=0'

expect_failure() {
    label=$1
    shift
    log="$build/$label.log"
    if "$@" >"$log" 2>&1; then
        echo "fxtest RAM helper unexpectedly passed: $label" >&2
        exit 1
    fi
}

expect_failure over-budget tools/check-fxtest-ram.sh "$build/fake.elf" "$tool" 99 over
grep -Fq 'uses 100 B static RAM; limit is 99 B' "$build/over-budget.log"
expect_failure missing-elf tools/check-fxtest-ram.sh "$build/missing.elf" "$tool" 100 missing
grep -Fq 'ELF not found' "$build/missing-elf.log"
expect_failure missing-tool tools/check-fxtest-ram.sh "$build/fake.elf" "$build/no-avr-size" 100 tool
grep -Fq 'avr-size not found' "$build/missing-tool.log"
expect_failure invalid-budget tools/check-fxtest-ram.sh "$build/fake.elf" "$tool" 2x invalid
grep -Fq 'invalid RAM limit' "$build/invalid-budget.log"
expect_failure excessive-budget tools/check-fxtest-ram.sh "$build/fake.elf" "$tool" 9999 invalid
grep -Fq 'ATmega32u4 SRAM is 2560 B' "$build/excessive-budget.log"

SIZE_FIXTURE="$ROOT/$fixtures/size-missing.txt"
export SIZE_FIXTURE
expect_failure missing-sections tools/check-fxtest-ram.sh "$build/fake.elf" "$tool" 100 missing
grep -Fq 'missing, duplicate, or invalid' "$build/missing-sections.log"
SIZE_FIXTURE="$ROOT/$fixtures/size-duplicate.txt"
export SIZE_FIXTURE
expect_failure duplicate-sections tools/check-fxtest-ram.sh "$build/fake.elf" "$tool" 100 duplicate
grep -Fq 'missing, duplicate, or invalid' "$build/duplicate-sections.log"
SIZE_FIXTURE="$ROOT/$fixtures/size-invalid.txt"
export SIZE_FIXTURE
expect_failure invalid-sections tools/check-fxtest-ram.sh "$build/fake.elf" "$tool" 100 invalid
grep -Fq 'missing, duplicate, or invalid' "$build/invalid-sections.log"
SIZE_FIXTURE="$ROOT/$fixtures/size-below.txt"
SIZE_TOOL_FAIL=1
export SIZE_FIXTURE SIZE_TOOL_FAIL
expect_failure tool-failure tools/check-fxtest-ram.sh "$build/fake.elf" "$tool" 100 failing
grep -Fq 'fixture avr-size failure' "$build/tool-failure.log"

echo 'fxtest RAM guard: PASS'
