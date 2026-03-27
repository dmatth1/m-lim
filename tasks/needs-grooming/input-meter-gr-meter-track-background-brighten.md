# Task: Input Meter and GR Meter Track Background Brighten

## Description
The input level meter strip (x=0-30) and GR meter strip (x=648-660) are dramatically darker than the equivalent zones in the reference at idle. Pixel analysis:

**Input meter (x=0-30, y=40-400):**
- Reference: `#625D6F` (R=98, G=93, B=111) — medium purple-grey
- M-LIM current: `#2E2D2A` (R=46, G=45, B=42) — very dark neutral grey
- Gap: ~52 units darker

**GR meter strip (x=648-660, y=40-400):**
- Reference: `#6B7089` (R=107, G=112, B=137) — bright blue-grey
- M-LIM current: `#32262A` (R=50, G=38, B=42) — very dark brownish-maroon
- Gap: ~57-75 units darker

Both components use `MLIMColours::barTrackBackground` = `0xff231417` = (35,20,23) as the bar track background. At idle (no signal), the bars are empty and only the track background + idle gradient overlay is visible. The track background is far too dark compared to the reference.

The fix is to lighten `barTrackBackground` to bring the idle track color closer to the waveform display's background color. However, since `barTrackBackground` is shared by LevelMeter, GainReductionMeter, and LoudnessPanel, the change needs to be verified visually across all components.

Target: `barTrackBackground` → `0xff2A2838` or similar dark blue-grey (around R=42, G=40, B=56) that is both dark enough for active meter fill contrast AND lighter than the current very dark reddish-maroon.

A more complete fix: use a per-component background that matches the waveform display gradient for these narrow strips, since they are part of the waveform display area in the reference.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — `barTrackBackground` constant
Read: `src/ui/LevelMeter.cpp` — uses barTrackBackground at line 76
Read: `src/ui/GainReductionMeter.cpp` — uses barTrackBackground at lines 82-104
Read: `src/ui/LoudnessPanel.cpp` — uses barTrackBackground at line 499

## Acceptance Criteria
- [ ] Run pixel check: `convert screenshots/task-track-bg-after.png -crop 30x360+0+40 +repage -resize 1x1! txt:-` → Expected: Input meter composite R ≥ 55 (up from 46)
- [ ] Run pixel check: `convert screenshots/task-track-bg-after.png -crop 12x360+648+40 +repage -resize 1x1! txt:-` → Expected: GR meter composite R ≥ 60 (up from 50)
- [ ] Run RMSE: `compare -metric RMSE <ref-crop> <mlim-crop> /dev/null 2>&1` → Expected: Wave zone RMSE improves from 17.59%
- [ ] Visual check: output meter and GR meter active fill still has visible contrast against the brighter track background

## Technical Details
In `Colours.h`, change:
```cpp
// Current (too dark, causes 52-75 unit darkness gap in input/GR meter zones):
const juce::Colour barTrackBackground { 0xff231417 };
// Try (dark blue-grey, matches waveform theme):
const juce::Colour barTrackBackground { 0xff2A2838 };
```

The goal is to bring the idle track background to around (42,40,56) — still dark for active signal contrast but visually integrated with the waveform display gradient. Adjust in steps of ~8 units, measuring the composite after each rebuild.

Build Standalone only: `cmake --build build --target MLIM_Standalone -j$(nproc)`.

## Dependencies
None
