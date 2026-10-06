#!/bin/sh
# Validate the public Make command API without requiring hardware/tool installs.
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd -P)
cd "$ROOT"

help=$(make --no-print-directory help)
for target in setup doctor gen test testvm build ram run dev arena-demo check final-gate fxtest fxtest-headless fxtest-spike test-fxtest-ram test-avr-build-budget; do
    printf '%s\n' "$help" | grep -Fq "  $target " || {
        printf 'missing help entry: %s\n' "$target" >&2
        exit 1
    }
done

case $help in
    *'/Users/'*|*'/home/'*)
        printf 'help exposes machine-specific path\n' >&2
        exit 1
        ;;
esac

# Exercise wrappers without compiling firmware or launching Ardens.
gate_dir="$ROOT/build/contract/gate"
mkdir -p "$gate_dir"
GATE_CALL_LOG="$gate_dir/calls.log"
export GATE_CALL_LOG
fixture_make="$ROOT/tools/tests/fixtures/gate-make.sh"
: >"$GATE_CALL_LOG"
spike=$(make --no-print-directory fxtest-spike MAKE="$fixture_make" \
    FXTEST_SPIKE_INO=tst/fxdatatest/test_dialog.ino ARDENS=fixture-ardens)
grep -Fxq -- '--no-print-directory -j1 fxtest-headless FXTEST_INOS=tst/fxdatatest/test_dialog.ino tst/fxdatatest/test_stack.ino' "$GATE_CALL_LOG"
: >"$GATE_CALL_LOG"
make --no-print-directory fxtest-spike MAKE="$fixture_make" \
    FXTEST_SPIKE_INO=tst/fxdatatest/test_stack.ino ARDENS=fixture-ardens >/dev/null
grep -Fxq -- '--no-print-directory -j1 fxtest-headless FXTEST_INOS=tst/fxdatatest/test_stack.ino' "$GATE_CALL_LOG"
for arguments in '' 'FXTEST_SPIKE_INO=missing.ino' 'FXTEST_SPIKE_INO=tst/fxdatatest/test_dialog.ino'; do
    if make --no-print-directory fxtest-spike MAKE="$fixture_make" ARDENS= $arguments >"$gate_dir/error.log" 2>&1; then
        echo 'spike accepted missing suite or Ardens' >&2
        exit 1
    fi
done
if make --no-print-directory final-gate ARDENS= >"$gate_dir/error.log" 2>&1; then
    echo 'final gate accepted missing Ardens' >&2
    exit 1
fi
: >"$GATE_CALL_LOG"
gate=$(make --no-print-directory -j4 final-gate MAKE="$fixture_make" \
    ARDENS=fixture-ardens FINAL_GATE_LOG_DIR="$gate_dir/logs" \
    FXTEST_INOS=tst/fxdatatest/test_dialog.ino)
