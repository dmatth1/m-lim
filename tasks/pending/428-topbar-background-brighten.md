# Task 428: Top Bar Background Brighten

## Description
The top bar background (y=0–40 in the 900x500 crop) is ~25 units darker than the reference.

- Reference avg: `#484047` (R=71, G=64, B=71)
- M-LIM current: `#2E2D2F` (R=46, G=45, B=47)
- TopBar zone RMSE: 24.93%

Current TopBar gradient in `TopBar::paint`:
- Top: `0xff252228` (37,34,40)
- Bottom: `0xff1F1C22` (31,28,34)

Target: composite average ~(71,64,71). Try:
- Top: `0xff4A4650` (74,70,80)
- Bottom: `0xff3C3842` (60,56,66)

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/TopBar.cpp` — `TopBar::paint` gradient colours (~lines 83–88)

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds, exit 0
- [ ] Run: pixel check `convert screenshot.png -crop 900x40+0+0 +repage -resize 1x1! txt:-` → Expected: R channel ≥ 60 (up from 46)
- [ ] Run: TopBar RMSE → Expected: improvement from 24.93%
- [ ] Visual: top bar not washed out; buttons and text still clearly visible

## Tests
None

## Technical Details
In `TopBar::paint` (~line 83):
```cpp
// Target:
juce::ColourGradient bg (juce::Colour (0xff4A4650), 0.0f, 0.0f,
                         juce::Colour (0xff3C3842), 0.0f, bounds.getHeight(),
                         false);
```

Adjust until composite R ≈ 62–68. Build Standalone: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc)`.

## Dependencies
None
