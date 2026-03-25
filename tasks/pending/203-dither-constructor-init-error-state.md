# Task 203: Initialize Dither Error State in Constructor

## Description
`Dither` uses `mError1` and `mError2` member variables for second-order noise-shaping error feedback. These are set in `prepare()` and `setBitDepth()`/`setNoiseShaping()`, but are not initialised in the constructor. If any code path calls `process()` before `prepare()` (for example, a host that fills a buffer before calling `prepareToPlay()`), the error accumulators hold undefined stack values, producing loud pops.

Fix: initialise `mError1` and `mError2` to `0.0f` in the constructor member-initialiser list.

This is a one-line fix but it closes a genuine undefined-behaviour path.

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/dsp/Dither.h` — find constructor declaration and member variable list
Modify: `M-LIM/src/dsp/Dither.h` — add default member initialisers `mError1{0.0f}` and `mError2{0.0f}`, OR
Modify: `M-LIM/src/dsp/Dither.cpp` — add member-initialiser list to constructor if it has a body there

## Acceptance Criteria
- [ ] Run: `grep -n "mError1\|mError2" M-LIM/src/dsp/Dither.h` → Expected: both are initialised to `0.0f` at declaration or in a constructor initialiser list
- [ ] Run: `cmake --build build -j4 2>&1 | tail -5` → Expected: build succeeds, exit 0
- [ ] Run: `cd build && ctest -R dither --output-on-failure 2>&1 | tail -5` → Expected: all dither tests pass

## Tests
Unit: `tests/dsp/test_dither.cpp` — add a test that calls `process()` on a freshly-constructed `Dither` (without calling `prepare()`) and verifies the output is finite (no NaN/Inf), confirming the error state was zero-initialised.

## Technical Details
Prefer in-class member initialisers (C++11):
```cpp
// Dither.h
float mError1 { 0.0f };
float mError2 { 0.0f };
```
This is simpler than adding a constructor body.

## Dependencies
None
