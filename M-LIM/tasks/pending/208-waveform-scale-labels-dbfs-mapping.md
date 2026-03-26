# Task 208: Waveform Display — dBFS Scale Labels and Input-Level Y Mapping

## Description
The current WaveformDisplay maps the Y-axis based on Gain Reduction (GR) amount: top = 0 dB GR (ceiling), bottom = maximum GR. The reference Pro-L 2 uses a **dBFS scale** where the Y-axis shows signal level: 0 dBFS at top, negative dBFS values descending.

### Reference scale (from prol2-main-ui.jpg, v1-0003.png):
- Scale labels on left side of waveform: "0 dB", "-3 dB", "-6 dB", "-9 dB", ... down to about "-27 dB"
- Input waveform fills FROM the 0 dB ceiling DOWNWARD showing signal amplitude
- The output envelope line sits BELOW the ceiling, showing how far the signal was reduced
- The region BETWEEN ceiling (0 dB) and output envelope = the GR fill (blue-grey)
- Grid lines align with dBFS labels

### Issues to fix:
1. **Scale labels**: Currently drawn on the RIGHT side and use GR dB values. Reference shows labels on LEFT side (or adjacent to waveform left) using dBFS values with "dB" suffix (e.g. "0 dB", "-3 dB").
2. **Input waveform Y position**: Input level (0.0–1.0 linear) should map to Y using dBFS, not reverse GR mapping. 0 dBFS = top, silence = bottom.
3. **Scale strip placement**: Currently a right-side scale strip (`kScaleWidth`). Reference has the dB labels on the RIGHT side of the waveform but the waveform fills full width. Keep right-side scale strip but fix labels to show dBFS.

### What NOT to change:
- `levelToY()` maps output level to Y — this is correct
- `grToHeight()` maps GR to bar height — used for GR fill from top, keep as-is
- The GR fill (already fixed to blue-grey in Colours.h) visual is correct

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/WaveformDisplay.cpp` — fix `drawScale()` to use dBFS labels and "dB" suffix; fix input waveform Y mapping in `drawInputFill()`
Read: `src/ui/Colours.h` — textSecondary colour for scale labels
Read: `/reference-docs/reference-screenshots/prol2-main-ui.jpg` — scale label reference

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: `[100%] Built target MLIM_Standalone`
- [ ] Run: visual inspection — waveform right-side scale shows "0 dB", "-3 dB", "-6 dB" etc. labels matching dBFS values, not GR values

## Tests
None

## Technical Details
In `drawScale()`:
- Instead of drawing GR dB labels, draw dBFS labels: `0 dB, -3 dB, -6 dB, ..., -27 dB`
- Y position for each label: `area.getY() + ((-db) / kMaxDBFS) * area.getHeight()` where `kMaxDBFS = 30.0f`
- Label text format: `juce::String(static_cast<int>(db)) + " dB"` (e.g. "0 dB", "-3 dB")

In `drawInputFill()`:
- Map input level (0.0–1.0 linear) to dBFS: `db = 20 * log10(level)`, clamp to [-kMaxDBFS, 0]
- Y position: `y = area.getY() + ((-db) / kMaxDBFS) * area.getHeight()`
- The fill should go from Y position upward to the top of the display area (ceiling)

## Dependencies
None
