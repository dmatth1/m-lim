# Task: Add safety assertion for SidechainFilter raw coefficient access

## Description
`SidechainFilter::updateCoefficients()` directly manipulates JUCE IIR filter coefficients via `getRawCoefficients()` (around line 156 of `SidechainFilter.cpp`), assuming a 5-element `[b0, b1, b2, a1, a2]` layout. This is a fragile coupling to JUCE internals — if JUCE changes its coefficient layout, the code silently breaks.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/SidechainFilter.cpp` — add assertion and documentation for coefficient layout assumption

## Acceptance Criteria
- [ ] Run: `cd build && ctest --output-on-failure -R sidechain` → Expected: all sidechain tests pass
- [ ] Run: `grep -n "getRawCoefficients" M-LIM/src/dsp/SidechainFilter.cpp` → Expected: each usage has a nearby jassert or comment

## Tests
None (safety assertion, no behavior change)

## Technical Details
Before each `getRawCoefficients()` call, add:
```cpp
// JUCE IIR::Coefficients layout: [b0, b1, b2, a1, a2] (5 floats, normalized)
jassert(mHP[ch].coefficients != nullptr);
```
And add a comment documenting the assumed layout so future developers know to verify this if upgrading JUCE.

## Dependencies
None
