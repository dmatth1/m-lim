# Task 530: Fix DCFilter Denormal Flush Threshold

## Description
`DCFilter::process()` (line 23 of `src/dsp/DCFilter.cpp`) flushes `yPrev` to zero when `std::abs(yPrev) < 1e-15f`. This threshold is problematic:

1. **1e-15f is too aggressive** — it zeros out legitimate tiny signals well above the denormal range (float denormals start at ~1.4e-45f). A value of 1e-15 is about -300 dBFS, still within normal precision.
2. **The flush happens after output** — `data[i] = y` is written before `yPrev` is potentially flushed, creating a one-sample inconsistency.
3. **Redundant with ScopedNoDenormals** — the DAZ/FTZ flags already handle denormals at the CPU level.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/DCFilter.cpp` — fix threshold at line 23

## Acceptance Criteria
- [ ] Run: `cd build && ctest --output-on-failure -R dc_filter` → Expected: all DC filter tests pass
- [ ] Run: `grep -n "1e-15" M-LIM/src/dsp/DCFilter.cpp` → Expected: no matches (threshold changed or removed)

## Tests
- Unit: `tests/dsp/test_dc_filter.cpp::test_tiny_signal_preserved` — verify that a signal at 1e-20 amplitude passes through without being zeroed

## Technical Details
Either:
- Remove the manual flush entirely (rely on ScopedNoDenormals)
- Or change threshold to ~1e-30f which is below any audible signal but above float denormal range
- Apply flush before output: `yPrev = y; if (...) yPrev = 0.0f; data[i] = yPrev;`

## Dependencies
None
