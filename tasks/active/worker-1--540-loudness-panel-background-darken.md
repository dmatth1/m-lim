# Task 540: Loudness Panel Background Darken

## Description
The loudness panel background (#3A384A) is too bright and blue compared to the Pro-L 2 reference (~#2B2729 dark warm gray). Darken to approximately #302E3A. Also darken the histogram gradient colours to match.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/Colours.h` — line 65: `loudnessPanelBackground` 0xff3A384A → 0xff302E3A; line 68: `loudnessHistogramTop` 0xff424050 → 0xff383648; line 69: `loudnessHistogramBottom` 0xff3A384A → 0xff302E3A

## Acceptance Criteria
- [ ] Run: `grep -n 'loudnessPanelBackground' M-LIM/src/ui/Colours.h` → Expected: `0xff302E3A`
- [ ] Run: `grep -n 'loudnessHistogramTop' M-LIM/src/ui/Colours.h` → Expected: `0xff383648`
- [ ] Run: `cd M-LIM && cmake --build build --config Release --target M-LIM_Standalone -j$(nproc)` → Expected: builds successfully

## Tests
None

## Dependencies
None
