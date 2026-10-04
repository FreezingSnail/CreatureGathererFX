#!/bin/sh
# Permanent mock-backed coverage for non-mutating readiness diagnostics.
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd -P)
doctor="$root/tools/doctor.sh"
valid="$root/tools/tests/fixtures/fxdata-manifest/valid/fxdata"
stale="$root/tools/tests/fixtures/fxdata-manifest/changed-input/fxdata"
mock_bin="$root/tools/tests/fixtures/doctor/bin"

run_doctor() {
    PATH="$mock_bin:$PATH" \
    DOCTOR_FXDATA="$1/fxdata.txt" \
    DOCTOR_MANIFEST="$1/generated/manifest.json" \
    CXX="${CXX:-g++}" \
    ARDUINO_CLI="$mock_bin/arduino-cli" \
    ARDENS="${ARDENS:-}" \
    "$doctor" 2>&1
}

output=$(ARDENS= run_doctor "$valid")
printf '%s\n' "$output" | grep -Fq "host: $(uname -s) $(uname -m)"
printf '%s\n' "$output" | grep -Fq 'PASS cgfx-tools: path='
printf '%s\n' "$output" | grep -Fq 'version=0.2.0'
printf '%s\n' "$output" | grep -Fq 'PASS cgfx-tools project capabilities'
printf '%s\n' "$output" | grep -Fq 'PASS manifest: fresh (generator=cgfx-tools:1.2.3)'
printf '%s\n' "$output" | grep -Fq 'SKIP Ardens: not requested'
printf '%s\n' "$output" | grep -Fq 'doctor: READY'

# Same advertised version as the compatible fixture, but each missing
# required generation mode must independently reject the binary.
for capability in --consumables-csv --sprite-config --emit-fixtures --pack; do
    if output=$(DOCTOR_MOCK_CGFX_MISSING="$capability" run_doctor "$valid"); then
        printf 'missing cgfx-tools capability %s unexpectedly passed\n' "$capability" >&2
        exit 1
    fi
    printf '%s\n' "$output" | grep -Fq "FAIL cgfx-tools capabilities: help_status=0 missing=$capability"
    printf '%s\n' "$output" | grep -Fq 'remedy: rebuild/install cgfx-tools from current CreatureGathererTools source'
done

for failure in bad-version bad-help; do
    if output=$(DOCTOR_MOCK_CGFX="$failure" run_doctor "$valid"); then
        printf 'cgfx-tools %s unexpectedly passed\n' "$failure" >&2
        exit 1
    fi
    printf '%s\n' "$output" | grep -Fq 'FAIL cgfx-tools'
done

if output=$(CXX=missing-cxx run_doctor "$valid"); then
    printf '%s\n' 'missing compiler unexpectedly passed' >&2
    exit 1
fi
printf '%s\n' "$output" | grep -Fq 'FAIL g++: missing-cxx not found'
printf '%s\n' "$output" | grep -Fq 'remedy: install a C++17 compiler and put g++ on PATH'

if output=$(DOCTOR_MOCK_ARDUINO=missing run_doctor "$valid"); then
    printf '%s\n' 'missing Arduino dependencies unexpectedly passed' >&2
    exit 1
fi
printf '%s\n' "$output" | grep -Fq 'FAIL Arduboy cores and libraries'
printf '%s\n' "$output" | grep -Fq 'remedy: run SETUP_APPLY=1 tools/setup.sh, or install the listed cores/libraries manually'

if output=$(run_doctor "$stale"); then
    printf '%s\n' 'stale manifest unexpectedly passed' >&2
    exit 1
fi
printf '%s\n' "$output" | grep -Fq 'FAIL manifest: stale or invalid generated artifacts'
printf '%s\n' "$output" | grep -Fq 'remedy: run make gen'

if output=$(ARDENS="$root/tools/tests/fixtures/doctor/missing-Ardens" run_doctor "$valid"); then
    printf '%s\n' 'requested missing Ardens unexpectedly passed' >&2
    exit 1
fi
printf '%s\n' "$output" | grep -Fq 'FAIL Ardens requested but executable not found:'
printf '%s\n' "$output" | grep -Fq 'remedy: build Ardens and set ARDENS=/absolute/path/to/Ardens'

printf '%s\n' 'doctor: PASS'
