# Task 539: Meter Idle Alpha Tuning (Input + Output)

## Description
Both input and output level meters need idle alpha adjustments to match the Pro-L 2 reference. The input meter (showScale_ == true) is too vivid at 1.0x idle alpha. The output meter (showScale_ == false) is nearly invisible at 0.18f. The reference shows both meters at moderate, clearly visible idle opacity.

These two changes MUST be done together since they modify the same ternary expression on LevelMeter.cpp line 98.

Changes:
1. Input meter idle alpha: 1.0f → 0.65f (less vivid)
2. Output meter idle alpha: 0.18f → 0.35f (more visible)
3. Background alpha top (output): 0.08f → 0.16f
4. Background alpha bottom (output): 0.12f → 0.22f
5. Background alpha top (input): 0.55f → 0.40f
6. Background alpha bottom (input): 0.65f → 0.50f

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/LevelMeter.cpp` — line 98: change aScale ternary; lines 238-239: adjust background alphas
Read: `M-LIM/src/ui/Colours.h` — meter colour constants

## Acceptance Criteria
- [ ] Run: `grep -n 'aScale' M-LIM/src/ui/LevelMeter.cpp` → Expected: shows `showScale_ ? 0.65f : 0.35f`
- [ ] Run: `grep -n 'bgAlpha' M-LIM/src/ui/LevelMeter.cpp` → Expected: input 0.40f/0.50f, output 0.16f/0.22f
- [ ] Run: `cd M-LIM && cmake --build build --config Release --target M-LIM_Standalone -j$(nproc)` → Expected: builds successfully

## Tests
None

## Dependencies
None
