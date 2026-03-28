# Task: Darken Loudness Histogram Gradient Background

## Description
The loudness histogram area gradient is too bright compared to Pro-L 2 reference. Pixel comparison at histogram region: M-LIM renders ~(55,54,69) while reference shows ~(23,21,24). The `loudnessHistogramTop` (0xff383848) and `loudnessHistogramBottom` (0xff303040) constants in Colours.h need significant darkening to approximately (24,22,26) / (20,18,22) to match the reference's very dark histogram background.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — darken `loudnessHistogramTop` from `0xff383848` to approximately `0xff18161A` and `loudnessHistogramBottom` from `0xff303040` to approximately `0xff141216`
Read: `src/ui/LoudnessPanel.cpp` — histogram paint section uses these gradient constants
Read: `/reference-docs/reference-screenshots/prol2-main-ui.jpg` — reference histogram area is very dark

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM && export CCACHE_DIR=/build-cache/ccache && cmake --build build --target MLIM_Standalone -j$(nproc)` → Expected: builds successfully
- [ ] Run: screenshot and pixel sample at histogram area → Expected: pixel values closer to (23,21,24) than current (55,54,69)

## Tests
None

## Technical Details
- Current loudnessHistogramTop: `0xff383848` = RGB(56,56,72) — ~32 units too bright per channel
- Current loudnessHistogramBottom: `0xff303040` = RGB(48,48,64) — ~28 units too bright
- Target: approximately RGB(24,22,26) / RGB(20,18,22) — matches reference near-black
- The histogram area in Pro-L 2 has a very dark background with bright colored bars drawn on top; our background is too bright, washing out contrast
- The idle bar fill (from separate grooming task) will look better against a darker background

## Dependencies
None
