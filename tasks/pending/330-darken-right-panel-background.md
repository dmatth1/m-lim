# Task 330: Darken Right Panel Background Color

## Description
The right panel (loudness histogram + LUFS readouts) background is currently #2B2729
(R=43, G=39, B=41). Pixel sampling of the reference (prol2-main-ui.jpg at 900x500) in the
static scale-label area of the right panel (x=770-800, y=150-400) consistently shows
#1D1B20 to #1B191E (R≈29, G≈27, B≈32).

The current value is ~15 units too bright across all channels. Darkening it to #1E1C21
(splitting the difference between measured reference values) will reduce the RMSE in the
right panel region (currently stuck at 29.66%).

Note: The main contributor to right-panel RMSE is that the reference was captured with active
audio (showing histogram bars) while our plugin shows empty background at rest. Making the
background color accurate reduces the luminance delta in the ~50% of pixels where the
reference shows background rather than histogram bars.

Pixel measurements (x=800, y=50-400 in 900x500 reference vs M-LIM):
  y=50:  ref=#48454B  mlim=#2B2729  (ΔR=+29, ΔG=+26, ΔB=+18 — too light)
  y=120: ref=#1B191E  mlim=#2B2729  (ΔR=+16, ΔG=+14, ΔB=+11)
  y=200: ref=#1B191E  mlim=#2B2729  (similar)
  y=770: ref=#171517  mlim=#2B2729  (ΔR=+20, ΔG=+18, ΔB=+12)

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/Colours.h` — change `loudnessPanelBackground` constant

## Acceptance Criteria
- [ ] Run: `cmake --build M-LIM/build --config Release -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds, exit 0
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot "screenshots/task-330-after.png" && stop_app` → Expected: screenshot saved
- [ ] Visual: Right panel (loudness histogram + LUFS readout area) is noticeably darker, closer to the reference's near-black background

## Tests
None

## Technical Details
In `M-LIM/src/ui/Colours.h`, change the `loudnessPanelBackground` constant:

```cpp
// BEFORE:
const juce::Colour loudnessPanelBackground { 0xff2B2729 };  // medium dark purple-gray, matches Pro-L 2 loudness panel

// AFTER:
const juce::Colour loudnessPanelBackground { 0xff1E1C21 };  // dark near-black purple-gray, matches Pro-L 2 reference (#1D1B20 measured)
```

This is the only change needed. The LoudnessPanel uses this color as its `fillRoundedRectangle`
background in `paint()`. All LUFS readout row backgrounds and histogram empty areas inherit
from this fill.

## Dependencies
None (can be done in parallel with tasks 328 and 329)
