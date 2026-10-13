#!/usr/bin/env bash
# Report local CreatureGathererFX readiness without installing or configuring anything.
set -u

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)
# shellcheck source=fxdata-manifest-lib.sh
source "$script_dir/fxdata-manifest-lib.sh"
root=${DOCTOR_ROOT:-$(CDPATH= cd -- "$script_dir/.." && pwd -P)}
cxx=${CXX:-g++}
arduino_cli=${ARDUINO_CLI:-arduino-cli}
: "${FQBN:=arduboy-homemade:avr:arduboy-fx}"
manifest_assert=${DOCTOR_ASSERT:-$root/tools/assert-fxdata-manifest.sh}
fxdata_file=${DOCTOR_FXDATA:-$root/fxlayout.toml}
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
        remedy 'use a supported host or install a compatible cgfx-tools executable on PATH'
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
    # Selected core id is the FQBN prefix before the board segment. A required
    # library is satisfied by a standalone install (lib list) or by the selected
    # installed core's bundled libraries under
    # <data>/packages/<vendor>/hardware/<arch>/<version>/libraries/<lib>.
    # Collector: arduino-cli config get directories.data (non-mutating).
    core_data=$($arduino_cli config get directories.data 2>/dev/null)
    core_vendor=${FQBN%%:*}
    core_arch=${FQBN#*:}
    core_arch=${core_arch%%:*}
    missing_cores=
    missing_libs=
    if test "$core_status" -ne 0 || ! printf '%s\n' "$core_output" | grep -Fq 'arduboy-homemade:avr'; then
        missing_cores='arduboy-homemade:avr'
    fi
    if test "$core_status" -ne 0 || ! printf '%s\n' "$core_output" | grep -Fq 'arduino:avr'; then
        test -n "$missing_cores" && missing_cores="$missing_cores arduino:avr" || missing_cores='arduino:avr'
    fi
    for required_lib in Arduboy2 ArdBitmap; do
        lib_present=0
        if test "$lib_status" -ne 0; then
            lib_present=0
        elif printf '%s\n' "$lib_output" | grep -Fq "$required_lib"; then
            lib_present=1
        else
            for candidate in \
                "$core_data"/packages/"$core_vendor"/hardware/"$core_arch"/*/libraries/"$required_lib"; do
                if test -d "$candidate"; then
                    lib_present=1
                    break
                fi
            done
        fi
        if test "$lib_present" -eq 0; then
            test -n "$missing_libs" && missing_libs="$missing_libs $required_lib" || missing_libs="$required_lib"
        fi
    done
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

if cgfx_tool=$(command -v cgfx-tools 2>/dev/null); then
    cgfx_version=$("$cgfx_tool" --version 2>&1)
    cgfx_status=$?
    if test "$cgfx_status" -ne 0 || test -z "$cgfx_version"; then
        fail "cgfx-tools version unavailable: $cgfx_tool"
        remedy 'install a compatible cgfx-tools executable on PATH'
    else
        cgfx_version=$(printf '%s\n' "$cgfx_version" | awk '{print $NF}')
        pass "cgfx-tools: path=$cgfx_tool version=$cgfx_version"
    fi
    cgfx_help=$("$cgfx_tool" --help 2>&1)
    cgfx_help_status=$?
    missing_capabilities=
    for capability in --project --arena-csv --arena-output --type-table-csv \
        --type-table-output --consumables-csv --consumables-output \
        --sprite-config --emit-fixtures --pack --layout --assert-layout-equivalent; do
        if ! printf '%s\n' "$cgfx_help" | grep -Eq -- "(^|[[:space:]])$capability([[:space:]=]|$)"; then
            missing_capabilities="${missing_capabilities:+$missing_capabilities }$capability"
        fi
    done
    if test "$cgfx_help_status" -ne 0 || test -n "$missing_capabilities"; then
        fail "cgfx-tools capabilities: help_status=$cgfx_help_status missing=${missing_capabilities:-none}"
        remedy 'rebuild/install cgfx-tools from current CreatureGathererTools source and put its bin directory first on PATH; version alone does not establish compatibility'
    else
        pass 'cgfx-tools project capabilities'
    fi
    # Identity agreement: the installed binary must be the build that recorded
    # the generation manifest. A matching capability set with a different
    # identity means the manifest is stale or the wrong generator produced it.
    if test -n "$cgfx_version" && test -f "$manifest"; then
        generator_name=$(awk -F '"' '/^[[:space:]]*"generator"[[:space:]]*:/ { print $6; exit }' "$manifest")
        if test "$generator_name" = cgfx-tools; then
            manifest_identity=$(fxdata_tool_version "$manifest" 2>/dev/null) || manifest_identity=
            if test -z "$manifest_identity"; then
                fail "cgfx-tools identity: manifest generator version unreadable: $manifest"
                remedy 'regenerate with the selected cgfx-tools on PATH: make gen'
            elif test "$manifest_identity" != "$cgfx_version"; then
                fail "cgfx-tools identity: PATH binary ($cgfx_version) differs from manifest generator ($manifest_identity)"
                remedy 'regenerate with the selected cgfx-tools (make gen) or install the generator that produced the manifest'
            else
                pass "cgfx-tools identity matches manifest generator ($manifest_identity)"
            fi
        fi
    fi
else
    fail 'cgfx-tools: not found on PATH'
    remedy 'install cgfx-tools and add its bin directory to PATH'
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
