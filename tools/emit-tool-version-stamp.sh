#!/usr/bin/env bash
# Derive the image stamp and device fixture from cgfx-tools' recorded identity.
#
# The manifest may carry the identity as the legacy top-level `tool_version`
# key or as the canonical `generator.version`; fxdata_tool_version resolves
# both. The identity is the exact token the selected binary prints after
# "cgfx-tools " (crate version plus built source revision), so regeneration
# records a distinguishable build rather than a bare version.
set -euo pipefail

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)
# shellcheck source=fxdata-manifest-lib.sh
source "$script_dir/fxdata-manifest-lib.sh"

manifest=fxdata/generated/manifest.json
stamp=fxdata/generated/tool_version.bin
fixture=tst/fxdatatest/generated/version_data.hpp

test -f "$manifest" || { echo "missing manifest: $manifest" >&2; exit 1; }
version=$(fxdata_tool_version "$manifest")
test -n "$version" || { echo "manifest has no cgfx-tools identity: $manifest" >&2; exit 1; }
identity="cgfx-tools $version"
case "$identity" in
    *[!A-Za-z0-9._\ +-]*) echo "unsupported tool identity for C++ stamp: $identity" >&2; exit 1 ;;
esac

printf '%s\0' "$identity" > "$stamp"
printf '%s\n' '// Generated from fxdata/generated/manifest.json - do not edit.' '#pragma once' '#include <avr/pgmspace.h>' > "$fixture"
printf 'const char generatedToolVersion[] PROGMEM = "%s";\n' "$identity" >> "$fixture"
printf 'constexpr uint8_t generatedToolVersionBytes = sizeof(generatedToolVersion);\n' >> "$fixture"
