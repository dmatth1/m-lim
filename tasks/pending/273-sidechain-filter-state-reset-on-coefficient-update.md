# Task 273: SidechainFilter resets IIR state on every coefficient update, causing false GR transients

## Description

`SidechainFilter::updateCoefficients()` ends with a hard reset of all 4 filter states for all channels:

```cpp
// SidechainFilter.cpp:283–293
for (int ch = 0; ch < kMaxChannels; ++ch)
{
    mHP[ch].reset();   // clears z1, z2
    mLP[ch].reset();
    mTiltLS[ch].reset();
    mTiltHS[ch].reset();
}
```

This is called from `process()` on the audio thread whenever `mCoeffsDirty` is set (i.e., whenever the user adjusts the sidechain HP frequency, LP frequency, or tilt parameter).

**The bug:** Resetting filter state from accumulated signal content to 0 creates a step discontinuity in the filtered sidechain signal. The sidechain feeds both `TransientLimiter` (Stage 1) and drives the detection path. A step to 0 is perceived by the limiter as a sudden peak disappearance followed by a sharp re-onset — this manifests as:
1. A brief false-low-peak period (GR releases prematurely)
2. A sharp re-limiting event when real signal re-enters the filter

In practice this sounds as a click or brief pumping artifact on the output whenever the user touches a sidechain EQ knob.

The reset was presumably added to avoid a transient from "stale state under old coefficients." However, the IIR filter naturally adapts within a few samples after coefficient changes (the state decays toward the new steady state), and this is universally preferable to a hard discontinuity.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/SidechainFilter.cpp` — `updateCoefficients()` at lines 283–293; remove `reset()` calls

## Acceptance Criteria
- [ ] Run: `cd M-LIM/build && ctest -R SidechainFilter --output-on-failure` → Expected: all SidechainFilter tests pass
- [ ] Manual test: apply a 1 kHz sine wave, set HP freq to 100 Hz, then change HP freq to 200 Hz mid-signal. The output must not contain a click or audible GR transient at the moment of the change.

## Tests
- Unit: Write a test that: (1) prepares a SidechainFilter with a 100 Hz HP cutoff, (2) processes 512 samples of 1 kHz sine to build up filter state, (3) changes the HP cutoff to 200 Hz, (4) processes one more sample, and (5) asserts that the filter output does not jump discontinuously (output sample N+1 must be within ±0.1 of output sample N, not jump to near-zero).

## Technical Details

**Fix**: Remove the four `reset()` calls at the end of `updateCoefficients()`:

```cpp
// REMOVE these lines from the end of SidechainFilter::updateCoefficients():
for (int ch = 0; ch < kMaxChannels; ++ch)
{
    mHP[ch].reset();    // ← DELETE
    mLP[ch].reset();    // ← DELETE
    mTiltLS[ch].reset();// ← DELETE
    mTiltHS[ch].reset();// ← DELETE
}
```

Without the reset, the JUCE `dsp::IIR::Filter` will continue using its existing `z1`/`z2` state on the first sample after the coefficient change. The output for that sample may not be perfectly correct under the new coefficients, but the deviation will be small (proportional to the state magnitude at the moment of change) and will decay naturally within ~10 samples at audio rates. This is far preferable to a zero-discontinuity.

The original code comment says: "Reset delay elements to avoid transient spikes caused by stale state accumulated under the old coefficients." In practice, the "stale state transient" is smaller and shorter than the reset discontinuity, making this a harmful optimization. The comment should also be removed.

## Dependencies
None
