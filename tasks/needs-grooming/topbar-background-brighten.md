# Task: Top Bar Background Brighten

## Description
The top bar background (y=0-40 in the 900x500 comparison crop) is significantly darker than the reference. Pixel analysis:
- Reference average: `#484047` (R=71, G=64, B=71) — medium dark grey-purple
- M-LIM current: `#2E2D2F` (R=46, G=45, B=47) — darker by ~25 units
- RMSE for topbar zone: 24.93%

The reference Pro-L 2 top bar has a lighter appearance due to its FabFilter branding elements (logo, Pro-L² colored text). M-LIM's top bar can approach this by lightening the gradient background in `TopBar::paint`.

Current TopBar gradient:
- Top: `0xff252228` = (37,34,40)
- Bottom: `0xff1F1C22` = (31,28,34)

Target: composite average should approach (71,64,71). The gradient needs to be approximately doubled in brightness: try top `0xff4A4650` = (74,70,80) and bottom `0xff3C3842` = (60,56,66).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/TopBar.cpp` — TopBar::paint gradient colors (lines ~83-88)

## Acceptance Criteria
- [ ] Run pixel check: `convert screenshots/task-topbar-after.png -crop 900x40+0+0 +repage -resize 1x1! txt:-` → Expected: R channel ≥ 60 (up from 46)
- [ ] Run RMSE on topbar: `compare -metric RMSE <ref-topbar-crop> <mlim-topbar-crop> /dev/null 2>&1` → Expected: TopBar RMSE improves from 24.93%
- [ ] Visual check: top bar should not appear washed out; elements (buttons, text) should still be clearly visible

## Technical Details
In `TopBar::paint` (~line 83), change the gradient:
```cpp
// Current (too dark):
juce::ColourGradient bg (juce::Colour (0xff252228), 0.0f, 0.0f,
                         juce::Colour (0xff1F1C22), 0.0f, bounds.getHeight(),
                         false);
// Try (brightened ~+37 units):
juce::ColourGradient bg (juce::Colour (0xff4A4650), 0.0f, 0.0f,
                         juce::Colour (0xff3C3842), 0.0f, bounds.getHeight(),
                         false);
```

Adjust until composite R ≈ 62-68. Build Standalone: `cmake --build build --target MLIM_Standalone -j$(nproc)`.

## Dependencies
None
