# Task: Darken Top Bar Gradient to Match Reference Dark Gray

## Description
Beyond the JUCE standalone yellow banner (separate task), the M-LIM top bar area at Y=60-65 shows srgb(73,69,79) vs the reference's srgb(43,38,44). The top bar gradient colors are too bright:

- `topBarGradientTop` = 0xff4A4650 (74,70,80)
- `topBarGradientBottom` = 0xff3C3842 (60,56,66)

The reference shows a consistent dark background ~(43,38,44) across the top bar, with button/text elements as the only brightness.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — `topBarGradientTop` and `topBarGradientBottom`
Read: `src/ui/TopBar.cpp` — top bar rendering

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc)` → Expected: builds successfully
- [ ] Run: screenshot comparison → Expected: top bar background is darker, closer to reference ~(43,38,44)

## Tests
None

## Technical Details
- Target `topBarGradientTop`: change from 0xff4A4650 to ~0xff2D2A30 (45,42,48)
- Target `topBarGradientBottom`: change from 0xff3C3842 to ~0xff2B262C (43,38,44)
- The reference top bar is almost uniform dark purple-gray with subtle warmth
- Also review `topBarPresetBackground` (0xff2D2D2D) — it may need matching adjustment

## Dependencies
None
