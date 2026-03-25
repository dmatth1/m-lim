# Task 185: Remove Dead Code `(void)` Suppressions in WaveformDisplay

## Description
`WaveformDisplay.cpp` contains four draw methods that declare a local variable, never use it, and silence the resulting compiler warning with explicit `(void)` casts:

- `drawOutputFill()` (≈line 306): `float colW = area.getWidth() / ...;` then `(void) colW;`
- `drawInputFill()` (≈line 343): `int total = std::min(...); ` then `(void) total;`
- `drawGainReduction()` (≈line 375): same `(void) total;` pattern
- `drawOutputEnvelope()` (≈line 402): same `(void) total;` pattern

These are copy-paste artifacts from an earlier version of the code where `colW`/`total` were used in the loop body. The lambda passed to `forEachFrame` now receives `totalCols` directly as its third parameter. The outer variables serve no purpose.

Remove the redundant declarations and their `(void)` suppressors entirely. This also removes the mismatch where `drawOutputFill` computes `colW` outside the lambda but then recomputes `area.getWidth() / totalCols` inside it.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/WaveformDisplay.cpp` — remove 4 × (unused variable + void-cast) pairs

## Acceptance Criteria
- [ ] Run: `grep -n "(void) colW\|(void) total" M-LIM/src/ui/WaveformDisplay.cpp` → Expected: no matches
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | grep -i warning | grep -i waveform` → Expected: no output (no new warnings)
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | tail -5` → Expected: build succeeds, exit 0

## Tests
None

## Technical Details
For each affected function, delete the line that declares the unused variable and the corresponding `(void) variable;` line inside the lambda or after it.

Example in `drawOutputFill()`:
- Delete: `float colW = area.getWidth() / static_cast<float> (total);`
- Delete: `(void) colW;` inside the lambda

The `total` variable IS used in `drawOutputFill` for computing `colW` (which is itself dead), so once `colW` is removed `total` also becomes unused. Remove it too if it is not used elsewhere in that function.

## Dependencies
None
