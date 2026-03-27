# Task 361: Revert Waveform Grid Alpha Regression from Task 357

## Description
Task 357 reduced the waveform grid line alpha from 0.6f to 0.35f and the dB label alpha from
0.35f to 0.25f. This caused a measurable regression: waveform sub-region RMSE went from 20.55%
(best ever) to 21.87%. Revert both alpha values to restore visual parity with the reference.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/WaveformDisplay.cpp` — revert grid line alpha at line ~293, label alpha at line ~321

## Acceptance Criteria
- [ ] Run: `grep -n "waveformGridLine.withAlpha" M-LIM/src/ui/WaveformDisplay.cpp` → Expected: shows `0.6f`
- [ ] Run: `grep -n "float alpha = " M-LIM/src/ui/WaveformDisplay.cpp` → Expected: shows `0.35f`
- [ ] Build and capture screenshot, compute waveform sub-region RMSE → Expected: ≤21.00% (recovering from 21.87%)

## Tests
None

## Technical Details

**Exact changes needed in `M-LIM/src/ui/WaveformDisplay.cpp`:**

Line ~293 (grid line draw):
```cpp
// BEFORE (task 357 regression):
g.setColour (MLIMColours::waveformGridLine.withAlpha (0.35f));

// AFTER (revert to pre-357 value):
g.setColour (MLIMColours::waveformGridLine.withAlpha (0.6f));
```

Line ~321 (dB overlay labels):
```cpp
// BEFORE (task 357 regression):
float alpha = 0.25f;

// AFTER (revert to pre-357 value):
float alpha = 0.35f;
```

**RMSE measurement methodology (correct):**
```bash
# Reference prep
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/ref361.png

# Build
export CCACHE_DIR=/build-cache
cmake -B /tmp/build361 -S M-LIM -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
cmake --build /tmp/build361 -j$(nproc)

# Launch and capture
source Scripts/ui-test-helper.sh
start_app
sleep 3
scrot /tmp/task361-raw.png
stop_app

# Crop
convert /tmp/task361-raw.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/task361-mlim.png

# Full RMSE
compare -metric RMSE /tmp/ref361.png /tmp/task361-mlim.png /dev/null 2>&1

# Waveform sub-region RMSE (key metric for this task)
compare -metric RMSE \
    <(convert /tmp/ref361.png -crop 600x400+150+50 +repage png:-) \
    <(convert /tmp/task361-mlim.png -crop 600x400+150+50 +repage png:-) \
    /dev/null 2>&1
```

Save results to `screenshots/task-361-rmse-results.txt`.

## Dependencies
None
