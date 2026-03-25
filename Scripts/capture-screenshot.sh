#!/usr/bin/env bash
# Capture a screenshot of the M-LIM standalone app running on Xvfb.
# Usage: ./Scripts/capture-screenshot.sh [output.png] [delay_seconds]
set -euo pipefail
OUTPUT="${1:-screenshots/current.png}"; DELAY="${2:-3}"
DISPLAY_NUM="${DISPLAY_NUM:-99}"; SCREEN_RES="${SCREEN_RES:-1920x1080x24}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$PROJECT_DIR/build}"
STANDALONE=""
for c in "$BUILD_DIR/MLIM_artefacts/"{,Release/,Debug/}Standalone/{MLIM,M-LIM}; do
  [ -x "$c" ] && { STANDALONE="$c"; break; }
done
[ -z "$STANDALONE" ] && { echo "ERROR: Standalone not found" >&2; exit 1; }
mkdir -p "$(dirname "$OUTPUT")"
if ! xdpyinfo -display ":$DISPLAY_NUM" &>/dev/null; then
  Xvfb ":$DISPLAY_NUM" -screen 0 "$SCREEN_RES" &; XVFB_PID=$!; sleep 1
else XVFB_PID=""; fi
export DISPLAY=":$DISPLAY_NUM"
"$STANDALONE" &; APP_PID=$!
sleep "$DELAY"; import -window root "$OUTPUT"
kill "$APP_PID" 2>/dev/null || true
[ -n "$XVFB_PID" ] && kill "$XVFB_PID" 2>/dev/null || true
echo "Screenshot saved: $OUTPUT"