test "$(awk '{print $3}' "$GATE_CALL_LOG")" = "$(printf 'check\nram')"
for suite in tst/fxdatatest/*.ino; do
    head -n 1 "$GATE_CALL_LOG" | grep -Fq "$suite"
done
printf '%s\n' "$gate" | grep -Fq 'Total Passed: 123'
printf '%s\n' "$gate" | grep -Fq 'RAM_FREE_BYTES=2360'
if printf '%s\n' "$gate" | grep -Fq 'fixture full diagnostics'; then
    echo 'gate leaked full success log into summary' >&2
    exit 1
fi
grep -Fq 'fixture full diagnostics' "$gate_dir/logs/check.log"
for stage in check ram; do
    : >"$GATE_CALL_LOG"
    if GATE_FAIL_STAGE="$stage" make --no-print-directory final-gate MAKE="$fixture_make" \
        ARDENS=fixture-ardens FINAL_GATE_LOG_DIR="$gate_dir/logs" >"$gate_dir/error.log" 2>&1; then
        echo 'final gate swallowed failure' >&2
        exit 1
    fi
    grep -Fq 'fixture failure detail' "$gate_dir/error.log"
    grep -Fq 'FAIL (exit 7)' "$gate_dir/error.log"
    if [ "$stage" = check ]; then
        test "$(wc -l <"$GATE_CALL_LOG" | tr -d ' ')" -eq 1
    fi
done

# Arena target phases are sequential, keep flags/budget/output isolated, and
# stop before building if generation fails.
: >"$GATE_CALL_LOG"
make --no-print-directory arena-demo MAKE="$fixture_make" \
    ARENA_DEMO_BUILD_DIR=build/arena-contract \
    FXDATA_DATA_BIN=build/arena-contract/fxdata-data.bin \
    FXDATA_SAVE_BIN=build/arena-contract/fxdata-save.bin \
    ARDENS=/bin/true \
    AVR_SHIPPING_CPP_FLAGS='-mrelax -DCGFX_SHIPPING_NO_USB -DLOCAL_EXTRA' >/dev/null
test "$(awk '{print $2}' "$GATE_CALL_LOG")" = "$(printf 'gen\nram')"
grep -Fq 'BUILD_DIR=build/arena-contract' "$GATE_CALL_LOG"
grep -Fq 'AVR_SHIPPING_CPP_FLAGS=-mrelax -DCGFX_SHIPPING_NO_USB -DLOCAL_EXTRA -DCGFX_ARENA_DEMO' "$GATE_CALL_LOG"
grep -Fq 'AVR_FLASH_BUDGET=29184' "$GATE_CALL_LOG"
: >"$GATE_CALL_LOG"
if GATE_FAIL_STAGE=gen make --no-print-directory arena-demo MAKE="$fixture_make" \
    ARENA_DEMO_BUILD_DIR=build/arena-contract ARDENS=/bin/true \
    FXDATA_DATA_BIN=build/arena-contract/fxdata-data.bin \
    FXDATA_SAVE_BIN=build/arena-contract/fxdata-save.bin >"$gate_dir/error.log" 2>&1; then
    echo 'arena-demo swallowed generation failure' >&2
    exit 1
fi
test "$(wc -l <"$GATE_CALL_LOG" | tr -d ' ')" -eq 1
grep -Fq 'fixture failure detail' "$gate_dir/error.log"
: >"$GATE_CALL_LOG"
if GATE_FAIL_STAGE=ram make --no-print-directory arena-demo MAKE="$fixture_make" \
    ARENA_DEMO_BUILD_DIR=build/arena-contract ARDENS=/bin/true \
    FXDATA_DATA_BIN=build/arena-contract/fxdata-data.bin \
    FXDATA_SAVE_BIN=build/arena-contract/fxdata-save.bin >"$gate_dir/error.log" 2>&1; then
    echo 'arena-demo swallowed RAM/build failure' >&2
    exit 1
fi
test "$(wc -l <"$GATE_CALL_LOG" | tr -d ' ')" -eq 2
grep -Fq 'fixture failure detail' "$gate_dir/error.log"

arena_dry=$(make --no-print-directory -n arena-demo BUILD_DIR=build/ignored \
    ARENA_DEMO_BUILD_DIR=build/arena-contract ARDENS=/bin/true)
printf '%s\n' "$arena_dry" | grep -Fq 'gen'
printf '%s\n' "$arena_dry" | grep -Fq 'ram BUILD_DIR="build/arena-contract"'
printf '%s\n' "$arena_dry" | grep -Fq 'AVR_FLASH_BUDGET="29184"'
printf '%s\n' "$arena_dry" | grep -Fq 'file="build/arena-demo/CreatureGathererFX.ino.hex"'
printf '%s\n' "$arena_dry" | grep -Fq 'save="build/arena-demo/isolated/fxdata-save.bin"'

build=$(make --no-print-directory -n build \
    ARDUINO_CLI=fixture-arduino FQBN=fixture:fx BUILD_DIR=build/contract)
printf '%s\n' "$build" | grep -Fq 'fixture-arduino compile --fqbn "fixture:fx"'
printf '%s\n' "$build" | grep -Fq 'ARDUINO_BUILD_CACHE_PATH="build/contract/arduino-cache" fixture-arduino compile'
printf '%s\n' "$build" | grep -Fq -- '--build-path "build/contract/arduino-build/fx"'
printf '%s\n' "$build" | grep -Fq -- '--output-dir "build/contract"'
printf '%s\n' "$build" | grep -Fq -- '--build-property "compiler.cpp.extra_flags=-mrelax -mcall-prologues -fno-move-loop-invariants -mstrict-X -DCGFX_SHIPPING_NO_USB"'
for property in compiler.c.extra_flags compiler.c.elf.extra_flags; do
    printf '%s\n' "$build" | grep -Fq -- "--build-property \"$property=-mrelax -mcall-prologues -fno-move-loop-invariants -mstrict-X\""
done
printf '%s\n' "$build" | grep -Fq 'check-avr-build-budget.sh "$log" "29696" "2160" FX'
grep -Fq 'if (initVariant != nullptr)' CreatureGathererFX.ino
grep -Fq 'Arduboy2Core::exitToBootloader()' CreatureGathererFX.ino
if rg -n 'CGFX_SHIPPING_NO_USB|ARDUBOY_NO_USB|int main\(' tst/fxdatatest -g '*.ino'; then
    printf 'shipping USB-free entry point leaked into FX serial suites\n' >&2
    exit 1
fi
grep -Fq 'fxTestSetup();' tst/fxdatatest/fxdatatest.ino
grep -Fq 'Serial.begin(9600)' tst/fxdatatest/harness/fx_globals.hpp

mini=$(make --no-print-directory -n mini \
    ARDUINO_CLI=fixture-arduino MINI_FQBN=fixture:mini BUILD_DIR=build/contract)
printf '%s\n' "$mini" | grep -Fq 'fixture-arduino compile --fqbn "fixture:mini"'
printf '%s\n' "$mini" | grep -Fq -- '--build-path "build/contract/arduino-build/mini"'
printf '%s\n' "$mini" | grep -Fq -- '--output-dir "build/contract"'
printf '%s\n' "$mini" | grep -Fq -- '--build-property "compiler.cpp.extra_flags=-mrelax -mcall-prologues -fno-move-loop-invariants -mstrict-X -DCGFX_SHIPPING_NO_USB"'
for property in compiler.c.extra_flags compiler.c.elf.extra_flags; do
    printf '%s\n' "$mini" | grep -Fq -- "--build-property \"$property=-mrelax -mcall-prologues -fno-move-loop-invariants -mstrict-X\""
done
printf '%s\n' "$mini" | grep -Fq 'check-avr-build-budget.sh "$log" "29696" "2160" Mini'

override=$(make --no-print-directory -n build \
    ARDUINO_CLI=fixture-arduino FQBN=fixture:fx BUILD_DIR=build/contract \
    AVR_SHIPPING_BUILD_PROPERTIES='--build-property compiler.cpp.extra_flags=-DLOCAL')
printf '%s\n' "$override" | grep -Fq -- '--build-property compiler.cpp.extra_flags=-DLOCAL'
if printf '%s\n' "$override" | grep -Fq -- 'CGFX_SHIPPING_NO_USB'; then
    printf 'AVR_SHIPPING_BUILD_PROPERTIES override was ignored\n' >&2
    exit 1
fi

device_override=$(make --no-print-directory -n fxtest-build \
    ARDUINO_CLI=fixture-arduino BUILD_DIR=build/contract \
    FXTEST_INOS=tst/fxdatatest/test_dialog.ino \
    AVR_FXTEST_BUILD_PROPERTIES='--build-property compiler.cpp.extra_flags=-DLOCAL')
printf '%s\n' "$device_override" | grep -Fq -- '--build-property compiler.cpp.extra_flags=-DLOCAL'
if printf '%s\n' "$device_override" | grep -Fq -- 'CGFX_SHIPPING_NO_USB'; then
    printf 'shipping USB-free flags leaked into FX device-test builds\n' >&2
    exit 1
fi

ram=$(make --no-print-directory -n ram \
    ARDUINO_CLI=fixture-arduino BUILD_DIR=build/contract \
    AVR_SIZE=fixture-size AVR_NM=fixture-nm)
printf '%s\n' "$ram" | grep -Fq 'fixture-arduino compile --fqbn'
printf '%s\n' "$ram" | grep -Fq 'elf="build/contract/CreatureGathererFX.ino.elf"'
printf '%s\n' "$ram" | grep -Fq 'fixture-size" --format=avr --mcu=atmega32u4'
printf '%s\n' "$ram" | grep -Fq 'fixture-nm" --print-size --size-sort --radix=d'
for key in RAM_ELF RAM_FLASH_BYTES RAM_FLASH_LIMIT RAM_STATIC_BYTES RAM_STATIC_LIMIT RAM_FREE_BYTES; do
    printf '%s\n' "$ram" | grep -Fq "$key=" || {
        printf 'ram target missing machine-readable key: %s\n' "$key" >&2
        exit 1
    }
done
printf '%s\n' "$ram" | grep -Fq 'RAM_TOP_SYMBOLS_BEGIN'
printf '%s\n' "$ram" | grep -Fq 'address size type name'
printf '%s\n' "$ram" | grep -Fq 'RAM_TOP_SYMBOLS_END'
if printf '%s\n' "$ram" | grep -Fq '7.3.0-atmel3.6.1-arduino7'; then
    printf 'ram target pins an AVR-GCC toolchain version\n' >&2
    exit 1
fi

fxtest=$(make --no-print-directory -n fxtest-build \
    ARDUINO_CLI=fixture-arduino FQBN=fixture:fx BUILD_DIR=build/contract)
printf '%s\n' "$fxtest" | grep -Fq 'stage="build/contract/fxtest/'
printf '%s\n' "$fxtest" | grep -Fq -- '--fqbn "fixture:fx"'
printf '%s\n' "$fxtest" | grep -Fq -- '--build-path "$stage/build"'
printf '%s\n' "$fxtest" | grep -Fq 'ARDUINO_BUILD_CACHE_PATH="build/contract/arduino-cache" fixture-arduino compile'
printf '%s\n' "$fxtest" | grep -Fq 'check-fxtest-ram.sh "$stage/build/$ino.ino.elf"'
printf '%s\n' "$fxtest" | grep -Fq 'check-fxtest-ram.sh "$stage/build/$ino.ino.elf"'
for property in compiler.cpp.extra_flags compiler.c.extra_flags compiler.c.elf.extra_flags; do
    if [ "$property" = compiler.cpp.extra_flags ]; then
        printf '%s\n' "$fxtest" | grep -Fq -- '--build-property "compiler.cpp.extra_flags=-mrelax -mcall-prologues -fno-move-loop-invariants -mstrict-X -DFX_READ_COUNTER"'
    else
        printf '%s\n' "$fxtest" | grep -Fq -- "--build-property \"$property=-mrelax -mcall-prologues -fno-move-loop-invariants -mstrict-X\""
    fi
done

grep -Fxq 'fxtest: fxtest-headless' Makefile

# dev = gen, then the interactive Ardens run, sequenced through a sub-make.
grep -Fxq 'dev: gen' Makefile
dev=$(make --no-print-directory -n dev \
    ARDENS=fixture-ardens ARDUINO_CLI=fixture-arduino BUILD_DIR=build/contract)
printf '%s\n' "$dev" | grep -Fq 'cgfx-tools --project cgfx-project.json'
printf '%s\n' "$dev" | grep -Fq 'fixture-arduino compile'
printf '%s\n' "$dev" | grep -Fq 'file="build/contract/CreatureGathererFX.ino.hex"'
printf '%s\n' "$dev" | grep -Fq 'save="dist/fxdata-save.bin"'
grep -Fxq 'FXTEST_INOS ?= $(wildcard tst/fxdatatest/*.ino)' Makefile
grep -Fxq 'fxtest-headless:' Makefile
grep -Fq 'strings "$(ARDENS)" | grep -Fxq captureserial' Makefile

headless_run=$(make --no-print-directory -n fxtest-run \
    ARDENS=fixture-ardens FXDATA_BIN=dist/fixture-fxdata.bin \
    BUILD_DIR=build/contract)
printf '%s\n' "$headless_run" | grep -Fq 'fixture-ardens captureserial=3000 fxport=d1 display=ssd1306'
printf '%s\n' "$headless_run" | grep -Fq 'cp -f "dist/fixture-fxdata.bin" "$stage/fxdata.bin"'
printf '%s\n' "$headless_run" | grep -Fq 'file=$stage/fxdata.bin'
printf '%s\n' "$headless_run" | grep -Fq "grep -qx 'F'"
printf '%s\n' "$headless_run" | grep -Fq "grep -qx 'P'"
if printf '%s\n' "$headless_run" | grep -Eq '(^|[;&|[:space:]])open([[:space:];]|$)'; then
    printf 'fxtest runner launches graphical Ardens\n' >&2
    exit 1
fi

host=$(make --no-print-directory -n test BUILD_DIR=build/contract)
gen=$(make --no-print-directory -n gen BUILD_DIR=build/contract DIST_DIR=dist/contract)
if printf '%s\n' "$gen" | grep -Fq 'python3 '; then
    printf 'gen retains Python dependency\n' >&2
    exit 1
fi

pack=$(make --no-print-directory -n pack BUILD_DIR=build/contract DIST_DIR=dist/contract)
printf '%s\n' "$pack" | grep -Fq 'cgfx-tools --project "$stage/cgfx-project.json"'
printf '%s\n' "$pack" | grep -Fq -- '--pack --layout "$stage/fxlayout.toml"'
printf '%s\n' "$pack" | grep -Fq '"dist/contract/fxdata.bin"'
if printf '%s\n' "$pack" | grep -Fq 'fxdata-build.py fxdata/fxdata.txt'; then
    printf 'pack retains direct legacy Python generation\n' >&2
    exit 1
fi
printf '%s\n' "$host" | grep -Fq -- '-o "build/contract/tests/host"'
if printf '%s\n' "$host" | grep -Fq -- '-I/src'; then
    printf 'host build retains machine-root include path\n' >&2
    exit 1
fi

printf '%s\n' 'make contract: PASS'
