# Task 140: Name the BS.1770-4 filter frequency constants in LoudnessMeter.cpp

## Description
`LoudnessMeter.cpp` uses two magic number frequency values for the ITU-R BS.1770-4 K-weighting
filter design:
- Line 69: `1681.974450955533` (pre-filter high-shelf centre frequency, Hz)
- Line 95: `38.13547087602444` (RLB high-pass corner frequency, Hz)

These are precise values from the BS.1770-4 specification. They appear as bare literals with no
names, making the code hard to audit against the standard. Extract them to named `constexpr`
constants at the top of `LoudnessMeter.cpp` (or in the `LoudnessMeter.h` private section if
shared across methods).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/LoudnessMeter.cpp` — replace magic number literals with named constants
Read: `M-LIM/src/dsp/LoudnessMeter.h` — check if constants belong in header

## Acceptance Criteria
- [ ] Run: `grep -n "1681\.974\|38\.135" M-LIM/src/dsp/LoudnessMeter.cpp` → Expected: only in constant declarations, not as bare literals in function bodies
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | grep -E "error:"` → Expected: no output

## Tests
None

## Technical Details
Add near the top of `LoudnessMeter.cpp`, after includes:
```cpp
static constexpr double kBS1770PreFilterFreqHz = 1681.974450955533;  // ITU-R BS.1770-4 pre-filter Fc
static constexpr double kBS1770RLBHighPassFreqHz = 38.13547087602444; // ITU-R BS.1770-4 RLB HP Fc
```
Replace occurrences on lines 69 and 95 with the new constants.

## Dependencies
None
