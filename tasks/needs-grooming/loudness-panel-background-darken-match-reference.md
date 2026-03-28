# Task: Loudness Panel Background Darken to Match Reference

## Description
The loudness panel background (#3A384A) is too bright and blue compared to the Pro-L 2 reference, which shows a much darker background (~#2B2729 dark warm gray) in the right panel area. This brightness mismatch drives up both the Left and Right region RMSE. Darken the background to approximately #302E3A (a compromise between the current value and the reference's very dark appearance).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — line 65: change `loudnessPanelBackground` from `0xff3A384A` to `0xff302E3A`
Modify: `src/ui/Colours.h` — line 68: change `loudnessHistogramTop` from `0xff424050` to `0xff383648`
Modify: `src/ui/Colours.h` — line 69: change `loudnessHistogramBottom` from `0xff3A384A` to `0xff302E3A`

## Acceptance Criteria
- [ ] Run: `grep -n 'loudnessPanelBackground' src/ui/Colours.h` → Expected: `0xff302E3A`
- [ ] Run: Build standalone, launch headless, capture screenshot, compare left crop RMSE → Expected: Left RMSE < 23%

## Tests
None

## Technical Details
These values were last tuned in task-536 (brightened for RMSE). The reference right panel is darker than the current colors. Going darker will bring the panel closer to the reference's warm dark gray. The histogram gradient colors should track to maintain consistency.

## Dependencies
None
