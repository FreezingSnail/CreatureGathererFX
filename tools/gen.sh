#!/usr/bin/env bash
# Compatibility entry point for the split Make generation surface.
set -euo pipefail

cd "$(dirname "$0")/.."
exec make gen
