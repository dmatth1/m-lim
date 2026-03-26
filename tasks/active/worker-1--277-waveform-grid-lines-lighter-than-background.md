# Task 277: Waveform Grid Lines — Make Lighter Than Background

## Description
In Pro-L 2, horizontal dB grid lines in the waveform display appear as **slightly lighter
stripes** on the blue-gray background (barely visible but brighter than the background).

In M-LIM currently:
- `waveformGridLine = 0xff757F97` (#757F97 = RGB 117, 127, 151)
- `displayGradientTop = 0xff8892AA` (#8892AA = RGB 136, 146, 170)
- The grid line is DARKER than the background at the top of the waveform

This creates the opposite visual effect from Pro-L 2 — M-LIM shows dark horizontal stripes
while Pro-L 2 shows subtle lighter stripes. Reference measurements from v1-0005.png:
- Background ~#858DA6 (at mid-waveform, between grid lines)
- Grid line area ~#8C96AE (slightly lighter: +7R, +9G, +8B)

Fix: Change `waveformGridLine` in `Colours.h` to a color slightly LIGHTER than the gradient
midpoint. Target: `0xff9AA0B4` (approximately +10 brightness above gradient top).

Also reduce alpha to make them more subtle: use
`MLIMColours::waveformGridLine.withAlpha(0.6f)` at the draw call in
`WaveformDisplay::drawBackground()` instead of full opacity.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — change `waveformGridLine` constant
Modify: `src/ui/WaveformDisplay.cpp` — add `.withAlpha(0.6f)` to the grid line draw call
Read: `src/ui/WaveformDisplay.cpp` — see `drawBackground()` around line 240

## Acceptance Criteria
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds, exit 0
- [ ] Run: `cd build && ctest --output-on-failure 2>&1 | tail -3` → Expected: all tests pass
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot /tmp/task-271-after.png && stop_app` → Expected: screenshot taken
- [ ] Run: `convert /tmp/task-271-after.png -crop 400x1+200+95 +repage - | convert - -resize 1x1! txt:-` → Expected: the pixel at the -6 dB grid line y-position is LIGHTER than the background pixels at y=85 and y=105 (within ±5 values)
- [ ] Visual check: horizontal grid lines appear as very subtle lighter stripes, not dark lines

## Tests
None

## Technical Details
In `Colours.h`, change:
```cpp
const juce::Colour waveformGridLine { 0xff757F97 };  // OLD — darker than background
```
to:
```cpp
const juce::Colour waveformGridLine { 0xff9AA0B4 };  // lighter than gradient, matches Pro-L 2 subtle grid
```

In `WaveformDisplay::drawBackground()`, change the grid line draw to:
```cpp
g.setColour (MLIMColours::waveformGridLine.withAlpha (0.6f));
```
(was: `g.setColour (MLIMColours::waveformGridLine);`)

Reference: v1-0005.png grid line samples show ~#8C96AE on ~#858DA6 background.
The new value #9AA0B4 is lighter than `displayGradientTop` #8892AA and creates the
correct "lighter stripe" appearance.

Take before/after screenshots and compare:
```bash
source Scripts/ui-test-helper.sh
start_app
screenshot screenshots/task-271-before.png
# ... rebuild ...
screenshot screenshots/task-271-after.png
compare_to_reference /reference-docs/reference-screenshots/prol2-main-ui.jpg \
  screenshots/task-271-after.png 0.15
stop_app
```

## Dependencies
None
