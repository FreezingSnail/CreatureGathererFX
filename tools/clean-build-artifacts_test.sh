#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P)
cd "$ROOT"
fixture=$(mktemp -d "$ROOT/build/cleanup-test.XXXXXX")
fixture_rel=${fixture#"$ROOT/"}
trap 'rm -rf "$fixture"' EXIT HUP INT TERM

mkdir -p "$fixture/arduino-cache" \
    "$fixture/arduino-build/fx/core" \
    "$fixture/arduino-build/fx/libraries" \
    "$fixture/arduino-build/fx/sketch" \
    "$fixture/scratch-clone/src"
printf cache >"$fixture/arduino-cache/cache.bin"
printf object >"$fixture/arduino-build/fx/core/core.a"
printf object >"$fixture/arduino-build/fx/libraries/lib.o"
printf object >"$fixture/arduino-build/fx/sketch/sketch.o"
printf dependency >"$fixture/arduino-build/fx/sketch/sketch.cpp.d"
printf source >"$fixture/arduino-build/fx/sketch/source.cpp"
printf report >"$fixture/arduino-build/fx/sketch/output.md"
printf csv >"$fixture/arduino-build/fx/sketch/measurement.csv"
printf shipping-elf >"$fixture/CreatureGathererFX.ino.elf"
printf control-elf >"$fixture/control.elf"
printf save-image >"$fixture/fxdata-save.bin"
printf source >"$fixture/scratch-clone/src/main.cpp"

preview=$(BUILD_DIR="$fixture_rel" ./tools/clean-build-artifacts.sh)
printf '%s\n' "$preview" | grep -Fq 'dry-run'
for file in \
    "$fixture/arduino-cache/cache.bin" \
    "$fixture/arduino-build/fx/core/core.a" \
    "$fixture/arduino-build/fx/libraries/lib.o" \
    "$fixture/arduino-build/fx/sketch/sketch.o" \
    "$fixture/arduino-build/fx/sketch/sketch.cpp.d" \
    "$fixture/CreatureGathererFX.ino.elf" \
    "$fixture/control.elf" \
    "$fixture/fxdata-save.bin"; do
    test -f "$file" || { echo "dry run changed $file" >&2; exit 1; }
done

BUILD_DIR="$fixture_rel" ./tools/clean-build-artifacts.sh --apply >/dev/null
for file in \
    "$fixture/arduino-cache" \
    "$fixture/arduino-build/fx/core" \
    "$fixture/arduino-build/fx/libraries" \
    "$fixture/arduino-build/fx/sketch/sketch.o" \
    "$fixture/arduino-build/fx/sketch/sketch.cpp.d"; do
    test ! -e "$file" || { echo "apply left generated artifact $file" >&2; exit 1; }
done
for file in \
    "$fixture/arduino-build/fx/sketch/source.cpp" \
    "$fixture/arduino-build/fx/sketch/output.md" \
    "$fixture/arduino-build/fx/sketch/measurement.csv" \
    "$fixture/CreatureGathererFX.ino.elf" \
    "$fixture/control.elf" \
    "$fixture/fxdata-save.bin" \
    "$fixture/scratch-clone/src/main.cpp"; do
    test -f "$file" || { echo "apply removed protected evidence $file" >&2; exit 1; }
done

mkdir -p "$fixture/victim"
printf keep >"$fixture/victim/keep.txt"
ln -s "$fixture/victim" "$fixture/arduino-build/fx/core"
if BUILD_DIR="$fixture_rel" ./tools/clean-build-artifacts.sh --apply >"$fixture/symlink.log" 2>&1; then
    echo 'cleanup accepted a symlink compiler directory' >&2
    exit 1
fi
test -f "$fixture/victim/keep.txt" || { echo 'symlink target was modified' >&2; exit 1; }

if (cd "$fixture" && BUILD_DIR="$fixture_rel" "$ROOT/tools/clean-build-artifacts.sh") >"$fixture/cwd.log" 2>&1; then
    echo 'cleanup accepted a non-repository working directory' >&2
    exit 1
fi
grep -Fq 'run from the repository root' "$fixture/cwd.log"

echo 'clean-build-artifacts: dry run, bounded deletion, evidence preservation, symlink guard, and cwd guard passed'
