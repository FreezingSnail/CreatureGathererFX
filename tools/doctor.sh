#!/bin/sh
# Report local CreatureGathererFX readiness without installing or configuring anything.
set -u

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)
root=${DOCTOR_ROOT:-$(CDPATH= cd -- "$script_dir/.." && pwd -P)}
cxx=${CXX:-g++}
arduino_cli=${ARDUINO_CLI:-arduino-cli}
manifest_assert=${DOCTOR_ASSERT:-$root/tools/assert-fxdata-manifest.sh}
fxdata_file=${DOCTOR_FXDATA:-$root/fxdata/fxdata.txt}
manifest=${DOCTOR_MANIFEST:-$root/fxdata/generated/manifest.json}
required_failures=0

pass() { printf 'PASS %s\n' "$1"; }
fail() { printf 'FAIL %s\n' "$1"; required_failures=1; }
skip() { printf 'SKIP %s\n' "$1"; }
remedy() { printf '  remedy: %s\n' "$1"; }

host_os=$(uname -s 2>/dev/null || printf unknown)
host_arch=$(uname -m 2>/dev/null || printf unknown)
printf 'CreatureGathererFX readiness\n'
printf 'host: %s %s\n' "$host_os" "$host_arch"
case "$host_os:$host_arch" in
    Darwin:arm64|Darwin:aarch64|Linux:x86_64|Linux:amd64) pass "supported host ($host_os $host_arch)" ;;
    *)
        fail "unsupported host ($host_os $host_arch)"
        remedy 'use macOS arm64 or Linux x86_64, or provide a compatible CGFX_TOOLS_BIN'
        ;;
esac

check_command() {
    label=$1
    command_name=$2
    fix=$3
    if path=$(command -v "$command_name" 2>/dev/null); then
        version=$($command_name --version 2>&1 | sed -n '1p')
        test -n "$version" || version=version-unknown
        pass "$label: $path ($version)"
    else
        fail "$label: $command_name not found"
        remedy "$fix"
    fi
}

check_command python3 python3 'install Python 3 and put python3 on PATH'
check_command g++ "$cxx" 'install a C++17 compiler and put g++ on PATH'
check_command make make 'install make and put it on PATH'

arduino_path=
if arduino_path=$(command -v "$arduino_cli" 2>/dev/null); then
    cli_version=$($arduino_cli version 2>&1 | sed -n '1p')
    test -n "$cli_version" || cli_version=version-unknown
    pass "arduino-cli: $arduino_path ($cli_version)"
    core_output=$($arduino_cli core list 2>&1)
    core_status=$?
    lib_output=$($arduino_cli lib list 2>&1)
    lib_status=$?
    missing_cores=
    missing_libs=
    if test "$core_status" -ne 0 || ! printf '%s\n' "$core_output" | grep -Fq 'arduboy-homemade:avr'; then
        missing_cores='arduboy-homemade:avr'
    fi
    if test "$core_status" -ne 0 || ! printf '%s\n' "$core_output" | grep -Fq 'arduino:avr'; then
        test -n "$missing_cores" && missing_cores="$missing_cores arduino:avr" || missing_cores='arduino:avr'
    fi
    if test "$lib_status" -ne 0 || ! printf '%s\n' "$lib_output" | grep -Fq 'Arduboy2'; then
        missing_libs='Arduboy2'
    fi
    if test "$lib_status" -ne 0 || ! printf '%s\n' "$lib_output" | grep -Fq 'ArdBitmap'; then
        test -n "$missing_libs" && missing_libs="$missing_libs ArdBitmap" || missing_libs='ArdBitmap'
    fi
    if test -z "$missing_cores" && test -z "$missing_libs"; then
        pass 'Arduboy cores and libraries'
    else
        detail=
        test -n "$missing_cores" && detail="missing cores: $missing_cores"
        test -n "$missing_libs" && test -n "$detail" && detail="$detail; missing libraries: $missing_libs"
        test -n "$missing_libs" && test -z "$detail" && detail="missing libraries: $missing_libs"
        fail "Arduboy cores and libraries ($detail)"
        remedy 'run SETUP_APPLY=1 tools/setup.sh, or install the listed cores/libraries manually'
    fi
else
    fail "arduino-cli: $arduino_cli not found"
    remedy 'install arduino-cli and put it on PATH; then run SETUP_APPLY=1 tools/setup.sh'
fi

sha256_file() {
    if command -v shasum >/dev/null 2>&1; then
        shasum -a 256 "$1" | awk '{print $1}'
    elif command -v sha256sum >/dev/null 2>&1; then
        sha256sum "$1" | awk '{print $1}'
    else
        return 1
    fi
}

