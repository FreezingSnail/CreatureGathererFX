#!/usr/bin/env bash
set -euo pipefail

root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd -P)
tmp_parent=${TMPDIR:-/tmp}
tmp=$(mktemp -d "${tmp_parent%/}/cgfx-ensure-generated.XXXXXX")
trap 'rm -rf "$tmp"' EXIT
fixture="$tmp/project"
mkdir -p "$fixture"
fixture=$(CDPATH= cd -- "$fixture" && pwd -P)
mkdir -p "$fixture"/bin "$fixture"/build/gen-cache "$fixture"/data \
    "$fixture"/maps "$fixture"/images "$fixture"/fxdata/generated \
    "$fixture"/src/vm "$fixture"/src/flags "$fixture"/dist "$fixture"/tools
for path in Makefile tools/ensure-generated.sh tools/fxdata-manifest-lib.sh \
    tools/record-fxdata-manifest.sh tools/assert-fxdata-manifest.sh \
    tools/emit-rust-teams.sh tools/emit-tool-version-stamp.sh; do
    cp -f "$root/$path" "$fixture/$path"
done
printf '%s\n' '{}' >"$fixture/cgfx-project.json"
printf '%s\n' 'layout fixture' >"$fixture/fxlayout.toml"
printf '%s\n' 'sprite config' >"$fixture/fxsprites.toml"
printf '%s\n' 'authored input' >"$fixture/data/source.txt"
printf '%s\n' 'map input' >"$fixture/maps/zone.txt"
printf '%s\n' 'image input' >"$fixture/images/sprite.bin"

for path in opcodes.hpp flags.hpp flag_bit_array.hpp flag_bit_array.cpp; do
    printf 'generated %s\n' "$path" >"$fixture/fxdata/generated/$path"
done
printf '%s\n' '{' '  "generator":{"name":"cgfx-tools","version":"fixture"}' '}' \
    >"$fixture/fxdata/generated/manifest.json"
cp -f "$fixture/fxdata/generated/opcodes.hpp" "$fixture/src/vm/opcodes.hpp"
cp -f "$fixture/fxdata/generated/flags.hpp" "$fixture/src/flags/flags.hpp"
cp -f "$fixture/fxdata/generated/flag_bit_array.hpp" "$fixture/src/flags/flag_bit_array.hpp"
cp -f "$fixture/fxdata/generated/flag_bit_array.cpp" "$fixture/src/flags/flag_bit_array.cpp"
printf '%s\n' 'published header' >"$fixture/src/fxdata.h"
printf '%s\n' image >"$fixture/dist/fxdata.bin"
printf '%s\n' data >"$fixture/dist/fxdata-data.bin"
printf '%s\n' save >"$fixture/dist/fxdata-save.bin"

cat >"$fixture/bin/cgfx-tools" <<'MOCK'
#!/bin/sh
exit 0
MOCK
cat >"$fixture/bin/mock-gen" <<'MOCK'
#!/bin/sh
set -eu
printf 'run\n' >>"$GEN_LOG"
test "${GEN_FAIL:-0}" = 0 || exit 42
MOCK
chmod +x "$fixture/bin/cgfx-tools" "$fixture/bin/mock-gen"
(cd "$fixture" && ./tools/record-fxdata-manifest.sh fxlayout.toml \
    fxdata/generated/manifest.json)

stamp=build/gen-cache/fingerprint
tool_path="$fixture/bin:$PATH"
check() {
    PATH="$tool_path" "$fixture/tools/ensure-generated.sh" --check "$fixture" "$stamp" \
        fxlayout.toml fxdata/generated/manifest.json dist dist dist/fxdata.bin \
        dist/fxdata-data.bin dist/fxdata-save.bin
}
record_stamp() {
    PATH="$tool_path" "$fixture/tools/ensure-generated.sh" --record "$fixture" "$stamp" \
        fxlayout.toml fxdata/generated/manifest.json dist dist dist/fxdata.bin \
        dist/fxdata-data.bin dist/fxdata-save.bin
}
ensure() {
    PATH="$tool_path" "$fixture/tools/ensure-generated.sh" --ensure "$fixture" "$stamp" \
        fxlayout.toml fxdata/generated/manifest.json dist dist dist/fxdata.bin \
        dist/fxdata-data.bin dist/fxdata-save.bin -- "$@"
}
must_be_stale() {
    if check >"$tmp/check.out" 2>&1; then
        echo "expected generated cache miss: $1" >&2
        exit 1
    fi
}
refresh_manifest_and_stamp() {
    (cd "$fixture" && ./tools/record-fxdata-manifest.sh fxlayout.toml \
        fxdata/generated/manifest.json)
    record_stamp
}
stamp_mtime() {
    stat -f %m "$fixture/$stamp" 2>/dev/null || stat -c %Y "$fixture/$stamp"
}

# Cold cache invokes the supplied generation command, but never stamps its failure.
if GEN_LOG="$tmp/gen.log" GEN_FAIL=1 ensure "$fixture/bin/mock-gen"; then
    echo 'cold failed generation unexpectedly succeeded' >&2
    exit 1
