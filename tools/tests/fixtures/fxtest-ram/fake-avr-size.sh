#!/bin/sh
[ "$#" -eq 2 ] && [ "$1" = -A ] || exit 2
if [ "${SIZE_TOOL_FAIL:-0}" = 1 ]; then
    echo 'fixture avr-size failure' >&2
    exit 7
fi
cat "$SIZE_FIXTURE"
