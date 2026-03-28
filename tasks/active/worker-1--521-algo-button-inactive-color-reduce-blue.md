# Task 521: Reduce algoButtonInactive Blue Excess to Match Reference

## Description

Pixel analysis of the algorithm selector buttons (TR, PU, DY, AG, AR, BU, SA, MO)
shows the inactive button background color has too much blue compared to reference:

- M-LIM current `algoButtonInactive = 0xff545870` = (84, 88, 112)
- Reference area average: (84, 84, 99)
- Gap: G+4, B+13 (buttons are too saturated blue)

**Fix**: Change `algoButtonInactive` in Colours.h from `0xff545870` to `0xff545463`:
- R=84, G=84, B=99 — matches reference

This is a single-line Colours.h change. Also affects ADVANCED panel background in
ControlStrip::paint() which uses the same color.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — change algoButtonInactive from 0xff545870 to 0xff545463

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds
- [ ] Run: Build standalone, launch on Xvfb, capture screenshot, measure button area average color → Expected: values within ±5 of (84,84,99)
- [ ] Run: Full RMSE compare → Expected: ≤ 19.46% (no regression from task-498 baseline)

## Tests
None

## Technical Details
- Single line change in Colours.h
- `algoButtonInactive = 0xff545463`: 0x54=84 (R), 0x54=84 (G), 0x63=99 (B)

## Dependencies
None
