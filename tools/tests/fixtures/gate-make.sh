#!/bin/sh
# Fake recursive make for command selection, ordering, and failure tests.
set -eu
printf '%s\n' "$*" >>"$GATE_CALL_LOG"
for argument do
    case "$argument" in
        check|gen|ram|fxtest-headless) stage=$argument ;;
    esac
done
printf 'fixture full diagnostics for %s\n' "$stage"
if [ "${GATE_FAIL_STAGE:-}" = "$stage" ]; then
    echo 'fixture failure detail'
    exit 7
fi
case "$stage" in
    check) printf 'Total Passed: 123\nTotal Failed: 0\nfixture: PASS\n' ;;
    ram) printf 'RAM_FLASH_BYTES=100\nRAM_STATIC_BYTES=200\nRAM_FREE_BYTES=2360\n' ;;
esac
if [ "$stage" = gen ]; then
    for argument do
        case "$argument" in
            FXDATA_DATA_BIN=*) data_path=${argument#*=} ;;
            FXDATA_SAVE_BIN=*) save_path=${argument#*=} ;;
        esac
    done
    [ -z "${data_path:-}" ] || { mkdir -p "$(dirname "$data_path")"; : >"$data_path"; }
    [ -z "${save_path:-}" ] || { mkdir -p "$(dirname "$save_path")"; : >"$save_path"; }
fi
if [ "$stage" = ram ]; then
    for argument do
        case "$argument" in
            BUILD_DIR=*) build_dir=${argument#*=} ;;
        esac
    done
    [ -z "${build_dir:-}" ] || { mkdir -p "$build_dir"; : >"$build_dir/CreatureGathererFX.ino.hex"; }
fi
