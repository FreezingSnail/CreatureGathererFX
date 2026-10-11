#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd -P)
tmp=$(mktemp -d "${TMPDIR:-/tmp}/cgfx-pack-stage.XXXXXX")
trap 'rm -rf "$tmp"' 0
trap 'exit 1' HUP INT TERM

fixture="$tmp/project"
mkdir -p "$fixture"/tools "$fixture"/bin "$fixture"/src "$fixture"/.git "$fixture"/.beads \
    "$fixture"/.codex "$fixture"/.battle-carve-spike "$fixture"/build/cache \
    "$fixture"/dist "$fixture"/.cache "$fixture"/harness/workspaces \
    "$fixture"/fxdata/generated
printf '%s\n' authored >"$fixture/authored.txt"
printf '%s\n' required >"$fixture/fxlayout.toml"
printf '%s\n' required >"$fixture/cgfx-project.json"
for path in .git/.private .beads/.private .codex/.private \
    .battle-carve-spike/.private build/cache/.private dist/.private \
    .cache/.private harness/workspaces/.private; do
    printf '%s\n' private >"$fixture/$path"
done

cat >"$fixture/bin/cgfx-tools" <<'MOCK'
#!/bin/sh
set -eu
[ "$1" = --project ]
stage=${2%/cgfx-project.json}
printf '%s\n' "$stage" >"$PACK_FIXTURE_STAGE_LOG"
for path in .git .beads .codex .battle-carve-spike build dist .cache harness/workspaces; do
    [ ! -e "$stage/$path" ] || { echo "pack stage retained $path" >&2; exit 31; }
done
[ -f "$stage/authored.txt" ]
if [ "${PACK_FIXTURE_FAIL:-0}" = 1 ]; then
    exit 32
fi
mkdir -p "$stage/src" "$stage/dist"
printf '%s\n' header >"$stage/src/fxdata.h"
printf '%s\n' packed >"$stage/dist/fxdata.bin"
printf '%s\n' data >"$stage/dist/fxdata-data.bin"
printf '%s\n' save >"$stage/dist/fxdata-save.bin"
MOCK
chmod +x "$fixture/bin/cgfx-tools"
for script in record-fxdata-manifest.sh assert-fxdata-manifest.sh; do
    cat >"$fixture/tools/$script" <<'MOCK'
#!/bin/sh
set -eu
[ -f dist/fxdata.bin ]
[ -f dist/fxdata-data.bin ]
[ -f dist/fxdata-save.bin ]
MOCK
    chmod +x "$fixture/tools/$script"
done

cd "$fixture"
PACK_FIXTURE_STAGE_LOG="$tmp/stage-success" PATH="$fixture/bin:$PATH" \
    make --no-print-directory -f "$ROOT/Makefile" pack \
        BUILD_DIR=build DIST_DIR=dist FX_LAYOUT=fxlayout.toml FXDATA_MANIFEST=manifest.json
stage=$(cat "$tmp/stage-success")
[ ! -e "$stage" ]
[ -f src/fxdata.h ] && [ -f dist/fxdata.bin ]

rm -rf build dist src
mkdir -p build dist
if PACK_FIXTURE_FAIL=1 PACK_FIXTURE_STAGE_LOG="$tmp/stage-failure" PATH="$fixture/bin:$PATH" \
    make --no-print-directory -f "$ROOT/Makefile" pack \
        BUILD_DIR=build DIST_DIR=dist FX_LAYOUT=fxlayout.toml FXDATA_MANIFEST=manifest.json; then
    echo 'pack failure fixture unexpectedly succeeded' >&2
    exit 1
fi
stage=$(cat "$tmp/stage-failure")
[ ! -e "$stage" ]
[ ! -e src/fxdata.h ] && [ ! -e dist/fxdata.bin ]

printf '%s\n' 'pack stage contract: PASS'
