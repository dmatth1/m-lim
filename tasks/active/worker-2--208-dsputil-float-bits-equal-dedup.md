# Task 208: Move floatBitsEqual() to DspUtil.h

## Description
`floatBitsEqual()` is defined as a static function in two separate translation units:
- `LimiterEngine.cpp` (somewhere around line 351–354)
- `SidechainFilter.cpp` (around line 84–87)

Both implementations are identical: compare two floats by their bit representation to avoid spurious parameter updates. This is a general-purpose DSP utility that belongs in `DspUtil.h` alongside `gainToDecibels()` and `decibelsToGain()`.

Move the function to `DspUtil.h` as an `inline` function, then remove both local static definitions and update callers to use the shared version.

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/dsp/DspUtil.h` — add the function here
Modify: `M-LIM/src/dsp/DspUtil.h` — add `inline bool floatBitsEqual(float a, float b)`
Modify: `M-LIM/src/dsp/LimiterEngine.cpp` — remove local static definition, use DspUtil version
Modify: `M-LIM/src/dsp/SidechainFilter.cpp` — remove local static definition, use DspUtil version

## Acceptance Criteria
- [ ] Run: `grep -n "floatBitsEqual" M-LIM/src/dsp/LimiterEngine.cpp M-LIM/src/dsp/SidechainFilter.cpp` → Expected: no `static` definition lines, only call sites
- [ ] Run: `grep -n "floatBitsEqual" M-LIM/src/dsp/DspUtil.h` → Expected: one `inline` definition present
- [ ] Run: `cmake --build build -j4 2>&1 | tail -5` → Expected: build succeeds, exit 0

## Tests
None

## Technical Details
The function is typically:
```cpp
inline bool floatBitsEqual(float a, float b) {
    return std::memcmp(&a, &b, sizeof(float)) == 0;
}
```
Both files must `#include "DspUtil.h"` (they likely already do).

## Dependencies
None
