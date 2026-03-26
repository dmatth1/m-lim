# Task 240: SidechainFilter IIR State Not Reset After Coefficient Change

## Description

`SidechainFilter::updateCoefficients()` overwrites the biquad coefficients of all four
IIR filters (HP, LP, low-shelf, high-shelf) in-place via `getRawCoefficients()`, but
does **not** reset the delay elements (internal state) of those filters. When a user
changes the sidechain HP/LP frequency or tilt while audio is playing, the filter delay
elements — accumulated under the old coefficients — are immediately used with the new
coefficients. This produces a transient spike or dip in the detection signal which causes
the limiter to apply a brief burst of incorrect gain reduction, producing an audible
click or pump artefact in the plugin output.

### Location
`src/dsp/SidechainFilter.cpp` — `updateCoefficients()`, lines 122–282.
Called from `process()` at line 61 when `mCoeffsDirty` is true.

### Current Behaviour
```cpp
void SidechainFilter::updateCoefficients()
{
    // ... writes raw[0..4] for each filter/channel ...
    // No reset() called — stale delay state remains!
}
```

### Required Fix
After all four filter coefficient blocks have been updated, reset the delay state of
every filter that was modified:

```cpp
// At the end of updateCoefficients():
for (int ch = 0; ch < kMaxChannels; ++ch)
{
    mHP[ch].reset();
    mLP[ch].reset();
    mTiltLS[ch].reset();
    mTiltHS[ch].reset();
}
```

`juce::dsp::IIR::Filter::reset()` zeroes the delay elements only — no heap allocation.
It is safe to call from the audio thread.

Note: resetting introduces a brief (< 1 filter-order = 2 samples) zero-input transition,
but this is far preferable to the transient spike from stale-coefficient state mismatch.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/dsp/SidechainFilter.cpp` — add `reset()` calls at end of `updateCoefficients()`
Read: `src/dsp/SidechainFilter.h` — filter members (`mHP`, `mLP`, `mTiltLS`, `mTiltHS`)
Read: `tests/dsp/test_sidechain_filter.cpp` — existing tests

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R test_sidechain_filter --output-on-failure` → Expected: all tests pass
- [ ] Add and run a test that: (a) processes a block at one HP frequency, (b) changes HP frequency, (c) processes the next block, and verifies the output immediately after the change does not contain a sample with absolute value > 2× the input amplitude (no transient spike)

## Tests
- Unit: `tests/dsp/test_sidechain_filter.cpp` — add test `test_coefficient_change_no_transient_spike` that changes HP frequency mid-stream and checks the output block after the change has no sample exceeding 2× the input ceiling

## Technical Details
- `juce::dsp::IIR::Filter<float>::reset()` is defined in JUCE as: sets `v[0] = v[1] = 0.f` (the two direct-form-II delay elements)
- No allocation occurs — the call is O(1) and RT-safe
- `kMaxChannels = 2` so the loop covers both stereo channels

## Dependencies
None
