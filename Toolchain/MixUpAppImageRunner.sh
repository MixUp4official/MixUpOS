#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 <path-to-appimage> [args...]" >&2
    exit 1
fi

APPIMAGE="$1"
shift || true

if [[ ! -f "$APPIMAGE" ]]; then
    echo "Error: '$APPIMAGE' not found." >&2
    exit 1
fi

chmod +x "$APPIMAGE"

echo "[MixUpOS/AppImage] Starting experimental AppImage execution for: $APPIMAGE"

# Fast path for AppImages that support direct extract-and-run.
if "$APPIMAGE" --appimage-version >/dev/null 2>&1; then
    if "$APPIMAGE" --appimage-extract-and-run --help >/dev/null 2>&1; then
        exec "$APPIMAGE" --appimage-extract-and-run "$@"
    fi
fi

WORKDIR="$(mktemp -d /tmp/mixupos-appimage-XXXXXX)"
trap 'rm -rf "$WORKDIR"' EXIT

pushd "$WORKDIR" >/dev/null
"$APPIMAGE" --appimage-extract >/dev/null
if [[ ! -x squashfs-root/AppRun ]]; then
    echo "Error: AppRun not found after extraction. Unsupported AppImage format." >&2
    exit 1
fi

exec ./squashfs-root/AppRun "$@"
