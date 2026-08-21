#!/usr/bin/env bash
# Derive the image stamp and device fixture from cgfx-tools' manifest.json.
set -euo pipefail

manifest=fxdata/generated/manifest.json
stamp=fxdata/generated/tool_version.bin
fixture=tst/fxdatatest/generated/version_data.hpp

test -f "$manifest" || { echo "missing manifest: $manifest" >&2; exit 1; }
version=$(awk -F '"' '/^[[:space:]]*"tool_version"[[:space:]]*:/ { print $4; exit }' "$manifest")
test -n "$version" || { echo "manifest has no tool_version: $manifest" >&2; exit 1; }
case "$version" in
    *[!A-Za-z0-9._\ -]*) echo "unsupported tool_version for C++ stamp: $version" >&2; exit 1 ;;
esac

printf '%s\0' "$version" > "$stamp"
printf '%s\n' '// Generated from fxdata/generated/manifest.json - do not edit.' '#pragma once' '#include <avr/pgmspace.h>' > "$fixture"
printf 'const char generatedToolVersion[] PROGMEM = "%s";\n' "$version" >> "$fixture"
printf 'constexpr uint8_t generatedToolVersionBytes = sizeof(generatedToolVersion);\n' >> "$fixture"
