#!/bin/sh
# Print the cgfx-tools executable selected by PATH for callers retaining this compatibility entry point.
set -eu

if tool=$(command -v cgfx-tools 2>/dev/null); then
    printf '%s\n' "$tool"
else
    printf '%s\n' 'error: cgfx-tools is not on PATH; install it and add its bin directory to PATH' >&2
    exit 1
fi
