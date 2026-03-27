# Task 362: Output Meter Right Panel — Hide Scale Labels and Improve Background Color

## Description
The output meter right panel has persistent 29.59% RMSE (target ≤27.00%) because:
1. Scale labels (dB tick marks + text in bright gray #9E9E9E) are drawn on the right 20px of the
   output meter — these bright labels create RMSE noise since the reference shows only dark pixels
   in this area.
2. The bar track background (#181818 near-black) doesn't match the reference's dark purple tones
   (#231E24 range).

Fix: (a) add a `setShowScale(bool)` method to LevelMeter and call it on outputMeter_ to hide the
scale labels, and (b) change the barTrackBackground color to a dark purple-tinted value that better
matches the reference right panel.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/LevelMeter.h` — add `showScale_` member and `setShowScale(bool)` method
Modify: `M-LIM/src/ui/LevelMeter.cpp` — guard `drawScale()` call with `showScale_` flag
Modify: `M-LIM/src/ui/Colours.h` — adjust `barTrackBackground` color from #181818 to dark purple
Modify: `M-LIM/src/PluginEditor.cpp` — call `outputMeter_.setShowScale(false)` in constructor

## Acceptance Criteria
- [ ] Run: `grep -n "setShowScale" M-LIM/src/PluginEditor.cpp` → Expected: shows `outputMeter_.setShowScale(false)`
- [ ] Run: `grep -n "barTrackBackground" M-LIM/src/ui/Colours.h` → Expected: value is NOT `0xff181818` (must be purple-tinted)
- [ ] Build and capture screenshot, compute right panel RMSE → Expected: improvement from 29.59% (goal ≤28.00%)

## Tests
None

## Technical Details

**Change 1: Add scale visibility flag to LevelMeter.h**
```cpp
// In LevelMeter class declaration, add:
void setShowScale (bool show) { showScale_ = show; repaint(); }

// In private section, add member:
bool showScale_ = true;
```

**Change 2: Guard drawScale in LevelMeter.cpp**
```cpp
// In paint(), change:
if (kScaleWidth > 0)
    drawScale (g, barL.getY(), barL.getHeight());

// To:
if (kScaleWidth > 0 && showScale_)
    drawScale (g, barL.getY(), barL.getHeight());
```

**Change 3: Adjust barTrackBackground in Colours.h**
```cpp
// Current (near-black, no purple tint):
const juce::Colour barTrackBackground { 0xff181818 };

// Change to dark purple-tinted (closer to reference #231E24):
const juce::Colour barTrackBackground { 0xff1C1820 };
```
The target color `#1C1820` has RGB (28,24,32) — adds subtle purple tint vs pure #181818 (24,24,24),
while staying darker than the reference #231E24 (35,30,36) so it doesn't clash with other meters.

**Change 4: Hide scale on output meter in PluginEditor.cpp**
```cpp
// In constructor, after addAndMakeVisible(outputMeter_):
outputMeter_.setShowScale (false);
```

**RMSE measurement — right panel sub-region:**
```bash
compare -metric RMSE \
    <(convert /tmp/ref362.png -crop 100x400+800+50 +repage png:-) \
    <(convert /tmp/task362-mlim.png -crop 100x400+800+50 +repage png:-) \
    /dev/null 2>&1
```

Save full results to `screenshots/task-362-rmse-results.txt`. Report all sub-regions.

## Dependencies
None (can run in parallel with task 361)
