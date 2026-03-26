# Task 239: LoudnessMeter LRA Missing EBU R128 Relative Gate

## Description

`LoudnessMeter::computeLRA()` only applies the absolute gate (−70 LUFS) when building
the histogram for loudness range. EBU R128 §3.3 (and EBU Tech 3342 §4.4) requires a
**second, relative gate** at −20 LU below the ungated mean of the absolute-gated blocks.
Blocks below this relative threshold must also be excluded from the 10th/95th-percentile
calculation. Without the relative gate the LRA value is wider than the standard mandates
because very-quiet-but-above-absolute-gate content inflates the lower tail of the
loudness distribution.

The integrated LUFS path already implements a correct two-pass gating algorithm (with a
−10 LU relative gate); the LRA path must mirror the same pattern but with −20 LU.

### Location
`src/dsp/LoudnessMeter.cpp` — `computeLRA()`, lines 327–389.

### Current Behaviour
```cpp
// Gate: above absolute threshold per EBU R128 §4.6 / ITU-R BS.1770-4 (-70 LUFS)
if (l > static_cast<float>(kAbsGateLUFS))
{
    int bin = ...;
    ++mLraHisto[bin]; ++validCount;
}
```
Only the absolute gate is applied; the relative gate is absent.

### Required Fix

Add a pre-pass over the 3-second windows to compute the ungated mean power of those
that pass the absolute gate, then derive the relative threshold as:
```
relGate_LUFS = powerToLUFS(ungatedMean) - 20.0f
```
Only windows passing **both** gates should be inserted into the histogram:
```cpp
if (l > absGate && l > relGate)
{
    ++mLraHisto[bin]; ++validCount;
}
```

A new `static constexpr double kLraRelGateOffset = -20.0;` constant should be added
alongside the existing `kRelGateOffset = -10.0`.

### Reference
EBU R128 §3.3; EBU Tech 3342 §4.4 (two-gate LRA algorithm).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/dsp/LoudnessMeter.cpp` — add relative gate in `computeLRA()`
Read: `src/dsp/LoudnessMeter.h` — `FixedRingBuffer`, `mPrefixSums`, `powerToLUFS`
Read: `tests/dsp/test_loudness_meter.cpp` — existing LRA tests to extend

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R test_loudness_meter --output-on-failure` → Expected: all existing tests pass
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R test_loudness_meter_accuracy --output-on-failure` → Expected: all tests pass
- [ ] Manual check: feed a sine at −23 LUFS for 10 s then silence for 10 s. LRA must be < 23 LU (the relative gate should suppress the silence blocks from widening the distribution)

## Tests
- Unit: `tests/dsp/test_loudness_meter.cpp` — add a test that validates LRA with a two-level signal (loud section + quiet section) and checks that the quiet section is excluded by the relative gate, resulting in LRA ≤ loudness difference between the two sections

## Technical Details
- The two-pass must use `mWindowPowers` and `mPrefixSums` (already pre-allocated), so no heap allocation is introduced on the audio thread. `computeLRA()` is called from `updateIntegratedAndLRA()` which runs on the audio thread.
- New constant: `static constexpr double kLraRelGateOffset = -20.0;`

## Dependencies
None
