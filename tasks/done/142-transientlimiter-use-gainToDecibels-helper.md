# Task 142: Normalize dB conversion in TransientLimiter to use DspUtil helpers

## Description
`TransientLimiter.cpp` uses inline `20.0f * std::log10(...)` expressions directly in
`computeRequiredGain()` (lines 127-128) instead of the `gainToDecibels()` helper from `DspUtil.h`.
`LevelingLimiter.cpp` already uses `gainToDecibels()` correctly (line 208). This inconsistency
means the same computation has two forms and the minimum-gain floor (`kDspUtilMinGain`) is not
consistently applied in the TransientLimiter path.

Additionally, `TransientLimiter.cpp` defines a local `kMinGain` constant (used in the GR report
on line 333) that is the same as `kDspUtilMinGain`. Use `kDspUtilMinGain` instead.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/TransientLimiter.cpp` — replace raw `20.0f * std::log10(...)` with `gainToDecibels()`, use `kDspUtilMinGain` instead of local `kMinGain`
Read: `M-LIM/src/dsp/DspUtil.h` — `gainToDecibels()` definition
Read: `M-LIM/src/dsp/LevelingLimiter.cpp` — reference for correct usage pattern

## Acceptance Criteria
- [ ] Run: `grep -n "20\.0f \* std::log10" M-LIM/src/dsp/TransientLimiter.cpp` → Expected: no output
- [ ] Run: `grep -n "kMinGain" M-LIM/src/dsp/TransientLimiter.cpp` → Expected: no output (replaced by kDspUtilMinGain)
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | grep -E "error:"` → Expected: no output

## Tests
None

## Technical Details
In `TransientLimiter.cpp`:
1. Add `#include "DspUtil.h"` if not already included (check existing includes)
2. Lines 127-128: replace `20.0f * std::log10(peakAbs)` with `gainToDecibels(peakAbs)` and
   `20.0f * std::log10(mThreshold)` with `gainToDecibels(mThreshold)`
3. Line 333: replace `20.0f * std::log10(std::max(minGain, kMinGain))` with
   `gainToDecibels(std::max(minGain, kDspUtilMinGain))`
4. Remove the local `kMinGain` constant definition

Note: `gainToDecibels` already clamps via `std::max(linearGain, kDspUtilMinGain)` internally,
so the `std::max(minGain, kMinGain)` in the outer call becomes redundant but is safe to leave in
or remove.

## Dependencies
None
