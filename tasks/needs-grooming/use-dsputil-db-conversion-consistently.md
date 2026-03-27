# Task: Use DspUtil dB Conversion Helpers Consistently Across DSP Code

## Description
`DspUtil.h` provides `decibelsToGain()` and `gainToDecibels()` helpers, but several DSP files inline their own `std::pow(10.0f, dB / 20.0f)` instead:

- `LimiterEngine.cpp:450` and `:459` — `setInputGain()` and `setOutputCeiling()`
- `TransientLimiter.cpp:199` — `computeRequiredGain()`
- `SidechainFilter.cpp:20-21` and `:207-208` — tilt gain calculations

This inconsistency means:
1. If the conversion formula needs a safety floor (like `decibelsToGain` has), some call sites are unprotected
2. Code is harder to grep for all dB conversions
3. Minor: `DspUtil.h`'s `decibelsToGain()` uses `dB * (1.0f / 20.0f)` (multiply by reciprocal) while the inline versions use `dB / 20.0f` — these should produce identical results but the inconsistency is unnecessary

Replace all inline `std::pow(10.0f, dB / 20.0f)` calls with `decibelsToGain()`.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/LimiterEngine.cpp` — replace `std::pow(10.0f, dB / 20.0f)` at lines 450, 459
Modify: `M-LIM/src/dsp/TransientLimiter.cpp` — replace at line 199
Modify: `M-LIM/src/dsp/SidechainFilter.cpp` — replace at lines 20-21, 207-208 (these need `decibelsToGain(dB * 0.5f)` pattern)
Read: `M-LIM/src/dsp/DspUtil.h` — verify the helper signature

## Acceptance Criteria
- [ ] Run: `grep -n "std::pow(10.0f" src/dsp/LimiterEngine.cpp src/dsp/TransientLimiter.cpp src/dsp/SidechainFilter.cpp` → Expected: no output (all replaced with decibelsToGain)
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass

## Tests
None (existing tests cover the behavior)

## Technical Details
For SidechainFilter, the tilt gain pattern `std::pow(10.0f, -mTiltDb * 0.5f / 20.0f)` becomes `decibelsToGain(-mTiltDb * 0.5f)`. Include `DspUtil.h` in files that don't already include it.

## Dependencies
None
