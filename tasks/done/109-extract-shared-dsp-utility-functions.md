# Task 109: Duplicated gainToDecibels / decibelsToGain Helpers — Extract to DspUtil.h

## Description
Identical `static inline float gainToDecibels(float)` and
`static inline float decibelsToGain(float)` utility functions are defined in two
separate translation units:

- `M-LIM/src/dsp/TransientLimiter.cpp` lines 15–23
- `M-LIM/src/dsp/LevelingLimiter.cpp` lines 17–25

These are copy-paste duplicates. They must be kept identical by hand, and any
future change (e.g., epsilon guard, double precision) would require editing both
files.  JUCE provides `juce::Decibels::gainToDecibels<float>()` and
`juce::Decibels::decibelsToGain<float>()`, but the project uses its own thin
wrappers around `std::log10` / `std::pow` to avoid a JUCE header dependency
inside tight DSP loops.

Fix:
1. Create `M-LIM/src/dsp/DspUtil.h` — a header-only file with the two inline
   functions (and any other small DSP primitives that appear in multiple .cpp files).
2. Remove the duplicate definitions from `TransientLimiter.cpp` and
   `LevelingLimiter.cpp`.
3. `#include "DspUtil.h"` in both .cpp files.
4. Do not change the function signatures, constants (kMinLinear, epsilon guards),
   or behaviour — this is a pure refactor.

## Produces
None

## Consumes
None

## Relevant Files
Create: `M-LIM/src/dsp/DspUtil.h` — shared inline DSP utilities
Modify: `M-LIM/src/dsp/TransientLimiter.cpp` — remove local definitions, add include
Modify: `M-LIM/src/dsp/LevelingLimiter.cpp` — remove local definitions, add include
Read: `M-LIM/CMakeLists.txt` — confirm DspUtil.h does not need to be listed (header-only)

## Acceptance Criteria
- [ ] Run: `grep -c "gainToDecibels" M-LIM/src/dsp/TransientLimiter.cpp` → Expected: 0 definitions (only call sites remain; no `static inline` definition)
- [ ] Run: `grep -c "gainToDecibels" M-LIM/src/dsp/LevelingLimiter.cpp` → Expected: 0 definitions
- [ ] Run: `grep -c "gainToDecibels" M-LIM/src/dsp/DspUtil.h` → Expected: 1
- [ ] Run: `cd M-LIM && cmake --build build -j$(nproc) 2>&1 | tail -5` → Expected: build succeeds, exit 0
- [ ] Run: `cd M-LIM && cd build && ctest --output-on-failure 2>&1 | tail -10` → Expected: all tests pass

## Tests
None — this is a pure refactor; existing tests validate correctness.

## Technical Details
- The function bodies are currently identical in both files; preserve them exactly.
- `DspUtil.h` should use `#pragma once` and include only `<cmath>` (or equivalent).
- No new tests are required beyond confirming the build and existing tests still pass.

## Dependencies
None
