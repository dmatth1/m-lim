# Task 088: LevelMeter Colour Zones and Scale Marks Don't Match Reference

## Description
Two separate issues with the already-implemented `LevelMeter` component:

**Issue 1 — Wrong bar colour**: `meterSafe` is `0xff4FC3F7` (bright cyan). In the Pro-L 2 reference, the level meter bars for signal-safe levels are a **pale silver-grey** — not bright cyan. Cyan bars will look jarring and off-brand.

**Issue 2 — Inconsistent scale marks**: `kScaleMarks[]` = `{0, -3, -6, -12, -18, -24, -48}`. The steps are non-uniform (3→6→6→6→24 dB jumps). The `-48` marking is especially odd. The reference uses **uniform 3 dB increments**: `{0, -3, -6, -9, -12, -15, -18, -21, -24, -27, -30}`.

Reference: `/reference-docs/video-frames/v1-0040.png` (right panel shows pale grey meter bars and 3 dB step scale labels). Also `/reference-docs/reference-screenshots/prol2-main-ui.jpg`.

## Produces
None

## Consumes
ColoursDefinition

## Relevant Files
Modify: `M-LIM/src/ui/LevelMeter.cpp` — fix `kScaleMarks`, update bar colour zones
Modify: `M-LIM/src/ui/LevelMeter.h` — kWarnDB / kDangerDB thresholds may need adjustment
Modify: `M-LIM/src/ui/Colours.h` — update `meterSafe` colour

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIM_VST3 -j$(nproc) 2>&1 | tail -3` → Expected: builds successfully
- [ ] Run: `grep "kScaleMarks" M-LIM/src/ui/LevelMeter.cpp` → Expected: contains `-9.0f` and `-27.0f` (uniform 3 dB steps, no -48)
- [ ] Run: `grep "meterSafe" M-LIM/src/ui/Colours.h` → Expected: colour value is NOT `4FC3F7` (should be a grey tone)

## Tests
None (visual component)

## Technical Details

**Fix 1 — `meterSafe` colour:**
Change from bright cyan `0xff4FC3F7` to a pale blue-grey matching the Pro-L 2 level meter bars:
```cpp
const juce::Colour meterSafe { 0xff8896AC };  // pale steel grey-blue (was bright cyan 4FC3F7)
```
The warning and danger zones can keep their existing yellow/red colours as those match the reference.

**Fix 2 — `kScaleMarks`:**
Replace the inconsistent array with uniform 3 dB steps:
```cpp
constexpr float kScaleMarks[] = { 0.0f, -3.0f, -6.0f, -9.0f, -12.0f,
                                   -15.0f, -18.0f, -21.0f, -24.0f, -27.0f, -30.0f };
```
Remove the `-48.0f` mark — it maps too close to the bottom of the bar to be readable.

**Optional — clip indicator:**
Pro-L 2 shows a small RED square at the very top of the level meter bar when the signal reaches 0 dBFS. If not done here, see task 078 for the dedicated clip indicator task.

**Threshold check:**
Review `kWarnDB = -6.0f` and `kDangerDB = -1.0f`. In Pro-L 2 the meter turns yellow around -3 dB and red around 0 dB — adjust thresholds:
- `kWarnDB = -3.0f`
- `kDangerDB = -0.5f`

## Dependencies
Requires task 020 (LevelMeter component)
