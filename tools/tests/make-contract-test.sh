#!/bin/sh
# Validate the public Make command API without requiring hardware/tool installs.
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd -P)
cd "$ROOT"

help=$(make --no-print-directory help)
for target in setup doctor gen test testvm build check fxtest; do
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

host=$(make --no-print-directory -n test BUILD_DIR=build/contract)
printf '%s\n' "$host" | grep -Fq -- '-o "build/contract/tests/host"'
if printf '%s\n' "$host" | grep -Fq -- '-I/src'; then
    printf 'host build retains machine-root include path\n' >&2
    exit 1
fi

printf '%s\n' 'make contract: PASS'
