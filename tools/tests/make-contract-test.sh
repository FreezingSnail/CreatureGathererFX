#!/bin/sh
# Validate the public Make command API without requiring hardware/tool installs.
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd -P)
cd "$ROOT"

help=$(make --no-print-directory help)
for target in setup doctor gen test testvm build check fxtest fxtest-headless; do
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

fxtest=$(make --no-print-directory -n fxtest-build \
    ARDUINO_CLI=fixture-arduino FQBN=fixture:fx BUILD_DIR=build/contract)
printf '%s\n' "$fxtest" | grep -Fq 'stage="build/contract/fxtest/'
printf '%s\n' "$fxtest" | grep -Fq -- '--fqbn "fixture:fx"'

grep -Fxq 'fxtest: fxtest-headless' Makefile
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

pack=$(make --no-print-directory -n pack BUILD_DIR=build/contract DIST_DIR=dist/contract)
printf '%s\n' "$pack" | grep -Fq 'cgfx-tools.sh'
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
