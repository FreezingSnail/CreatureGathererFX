#!/bin/sh
# Validate the public Make command API without requiring hardware/tool installs.
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd -P)
cd "$ROOT"

help=$(make --no-print-directory help)
for target in setup doctor gen test testvm build ram run dev check fxtest fxtest-headless; do
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

build=$(make --no-print-directory -n build \
    ARDUINO_CLI=fixture-arduino FQBN=fixture:fx BUILD_DIR=build/contract)
printf '%s\n' "$build" | grep -Fq 'fixture-arduino compile --fqbn "fixture:fx"'
printf '%s\n' "$build" | grep -Fq -- '--output-dir "build/contract"'
for property in compiler.cpp.extra_flags compiler.c.extra_flags compiler.c.elf.extra_flags; do
    printf '%s\n' "$build" | grep -Fq -- "--build-property $property=-mrelax"
done

mini=$(make --no-print-directory -n mini \
    ARDUINO_CLI=fixture-arduino MINI_FQBN=fixture:mini BUILD_DIR=build/contract)
printf '%s\n' "$mini" | grep -Fq 'fixture-arduino compile --fqbn "fixture:mini"'
printf '%s\n' "$mini" | grep -Fq -- '--output-dir "build/contract"'
for property in compiler.cpp.extra_flags compiler.c.extra_flags compiler.c.elf.extra_flags; do
    printf '%s\n' "$mini" | grep -Fq -- "--build-property $property=-mrelax"
done

override=$(make --no-print-directory -n build \
    ARDUINO_CLI=fixture-arduino FQBN=fixture:fx BUILD_DIR=build/contract \
    AVR_BUILD_PROPERTIES='--build-property compiler.cpp.extra_flags=-DLOCAL')
printf '%s\n' "$override" | grep -Fq -- '--build-property compiler.cpp.extra_flags=-DLOCAL'
if printf '%s\n' "$override" | grep -Fq -- 'compiler.cpp.extra_flags=-mrelax'; then
    printf 'AVR_BUILD_PROPERTIES override was ignored\n' >&2
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
for property in compiler.cpp.extra_flags compiler.c.extra_flags compiler.c.elf.extra_flags; do
    printf '%s\n' "$fxtest" | grep -Fq -- "--build-property $property=-mrelax"
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
