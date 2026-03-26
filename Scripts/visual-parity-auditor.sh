#!/usr/bin/env bash
# visual-parity-auditor.sh — Full build + launch + screenshot + compare workflow.
# Compares the running M-LIM UI against the FabFilter Pro-L 2 reference screenshot.
# Exit 0 = PASS (RMSE < threshold), Exit 1 = FAIL.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
REFERENCE_IMAGE="/reference-docs/reference-screenshots/prol2-main-ui.jpg"
SCREENSHOTS_DIR="${PROJECT_ROOT}/screenshots"
DISPLAY_NUM="99"
RMSE_THRESHOLD="0.40"
TIMESTAMP="$(date +%Y%m%d-%H%M%S)"
SCREENSHOT_OUT="${SCREENSHOTS_DIR}/parity-check-${TIMESTAMP}.png"

APP_PID=""
XVFB_PID=""

cleanup() {
  [ -n "$APP_PID" ] && kill "$APP_PID" 2>/dev/null || true
  [ -n "$XVFB_PID" ] && kill "$XVFB_PID" 2>/dev/null || true
}
trap cleanup EXIT

# ---------------------------------------------------------------------------
# 0. Install missing dependencies
# ---------------------------------------------------------------------------
MISSING_PKGS=""
for pkg in xvfb scrot bc; do
  command -v "$pkg" &>/dev/null || MISSING_PKGS="$MISSING_PKGS $pkg"
done
command -v convert &>/dev/null || MISSING_PKGS="$MISSING_PKGS imagemagick"
if [ -n "$MISSING_PKGS" ]; then
  echo "[auditor] Installing missing packages:$MISSING_PKGS"
  sudo apt-get update -qq && sudo apt-get install -y $MISSING_PKGS
fi

# ---------------------------------------------------------------------------
# 1. Build the plugin
# ---------------------------------------------------------------------------
echo "[auditor] Building M-LIM..."
export CCACHE_DIR="${CCACHE_DIR:-/build-cache}"
cd "$PROJECT_ROOT/M-LIM" 2>/dev/null || cd "$PROJECT_ROOT"
cmake -B build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache 2>&1 | grep -v "^--" || true
cmake --build build --config Release -j"$(nproc)" 2>&1 | tail -5
echo "[auditor] Build complete."

# ---------------------------------------------------------------------------
# 2. Find the standalone binary
# ---------------------------------------------------------------------------
STANDALONE=""
for c in \
  "${PROJECT_ROOT}/M-LIM/build/MLIM_artefacts/Release/Standalone/M-LIM" \
  "${PROJECT_ROOT}/M-LIM/build/MLIM_artefacts/Standalone/M-LIM" \
  "${PROJECT_ROOT}/build/MLIM_artefacts/Release/Standalone/M-LIM" \
  "${PROJECT_ROOT}/build/MLIM_artefacts/Standalone/M-LIM"; do
  [ -x "$c" ] && { STANDALONE="$c"; break; }
done
if [ -z "$STANDALONE" ]; then
  STANDALONE="$(find "${PROJECT_ROOT}" -name "M-LIM" -type f -perm /111 2>/dev/null | head -1)"
fi
if [ -z "$STANDALONE" ]; then
  echo "[auditor] ERROR: Standalone binary not found. Build failed?" >&2
  exit 1
fi
echo "[auditor] Using binary: $STANDALONE"

# ---------------------------------------------------------------------------
# 3. Start Xvfb
# ---------------------------------------------------------------------------
if xdpyinfo -display ":${DISPLAY_NUM}" &>/dev/null; then
  echo "[auditor] Xvfb :${DISPLAY_NUM} already running, reusing."
else
  Xvfb ":${DISPLAY_NUM}" -screen 0 1280x800x24 &
  XVFB_PID=$!
  sleep 1
  if ! kill -0 "$XVFB_PID" 2>/dev/null; then
    echo "[auditor] ERROR: Xvfb failed to start." >&2
    exit 1
  fi
  echo "[auditor] Xvfb started (pid $XVFB_PID)."
fi
export DISPLAY=":${DISPLAY_NUM}"

# ---------------------------------------------------------------------------
# 4. Launch the standalone app
# ---------------------------------------------------------------------------
"$STANDALONE" &
APP_PID=$!
echo "[auditor] App launched (pid $APP_PID), waiting 4s for UI..."
sleep 4

# ---------------------------------------------------------------------------
# 5. Capture screenshot
# ---------------------------------------------------------------------------
mkdir -p "$SCREENSHOTS_DIR"
import -window root "$SCREENSHOT_OUT" 2>/dev/null \
  || scrot "$SCREENSHOT_OUT" 2>/dev/null \
  || { echo "[auditor] ERROR: screenshot capture failed." >&2; exit 1; }
echo "[auditor] Screenshot saved: $SCREENSHOT_OUT"

# ---------------------------------------------------------------------------
# 6. Compare against reference
# ---------------------------------------------------------------------------
if [ ! -f "$REFERENCE_IMAGE" ]; then
  echo "[auditor] WARNING: reference not found at $REFERENCE_IMAGE — skipping compare." >&2
  exit 0
fi

TMP_CURRENT="$(mktemp /tmp/auditor-current-XXXXXX.png)"
TMP_REFERENCE="$(mktemp /tmp/auditor-reference-XXXXXX.png)"
TMP_DIFF="$(mktemp /tmp/auditor-diff-XXXXXX.png)"

convert "$SCREENSHOT_OUT"  -resize 900x500\! "$TMP_CURRENT"
convert "$REFERENCE_IMAGE" -resize 900x500\! "$TMP_REFERENCE"

RMSE_RAW="$(compare -metric RMSE "$TMP_CURRENT" "$TMP_REFERENCE" "$TMP_DIFF" 2>&1 || true)"
# compare exits 1 when images differ; extract numeric value regardless
RMSE="$(echo "$RMSE_RAW" | grep -Eo '[0-9]+\.[0-9]+' | head -1)"
if [ -z "$RMSE" ]; then
  RMSE="$(echo "$RMSE_RAW" | grep -Eo '^[0-9]+' | head -1)"
  RMSE="${RMSE:-1}"
fi

echo "[auditor] RMSE: $RMSE (threshold: $RMSE_THRESHOLD)"

PASS="$(awk "BEGIN { print ($RMSE <= $RMSE_THRESHOLD) ? \"1\" : \"0\" }")"
if [ "$PASS" = "1" ]; then
  echo "PASS — UI visual parity within threshold."
  STATUS=0
else
  echo "FAIL — RMSE $RMSE exceeds threshold $RMSE_THRESHOLD. Diff: $TMP_DIFF"
  STATUS=1
fi

rm -f "$TMP_CURRENT" "$TMP_REFERENCE"

exit "$STATUS"
