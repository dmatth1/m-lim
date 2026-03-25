# Task 139: Remove kMinLinear duplicate in LimiterEngine.cpp

## Description
`LimiterEngine.cpp` defines `static constexpr float kMinLinear = 1e-6f;  // -120 dB floor` at line 7.
This is an exact duplicate of `kDspUtilMinGain = 1e-6f` already defined in `DspUtil.h`.
Both have the same value and the same semantic meaning. Remove the local duplicate and use the
`DspUtil.h` constant throughout `LimiterEngine.cpp`.

Additionally, in `LimiterEngine.cpp` line 316 the variable named `grL` stores the GainReduction
from TransientLimiter — the name `grL` misleadingly suggests "left channel GR". Rename it to
`grStage1` (and `grS` to `grStage2`) for clarity.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/LimiterEngine.cpp` — remove `kMinLinear` local constant (line 7), replace all uses with `kDspUtilMinGain`; rename `grL`/`grS` variables at line 314-316
Read: `M-LIM/src/dsp/DspUtil.h` — source of `kDspUtilMinGain`

## Acceptance Criteria
- [ ] Run: `grep -n "kMinLinear" M-LIM/src/dsp/LimiterEngine.cpp` → Expected: no output (constant removed)
- [ ] Run: `grep -n "kDspUtilMinGain" M-LIM/src/dsp/LimiterEngine.cpp` → Expected: at least one hit where `kMinLinear` was used
- [ ] Run: `grep -n "grL\b\|grS\b" M-LIM/src/dsp/LimiterEngine.cpp` → Expected: no output (renamed)
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | grep -E "error:"` → Expected: no output

## Tests
None

## Technical Details
- `LimiterEngine.cpp` includes `LimiterEngine.h` but not `DspUtil.h` directly. Add `#include "DspUtil.h"` to `LimiterEngine.cpp`.
- Uses of `kMinLinear`: line 253 (`std::max(inputGain, kMinLinear)`) — replace with `kDspUtilMinGain`
- GR rename: line 314 `const float grL = ...` → `const float grStage1 = ...`; line 315 `const float grS = ...` → `const float grStage2 = ...`; line 316 update the expression accordingly

## Dependencies
None
