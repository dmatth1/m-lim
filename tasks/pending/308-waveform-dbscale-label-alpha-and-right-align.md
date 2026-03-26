# Task 308: Waveform — dB Label Alpha Reduction and Right-Align

## Description
The dB scale overlay labels in the waveform display (e.g. "−3 dB", "−6 dB", ...) are drawn at `textPrimary.withAlpha(0.75f)` (bright near-white). Comparing against the reference (v1-0010, v1-0040), the labels look somewhat similar but the M-LIM labels appear slightly brighter/more prominent than needed.

Additionally, in v1-0010 (showing the Pro-L 2 waveform with context menu), the dB labels appear to be **right-aligned against the left waveform border**, while M-LIM draws them **left-aligned** (left-padded from `area.getX() + 2.0f`). This means the text extends rightward INTO the waveform content more than it should.

## Fix

In `WaveformDisplay.cpp`, `drawBackground()`:

1. Change `alpha` from `0.75f` to `0.55f` for the dB overlay labels (less prominent, blend better with waveform content).
2. Change label alignment from `juce::Justification::centredLeft` to `juce::Justification::centredRight` and set the label rect to extend from `area.getX() + 2.0f` with a narrower fixed width (e.g., 38px instead of 44px).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/WaveformDisplay.cpp` — `drawBackground()` lines ~303–324

## Acceptance Criteria
- [ ] Run: `cmake --build M-LIM/build --config Release -j$(nproc) 2>&1 | tail -5` → Expected: build succeeds, exit 0
- [ ] Run: launch app, take screenshot → Expected: dB scale labels (−3 dB, −6 dB …) still visible but slightly more muted (blending better with the gradient background)
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot screenshots/task-308-after.png && stop_app` → Expected: screenshot saved
- [ ] Run: `compare -metric RMSE screenshots/task-308-after.png /reference-docs/reference-screenshots/prol2-main-ui.jpg null: 2>&1` → Expected: RMSE for waveform region not worse than task-299 baseline 0.2303

## Tests
None

## Technical Details
- `WaveformDisplay.cpp` line ~322: `float alpha = 0.75f;` → `float alpha = 0.55f;`
- Same function, label rect: width `44.0f` → `38.0f`; justification `centredLeft` → `centredRight`
- The labels still read clearly at 0.55 alpha against the gradient background (textPrimary = #E0E0E0, at 55% = effectively ~#7B7B7B over the gradient)

## Dependencies
None (can run in parallel with 304, 305, 306, 307)
