# Task 306: Loudness Panel — Increase Large LUFS Readout Font Size

## Description
In the reference Pro-L 2 (video frame v1-0040), the large short-term LUFS readout at the bottom of the loudness panel displays the value (e.g. "−10.0") in a **very large bold font** that visually dominates its area. It appears to be approximately 36–40px tall based on panel proportions.

M-LIM currently uses `juce::Font (28.0f, juce::Font::bold)` for this readout, which is noticeably smaller than the reference. The readout area height (`kLargeReadoutH`) must be increased to match.

Also, the "LUFS" unit label below the large value should use a slightly larger font as well (currently 10pt, reference appears ~11–12pt).

## Fix

In `LoudnessPanel.cpp`, in `drawLargeReadout()`:
1. Change the value font from `28.0f` to `36.0f` bold.
2. Change the "LUFS" unit label font from `kFontSizeMedium` (10pt) to `kFontSizeLarge` (11pt) or 12pt.

In `LoudnessPanel.h`:
3. Increase `kLargeReadoutH` from its current value to accommodate the larger font. Check current value and increase by ~15–20px.

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/ui/LoudnessPanel.h` — find `kLargeReadoutH` constant and adjust
Modify: `M-LIM/src/ui/LoudnessPanel.cpp` — `drawLargeReadout()` font size changes

## Acceptance Criteria
- [ ] Run: `cmake --build M-LIM/build --config Release -j$(nproc) 2>&1 | tail -5` → Expected: build succeeds, exit 0
- [ ] Run: launch app, check loudness panel bottom area → Expected: "---" or LUFS value text appears noticeably larger than before (visually closer to reference)
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot screenshots/task-306-after.png && stop_app` → Expected: screenshot saved
- [ ] Run: compare screenshot to `/reference-docs/video-frames/v1-0040.png` visually → Expected: large readout proportions closer to reference

## Tests
None

## Technical Details
- `LoudnessPanel.h` line ~124: `kLargeReadoutH = 48` → `kLargeReadoutH = 62`
  - This increases the readout strip by 14px; `kReadoutAreaH` (computed as `kPadding + 5*kRowH + kPadding + kLargeReadoutH + kMeasureBtnRowH`) will grow from 190 to 204, reducing the histogram area by 14px.
- `LoudnessPanel.cpp` `drawLargeReadout()`:
  - Value font: `juce::Font (28.0f, juce::Font::bold)` → `juce::Font (38.0f, juce::Font::bold)`
  - LUFS label: `juce::Font (MLIMColours::kFontSizeMedium)` → `juce::Font (12.0f)`
- Verify that the LUFS readout area does not overflow into the button row below it by confirming the text fits within `kLargeReadoutH = 62` (upper 2/3 for value = ~41px, plenty for 38pt text)

## Dependencies
None
