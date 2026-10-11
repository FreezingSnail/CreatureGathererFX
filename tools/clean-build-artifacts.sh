#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P)
HERE=$(pwd -P)
if [ "$HERE" != "$ROOT" ]; then
    echo "clean-build-artifacts: run from the repository root: $ROOT" >&2
    exit 2
fi

BUILD_DIR=${BUILD_DIR:-build}
case "$BUILD_DIR" in
    build|build/*) ;;
    *) echo "clean-build-artifacts: BUILD_DIR must be build or a path below build/" >&2; exit 2 ;;
esac
case "/$BUILD_DIR/" in
    */../*|*//*) echo "clean-build-artifacts: invalid BUILD_DIR: $BUILD_DIR" >&2; exit 2 ;;
esac

MODE=dry-run
if [ "$#" -gt 1 ]; then
    echo "usage: $0 [--apply]" >&2
    exit 2
fi
if [ "$#" -eq 1 ]; then
    if [ "$1" != "--apply" ]; then
        echo "usage: $0 [--apply]" >&2
        exit 2
    fi
    MODE=apply
fi

BUILD_ROOT="$ROOT/$BUILD_DIR"
if [ -L "$ROOT/build" ] || [ -L "$BUILD_ROOT" ]; then
    echo "clean-build-artifacts: refusing symlink build directory: $BUILD_ROOT" >&2
    exit 2
fi

candidate_bytes=0
candidate_count=0
measure_path() {
    path=$1
    [ -e "$path" ] || return 0
    if [ -L "$path" ]; then
        echo "clean-build-artifacts: refusing symlink candidate: $path" >&2
        exit 2
    fi
    kb=$(du -sk "$path" | awk 'NR == 1 {print $1}')
    candidate_bytes=$((candidate_bytes + kb * 1024))
    candidate_count=$((candidate_count + 1))
    printf '%s  %s  %s KiB\n' "$MODE" "$path" "$kb"
    if [ "$MODE" = apply ]; then
        rm -rf "$path"
    fi
}

measure_file() {
    path=$1
    [ -e "$path" ] || return 0
    if [ -L "$path" ] || [ ! -f "$path" ]; then
        echo "clean-build-artifacts: refusing non-regular compiler output: $path" >&2
        exit 2
    fi
    bytes=$(wc -c < "$path" | tr -d ' ')
    candidate_bytes=$((candidate_bytes + bytes))
    candidate_count=$((candidate_count + 1))
    printf '%s  %s  %s bytes\n' "$MODE" "$path" "$bytes"
    if [ "$MODE" = apply ]; then
        rm -f "$path"
    fi
}

# These are generated Arduino compiler/cache paths. Keep each sketch copy's
# source and reports, plus all ELFs, HEX files, save images, and other evidence.
measure_path "$BUILD_ROOT/arduino-cache"
for variant in fx mini; do
    if [ -L "$BUILD_ROOT/arduino-build" ] || [ -L "$BUILD_ROOT/arduino-build/$variant" ]; then
        echo "clean-build-artifacts: refusing symlink Arduino build path: $BUILD_ROOT/arduino-build/$variant" >&2
        exit 2
    fi
    variant_root="$BUILD_ROOT/arduino-build/$variant"
    measure_path "$variant_root/core"
    measure_path "$variant_root/libraries"
    sketch="$variant_root/sketch"
    if [ -d "$sketch" ] && [ ! -L "$sketch" ]; then
        while IFS= read -r file; do
            measure_file "$file"
        done <<EOF
$(find "$sketch" -type f \( -name '*.o' -o -name '*.d' \) -print | sort)
EOF
    elif [ -L "$sketch" ]; then
        echo "clean-build-artifacts: refusing symlink sketch directory: $sketch" >&2
        exit 2
    fi
done

if [ "$candidate_count" -eq 0 ]; then
    echo "clean-build-artifacts: no eligible generated artifacts under $BUILD_ROOT"
else
    printf 'clean-build-artifacts: %s candidates, %s bytes reported by du/wc (%s)\n' \
        "$candidate_count" "$candidate_bytes" "$MODE"
fi
