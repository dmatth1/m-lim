# Task 330: Fix Waveform Display Gradient Colors

## Description
The waveform display background gradient colors in `Colours.h` are wrong. The code comment even
states the reference measurement (`#8992AB` top, `#6F7790` middle), but the actual constants
use completely different values:

- `displayGradientTop = 0xff686468` (#686468) — a neutral/desaturated gray, should be `#8992AB`
- `displayGradientBottom = 0xff506090` (#506090) — too saturated/dark blue

**Pixel measurements from reference (`/reference-docs/video-frames/v1-0005.png`):**
```
y=100 (top of waveform area):  RGB(137, 146, 171) = #8992AB
y=150:                          RGB(126, 134, 159) = #7E869F
y=200 (middle):                 RGB(111, 119, 144) = #6F7790
y=250:                          RGB(95, 103, 127)  = #5F677F
y=300 (bottom):                 RGB(93, 97, 123)   = #5D617B
```

The reference waveform gradient is a blue-gray, lighter at the top and darker toward the bottom.
Our current top is too dark and too gray-neutral. Our current bottom is too blue/saturated.

Also update the grid line color `waveformGridLine` from `#9AA0B4` to a slightly darker/more-opaque
value since the background will now be lighter (improving contrast).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — update `displayGradientTop`, `displayGradientBottom`, update comment

## Acceptance Criteria
- [ ] Run: `grep "displayGradientTop" /workspace/M-LIM/src/ui/Colours.h` → Expected: contains `0xff8992AB`
- [ ] Run: `grep "displayGradientBottom" /workspace/M-LIM/src/ui/Colours.h` → Expected: contains `0xff5C6880` or similar (darker/less-saturated than before)
- [ ] Run: `cd /workspace/M-LIM && cmake --build build -j$(nproc) 2>&1 | grep -c "error:"` → Expected: `0`
- [ ] Build, screenshot, compare to reference — Expected: waveform area visually brighter/more blue-gray, matching reference

## Tests
None

## Technical Details
In `src/ui/Colours.h`, change the waveform display gradient constants:

```cpp
// BEFORE:
const juce::Colour displayGradientTop   { 0xff686468 };  // neutral/warm gray, matches ref top
const juce::Colour displayGradientBottom{ 0xff506090 };  // more blue-saturated, matches ref center

// AFTER:
const juce::Colour displayGradientTop   { 0xff8992AB };  // steel-blue/gray, matches ref top (#8992AB measured from v1-0005.png)
const juce::Colour displayGradientBottom{ 0xff5C6880 };  // darker steel-blue at bottom, matches ref bottom (#5D617B measured)
```

Also update the comment line above:
```cpp
// Reference samples from Pro-L 2: top ~#8992AB, middle ~#6F7790 (measured from v1-0005.png)
```
This already exists but was not used — now the values match.

The `waveformGridLine` stays at `0xff9AA0B4` (it will naturally contrast better against
the brighter background).

## Dependencies
None
