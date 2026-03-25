#!/bin/bash
# UI test helper functions for interactive testing via Xvfb + xdotool + ImageMagick.
# Source this file to use start_app, stop_app, screenshot, click_at, drag_from_to,
# type_text, and wait_for_render.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
SCREENSHOTS_DIR="${PROJECT_ROOT}/screenshots"
mkdir -p "$SCREENSHOTS_DIR"

APP_PID=""
XVFB_PID=""
DISPLAY_NUM=""

_find_standalone() {
  local candidates=(
    "${PROJECT_ROOT}/build/MLIM_artefacts/Standalone/MLIM"
    "${PROJECT_ROOT}/build/MLIM_artefacts/Standalone/M-LIM"
    "${PROJECT_ROOT}/build/MLIM_artefacts/Debug/Standalone/MLIM"
    "${PROJECT_ROOT}/build/MLIM_artefacts/Debug/Standalone/M-LIM"
    "${PROJECT_ROOT}/build/MLIM_artefacts/Release/Standalone/MLIM"
    "${PROJECT_ROOT}/build/MLIM_artefacts/Release/Standalone/M-LIM"
  )
  for c in "${candidates[@]}"; do
    if [ -x "$c" ]; then echo "$c"; return 0; fi
  done
  echo "ERROR: Standalone binary not found. Build first." >&2
  return 1
}

start_app() {
  DISPLAY_NUM="${1:-99}"
  export DISPLAY=":${DISPLAY_NUM}"
  if [ -f "/tmp/.X${DISPLAY_NUM}-lock" ]; then
    local old_pid; old_pid=$(cat "/tmp/.X${DISPLAY_NUM}-lock" 2>/dev/null | tr -d ' ')
    if [ -n "$old_pid" ] && ! kill -0 "$old_pid" 2>/dev/null; then
      rm -f "/tmp/.X${DISPLAY_NUM}-lock" "/tmp/.X11-unix/X${DISPLAY_NUM}" 2>/dev/null
    fi
  fi
  Xvfb ":${DISPLAY_NUM}" -screen 0 1920x1080x24 &
  XVFB_PID=$!; sleep 1
  if ! kill -0 "$XVFB_PID" 2>/dev/null; then echo "ERROR: Xvfb failed" >&2; return 1; fi
  local binary; binary=$(_find_standalone) || return 1
  "$binary" &
  APP_PID=$!; sleep 1
  local waited=0
  while [ $waited -lt 10 ]; do
    if xdotool search --onlyvisible --name "." 2>/dev/null | head -1 | grep -q .; then
      echo "Window detected after ${waited}s"; return 0
    fi
    sleep 1; waited=$((waited + 1))
  done
  echo "WARNING: No window detected after 10s" >&2; return 0
}

stop_app() {
  [ -n "$APP_PID" ] && kill -0 "$APP_PID" 2>/dev/null && { kill "$APP_PID" 2>/dev/null; wait "$APP_PID" 2>/dev/null || true; APP_PID=""; }
  [ -n "$XVFB_PID" ] && kill -0 "$XVFB_PID" 2>/dev/null && { kill "$XVFB_PID" 2>/dev/null; wait "$XVFB_PID" 2>/dev/null || true; XVFB_PID=""; }
}

screenshot() { local f="${1:-screenshot.png}"; import -window root "${SCREENSHOTS_DIR}/${f}"; echo "Screenshot saved: screenshots/${f}"; }
click_at() { xdotool mousemove --sync "$1" "$2" click 1; }
drag_from_to() { xdotool mousemove "$1" "$2" mousedown 1 mousemove "$3" "$4" mouseup 1; }
type_text() { xdotool type "$1"; }
wait_for_render() { local ms="${1:-500}"; sleep "$(awk "BEGIN {printf \"%.3f\", $ms / 1000}")"; }

wait_for_paint() {
  local max_wait="${1:-2000}" start_ms=$(date +%s%3N)
  while true; do
    local tmp=$(mktemp --suffix=.png)
    import -window root "$tmp" 2>/dev/null
    local mean=$(identify -format '%[mean]' "$tmp" 2>/dev/null || echo "0")
    rm -f "$tmp"
    [ "$(echo "$mean > 1000" | bc)" = "1" ] && return 0
    [ $(( $(date +%s%3N) - start_ms )) -gt $max_wait ] && { echo "WARNING: paint timeout" >&2; return 1; }
    sleep 0.1
  done
}

screenshot_safe() { local f="${1:-screenshot.png}" w="${2:-2000}"; wait_for_paint "$w"; screenshot "$f"; }

compare_to_reference() {
  local ref="$1" captured="$2" threshold="${3:-0.15}"
  [ ! -f "$ref" ] && { echo "WARNING: ref not found: $ref" >&2; return 2; }
  [ ! -f "$captured" ] && { echo "ERROR: captured not found: $captured" >&2; return 1; }
  local diff_out="${captured%.*}_diff.png"
  local rmse; rmse=$(compare -metric RMSE "$ref" "$captured" "$diff_out" 2>&1 | grep -Eo '[0-9]+\.[0-9]+' | head -1)
  [ -z "$rmse" ] && { rmse=$(compare -metric RMSE "$ref" "$captured" "$diff_out" 2>&1 | grep -Eo '^[0-9]+' | head -1); rmse="${rmse:-1}"; }
  echo "RMSE: $rmse (threshold: $threshold) — ref: $(basename "$ref")"
  awk "BEGIN {exit ($rmse <= $threshold) ? 0 : 1}" && { echo "PASS"; return 0; } || { echo "FAIL — diff: $diff_out"; return 1; }
}
