#!/usr/bin/env bash
# Capture a screenshot of the M-LIM standalone app running on Xvfb.
# Usage: ./Scripts/capture-screenshot.sh [output.png] [delay_seconds]
set -euo pipefail

OUTPUT="${1:-screenshots/current.png}"
DELAY="${2:-3}"
DISPLAY_NUM="${DISPLAY_NUM:-99}"
SCREEN_RES="${SCREEN_RES:-1920x1080x24}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$PROJECT_DIR/build}"

STANDALONE=""
for candidate in \
    "$BUILD_DIR/MLIM_artefacts/Standalone/MLIM" \
    "$BUILD_DIR/MLIM_artefacts/Release/Standalone/MLIM" \
    "$BUILD_DIR/MLIM_artefacts/Debug/Standalone/MLIM" \
    "$BUILD_DIR/MLIM_artefacts/Standalone/M-LIM" \
    "$BUILD_DIR/MLIM_artefacts/Release/Standalone/M-LIM" \
    "$BUILD_DIR/MLIM_artefacts/Debug/Standalone/M-LIM"; do
    if [[ -x "$candidate" ]]; then
        STANDALONE="$candidate"
        break
    fi
done

if [[ -z "$STANDALONE" ]]; then
    echo "ERROR: Standalone binary not found. Build with: cmake --build build -j\$(nproc)" >&2
    exit 1
fi

mkdir -p "$(dirname "$OUTPUT")"

if ! xdpyinfo -display ":$DISPLAY_NUM" &>/dev/null; then
    Xvfb ":$DISPLAY_NUM" -screen 0 "$SCREEN_RES" &
    XVFB_PID=$!
    sleep 1
else
    XVFB_PID=""
fi

export DISPLAY=":$DISPLAY_NUM"

"$STANDALONE" &
APP_PID=$!

sleep "$DELAY"
import -window root "$OUTPUT"

kill "$APP_PID" 2>/dev/null || true
if [[ -n "$XVFB_PID" ]]; then
    kill "$XVFB_PID" 2>/dev/null || true
fi

echo "Screenshot saved: $OUTPUT"
