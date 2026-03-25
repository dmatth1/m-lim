# Task 181: Deduplicate dB Grid Scale Arrays

## Description
The dBFS scale markings are defined independently in two UI files:

- `WaveformDisplay.cpp` line 6: `static const float kGridDB[] = { 0.0f, -3.0f, -6.0f, ..., -27.0f }` (10 entries)
- `LevelMeter.cpp` line 7: `constexpr float kScaleMarks[] = { 0.0f, -3.0f, -6.0f, ..., -30.0f }` (11 entries, adds -30)

These are two versions of the same logical construct. If the display range changes (e.g. extending to -33 dBFS) both files must be updated. The inconsistency (WaveformDisplay stops at -27, LevelMeter continues to -30) is itself a visual inconsistency to review.

Move the shared markings to `Colours.h` (which is already the shared UI constants header, included by both files) or a new `UIConstants.h`. Keep the file-local array only if it genuinely differs.

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/ui/Colours.h` — current shared UI header; add scale constants here
Modify: `M-LIM/src/ui/WaveformDisplay.cpp` — replace local `kGridDB[]` with shared constant
Modify: `M-LIM/src/ui/LevelMeter.cpp` — replace local `kScaleMarks[]` with shared constant (or keep a small file-local extension if -30 is intentional)
Skip: `M-LIM/src/dsp/` — not relevant

## Acceptance Criteria
- [ ] Run: `grep -n "kGridDB\|kScaleMarks" M-LIM/src/ui/WaveformDisplay.cpp M-LIM/src/ui/LevelMeter.cpp` → Expected: no matches (both files use the shared constant from Colours.h)
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | tail -5` → Expected: build succeeds, exit 0

## Tests
None

## Technical Details
Add to `Colours.h` in the `MLIMColours` namespace or as file-scope constants:
```cpp
// dBFS grid markings used across meter and waveform displays
static constexpr float kMeterGridDB[] = {
    0.0f, -3.0f, -6.0f, -9.0f, -12.0f,
    -15.0f, -18.0f, -21.0f, -24.0f, -27.0f, -30.0f
};
static constexpr int kMeterGridDBCount = 11;
```

WaveformDisplay currently only uses 10 of these (stops at -27). If the waveform intentionally stops at -27 (because its max GR is 30 dB but -27 is the last label before the floor), keep a local `static constexpr int kWaveformGridDBCount = 10;` that slices the shared array. Document the intent with a comment.

## Dependencies
Requires task 185 (both modify WaveformDisplay.cpp — dead code removal lands first)