lock_value() {
    key=$1
    awk -F= -v key="$key" '$1 == key { print $2; exit }' "$root/tools/toolchain.lock"
}

cgfx_tool=
if test -n "${CGFX_TOOLS_BIN:-}"; then
    cgfx_tool=$CGFX_TOOLS_BIN
elif test -x "$root/../CreatureGathererTools/target/release/cgfx-tools"; then
    cgfx_tool=$root/../CreatureGathererTools/target/release/cgfx-tools
else
    release_tag=$(lock_value RELEASE_TAG)
    repository=$(lock_value RELEASE_REPOSITORY)
    case "$host_os:$host_arch" in
        Darwin:arm64|Darwin:aarch64) asset=cgfx-tools-macos-arm64; expected=$(lock_value SHA256_CGFX_TOOLS_MACOS_ARM64) ;;
        Linux:x86_64|Linux:amd64) asset=cgfx-tools-linux-x64; expected=$(lock_value SHA256_CGFX_TOOLS_LINUX_X64) ;;
        *) asset=; expected= ;;
    esac
    cache="$root/.cache/cgfx-tools/$release_tag/$asset"
    if test -n "$asset" && test -f "$cache" && test -x "$cache"; then
        actual=$(sha256_file "$cache" 2>/dev/null || printf invalid)
        if test "$actual" = "$expected"; then
            cgfx_tool=$cache
        else
            fail "cgfx-tools cache checksum mismatch: $cache"
            remedy 'remove the bad cache entry, set CGFX_TOOLS_BIN to a verified executable, or rebuild the pinned release'
        fi
    fi
fi

if test -n "$cgfx_tool"; then
    if test ! -f "$cgfx_tool" || test ! -x "$cgfx_tool"; then
        fail "cgfx-tools is not executable: $cgfx_tool"
        remedy 'set CGFX_TOOLS_BIN to an executable cgfx-tools path, or build ../CreatureGathererTools/target/release/cgfx-tools'
    else
        cgfx_version=$($cgfx_tool --version 2>&1 | sed -n '1p')
        cgfx_status=$?
        if test "$cgfx_status" -ne 0 || test -z "$cgfx_version"; then
            fail "cgfx-tools version unavailable: $cgfx_tool"
            remedy 'run cgfx-tools --version successfully, or set CGFX_TOOLS_BIN to a compatible release'
        else
            cgfx_version=$(printf '%s\n' "$cgfx_version" | awk '{print $NF}')
            pass "cgfx-tools: path=$cgfx_tool version=$cgfx_version"
        fi
    fi
else
    fail 'cgfx-tools: no non-mutating local resolution found'
    remedy 'set CGFX_TOOLS_BIN=/absolute/path/to/cgfx-tools, or clone/build ../CreatureGathererTools; doctor never downloads or builds it'
fi

if test ! -f "$fxdata_file"; then
    fail "manifest: missing FX definition: $fxdata_file"
    remedy 'run make gen'
elif test ! -f "$manifest"; then
    fail "manifest: missing generated manifest: $manifest"
    remedy 'run make gen'
elif test ! -x "$manifest_assert"; then
    fail "manifest: verifier unavailable: $manifest_assert"
    remedy 'restore tools/assert-fxdata-manifest.sh, then run make gen'
else
    manifest_output=$($manifest_assert "$fxdata_file" "$manifest" 2>&1)
    manifest_status=$?
    generator=$(awk -F '"' '/^[[:space:]]*"generator"[[:space:]]*:/ { print $6 ":" $10; exit }' "$manifest")
    if test "$manifest_status" -eq 0; then
        pass "manifest: fresh${generator:+ (generator=$generator)}"
    else
        fail "manifest: stale or invalid generated artifacts"
        printf '%s\n' "$manifest_output" | sed -n '1p'
        remedy 'run make gen'
    fi
fi

if test -n "${ARDENS:-}"; then
    if test -x "$ARDENS"; then
        pass "Ardens: $ARDENS"
    else
        fail "Ardens requested but executable not found: $ARDENS"
        remedy 'build Ardens and set ARDENS=/absolute/path/to/Ardens'
    fi
else
    skip 'Ardens: not requested (set ARDENS=/absolute/path/to/Ardens for device tests)'
fi

if test "$required_failures" -eq 0; then
    printf '%s\n' 'doctor: READY'
else
    printf '%s\n' 'doctor: NOT READY' >&2
fi
exit "$required_failures"
