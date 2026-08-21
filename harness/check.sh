#!/usr/bin/env bash
set -euo pipefail

# Propagate strict failure handling into tools/gen.sh, invoked by make gen.
export SHELLOPTS

cd "$(dirname "$0")/.."

make gen
make test
make fxtest-build
