# Task 541: Loudness Histogram Idle Fill Alpha Boost

## Description
The idle warm-fill simulation in the loudness histogram is too faint (gaussian * 0.28f). The Pro-L 2 reference shows clearly visible warm amber histogram bars. Increase the alpha multiplier to 0.45f.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/LoudnessPanel.cpp` — line 369: change `gaussian * 0.28f` to `gaussian * 0.45f`

## Acceptance Criteria
- [ ] Run: `grep -n 'gaussian.*0\.' M-LIM/src/ui/LoudnessPanel.cpp` → Expected: shows `gaussian * 0.45f`
- [ ] Run: `cd M-LIM && cmake --build build --config Release --target M-LIM_Standalone -j$(nproc)` → Expected: builds successfully

## Tests
None

## Dependencies
None
