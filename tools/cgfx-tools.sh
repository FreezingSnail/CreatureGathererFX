#!/usr/bin/env bash
# Print one usable cgfx-tools executable path; diagnostics remain on stderr.
set -euo pipefail

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)
ROOT_DIR=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd -P)
SIBLING_DIR=$(CDPATH= cd -- "$ROOT_DIR/../CreatureGathererTools" 2>/dev/null && pwd -P || true)

absolute_path() {
    local path=$1
    local directory
    directory=$(CDPATH= cd -- "$(dirname -- "$path")" && pwd -P)
    printf '%s/%s\n' "$directory" "$(basename -- "$path")"
}

fail_resolution() {
    printf '%s\n' "$1" >&2
    printf '%s\n' 'Fix with one of:' >&2
    printf '  CGFX_TOOLS_BIN=/absolute/path/to/cgfx-tools %s\n' "$SCRIPT_DIR/cgfx-tools.sh" >&2
    printf '  git clone git@github.com:FreezingSnail/CreatureGathererTools.git %s\n' "$ROOT_DIR/../CreatureGathererTools" >&2
    printf '  (cd %s && cargo build --release -p cgfx-core --bin cgfx-tools)\n' "$ROOT_DIR/../CreatureGathererTools" >&2
    exit 1
}

if [[ -n ${CGFX_TOOLS_BIN:-} ]]; then
    if [[ ! -f $CGFX_TOOLS_BIN || ! -x $CGFX_TOOLS_BIN ]]; then
        fail_resolution "error: CGFX_TOOLS_BIN is not an executable file: $CGFX_TOOLS_BIN"
    fi
    absolute_path "$CGFX_TOOLS_BIN"
    exit 0
fi

if [[ -n $SIBLING_DIR && -d $SIBLING_DIR ]]; then
    if ! (cd "$SIBLING_DIR" && cargo build --release -p cgfx-core --bin cgfx-tools); then
        fail_resolution "error: failed to build local sibling tool: $SIBLING_DIR"
    fi

    LOCAL_TOOL=$SIBLING_DIR/target/release/cgfx-tools
    if [[ ! -f $LOCAL_TOOL || ! -x $LOCAL_TOOL ]]; then
        fail_resolution "error: local sibling build did not produce an executable: $LOCAL_TOOL"
    fi

    printf 'warning: using unpinned local cgfx-tools: %s\n' "$LOCAL_TOOL" >&2
    absolute_path "$LOCAL_TOOL"
    exit 0
fi

LOCK_FILE=$SCRIPT_DIR/toolchain.lock
if [[ ! -r $LOCK_FILE ]]; then
    fail_resolution "error: missing toolchain lock: $LOCK_FILE"
fi
# shellcheck source=toolchain.lock
source "$LOCK_FILE"

case "$(uname -s):$(uname -m)" in
    Darwin:arm64|Darwin:aarch64)
        ASSET=cgfx-tools-macos-arm64
        EXPECTED_SHA256=$SHA256_CGFX_TOOLS_MACOS_ARM64
        ;;
    Linux:x86_64|Linux:amd64)
        ASSET=cgfx-tools-linux-x64
        EXPECTED_SHA256=$SHA256_CGFX_TOOLS_LINUX_X64
        ;;
    *)
        fail_resolution "error: unsupported cgfx-tools host: $(uname -s) $(uname -m)"
        ;;
esac

if [[ ! $EXPECTED_SHA256 =~ ^[a-fA-F0-9]{64}$ ]]; then
    fail_resolution "error: invalid SHA-256 for $ASSET in $LOCK_FILE"
fi

sha256_file() {
    if command -v shasum >/dev/null 2>&1; then
        shasum -a 256 "$1" | awk '{print $1}'
    elif command -v sha256sum >/dev/null 2>&1; then
        sha256sum "$1" | awk '{print $1}'
    else
        fail_resolution 'error: neither shasum nor sha256sum is available'
    fi
}

CACHE_DIR=$ROOT_DIR/.cache/cgfx-tools/$RELEASE_TAG
CACHE_TOOL=$CACHE_DIR/$ASSET

if [[ -e $CACHE_TOOL ]]; then
    if [[ ! -f $CACHE_TOOL || ! -x $CACHE_TOOL ]]; then
        rm -f -- "$CACHE_TOOL"
        printf 'error: removed unusable cached cgfx-tools: %s\n' "$CACHE_TOOL" >&2
        exit 1
    fi

    ACTUAL_SHA256=$(sha256_file "$CACHE_TOOL")
    if [[ $ACTUAL_SHA256 != "$EXPECTED_SHA256" ]]; then
        rm -f -- "$CACHE_TOOL"
        printf 'error: checksum mismatch for cached %s; removed %s\n' "$ASSET" "$CACHE_TOOL" >&2
        printf 'expected: %s\nactual:   %s\n' "$EXPECTED_SHA256" "$ACTUAL_SHA256" >&2
        exit 1
    fi

    absolute_path "$CACHE_TOOL"
    exit 0
fi

mkdir -p "$CACHE_DIR"
PARTIAL_TOOL=$CACHE_TOOL.partial.$$
trap 'rm -f -- "$PARTIAL_TOOL"' EXIT
DOWNLOAD_URL=https://github.com/$RELEASE_REPOSITORY/releases/download/$RELEASE_TAG/$ASSET

if ! curl --fail --location --retry 3 --connect-timeout 15 --output "$PARTIAL_TOOL" "$DOWNLOAD_URL"; then
    fail_resolution "error: unable to download pinned cgfx-tools asset: $DOWNLOAD_URL"
fi

ACTUAL_SHA256=$(sha256_file "$PARTIAL_TOOL")
if [[ $ACTUAL_SHA256 != "$EXPECTED_SHA256" ]]; then
    rm -f -- "$PARTIAL_TOOL"
    printf 'error: checksum mismatch for downloaded %s; removed partial cache entry\n' "$ASSET" >&2
    printf 'expected: %s\nactual:   %s\n' "$EXPECTED_SHA256" "$ACTUAL_SHA256" >&2
    exit 1
fi

chmod +x "$PARTIAL_TOOL"
mv -f -- "$PARTIAL_TOOL" "$CACHE_TOOL"
absolute_path "$CACHE_TOOL"