fi
test ! -e "$fixture/$stamp"
test "$(wc -l <"$tmp/gen.log" | tr -d ' ')" -eq 1
GEN_LOG="$tmp/gen.log" GEN_FAIL=0 ensure "$fixture/bin/mock-gen"
test -f "$fixture/$stamp"
grep -Eq '^[a-f0-9]{64}$' "$fixture/$stamp"
test "$(wc -l <"$tmp/gen.log" | tr -d ' ')" -eq 2
check

# Warm reuse leaves both the cache stamp and generated outputs untouched.
touch -t 200001010000 "$fixture/$stamp" "$fixture/src/fxdata.h"
before_stamp=$(stamp_mtime)
before_header=$(stat -f %m "$fixture/src/fxdata.h" 2>/dev/null || stat -c %Y "$fixture/src/fxdata.h")
GEN_LOG="$tmp/gen.log" GEN_FAIL=1 ensure "$fixture/bin/mock-gen"
test "$(wc -l <"$tmp/gen.log" | tr -d ' ')" -eq 2
test "$(stamp_mtime)" = "$before_stamp"
test "$(stat -f %m "$fixture/src/fxdata.h" 2>/dev/null || stat -c %Y "$fixture/src/fxdata.h")" = "$before_header"

# Manifest validation rejects edited, added, and deleted authored inputs.
printf 'edited\n' >>"$fixture/data/source.txt"
must_be_stale 'edited source'
printf '%s\n' 'authored input' >"$fixture/data/source.txt"
printf '%s\n' extra >"$fixture/data/added.txt"
must_be_stale 'added source'
rm -f "$fixture/data/added.txt"
refresh_manifest_and_stamp
rm -f "$fixture/maps/zone.txt"
must_be_stale 'deleted source'
printf '%s\n' 'map input' >"$fixture/maps/zone.txt"
refresh_manifest_and_stamp

# Manifest validation catches missing/tampered generated and packed artifacts.
cp -f "$fixture/fxdata/generated/flags.hpp" "$tmp/flags.hpp"
rm -f "$fixture/fxdata/generated/flags.hpp"
must_be_stale 'missing generated output'
cp -f "$tmp/flags.hpp" "$fixture/fxdata/generated/flags.hpp"
refresh_manifest_and_stamp
printf 'tamper\n' >>"$fixture/dist/fxdata-save.bin"
must_be_stale 'tampered packed image'
printf '%s\n' save >"$fixture/dist/fxdata-save.bin"
refresh_manifest_and_stamp

# Adding and deleting generated files also invalidates the output manifest.
printf '%s\n' unexpected >"$fixture/fxdata/generated/unexpected.txt"
must_be_stale 'added generated output'
rm -f "$fixture/fxdata/generated/unexpected.txt"
refresh_manifest_and_stamp
rm -f "$fixture/fxdata/generated/flag_bit_array.cpp"
must_be_stale 'deleted generated output'
cp -f "$fixture/src/flags/flag_bit_array.cpp" "$fixture/fxdata/generated/flag_bit_array.cpp"
refresh_manifest_and_stamp

# The four source mirrors are checked independently of the manifest.
printf 'stale mirror\n' >>"$fixture/src/flags/flags.hpp"
must_be_stale 'header mirror drift'
cp -f "$fixture/fxdata/generated/flags.hpp" "$fixture/src/flags/flags.hpp"
check

# Tool and generation recipe changes invalidate the fingerprint.
printf '# changed tool bytes\n' >>"$fixture/bin/cgfx-tools"
must_be_stale 'cgfx-tools changed'
record_stamp
printf '\n# generation recipe change\n' >>"$fixture/Makefile"
must_be_stale 'Makefile changed'
record_stamp
printf '\n# generation script change\n' >>"$fixture/tools/emit-rust-teams.sh"
must_be_stale 'generation script changed'
record_stamp
printf '\n# freshness policy change\n' >>"$fixture/tools/ensure-generated.sh"
must_be_stale 'freshness helper changed'
record_stamp

# A failed regeneration after a stale input must leave the previous stamp intact.
old_stamp=$(cat "$fixture/$stamp")
printf 'stale input\n' >>"$fixture/data/source.txt"
if GEN_LOG="$tmp/gen.log" GEN_FAIL=1 ensure "$fixture/bin/mock-gen" >"$tmp/fail.out" 2>&1; then
    echo 'stale failed generation unexpectedly succeeded' >&2
    exit 1
fi
test "$(cat "$fixture/$stamp")" = "$old_stamp"

# Unsupported custom artifact paths always regenerate rather than reuse.
if PATH="$tool_path" "$fixture/tools/ensure-generated.sh" --check "$fixture" "$stamp" \
    custom-layout.toml fxdata/generated/manifest.json dist dist dist/fxdata.bin \
    dist/fxdata-data.bin dist/fxdata-save.bin >"$tmp/custom.out" 2>&1; then
    echo 'custom layout incorrectly reused default cache' >&2
    exit 1
fi

echo 'ensure-generated: PASS'
