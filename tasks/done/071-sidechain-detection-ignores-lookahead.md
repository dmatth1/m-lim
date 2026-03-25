# Task 071: TransientLimiter — Sidechain Detection Ignores Lookahead Window

## Description
In `TransientLimiter::process()`, when sidechain data is provided and lookahead is enabled, the code scans the delay buffer for peaks (lines 204-212) but then immediately overrides the result with just the current sidechain sample (lines 216-219):

```cpp
if (sidechainData != nullptr)
{
    peakAbs = std::abs(sidechainData[ch][s]);
}
```

This defeats the purpose of lookahead for the sidechain path. The lookahead buffer exists so the limiter can "see ahead" and apply gain reduction before a peak arrives. When using sidechain detection, the limiter should scan the SIDECHAIN signal's lookahead window, not just the current sidechain sample.

Without this fix, sidechain-filtered limiting has no lookahead anticipation and will react late to transients detected via the sidechain — producing overshoot that the lookahead was designed to prevent.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/TransientLimiter.h` — add sidechain delay buffers
Modify: `M-LIM/src/dsp/TransientLimiter.cpp` — maintain separate delay buffer for sidechain detection, scan sidechain lookahead for peaks

## Acceptance Criteria
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass
- [ ] Run: Verify by code review that when sidechainData is provided, the lookahead window scan uses sidechain samples, not main audio samples

## Tests
- Unit: `tests/dsp/test_transient_limiter.cpp::test_sidechain_with_lookahead` — provide main audio and different sidechain data with a peak arriving in the lookahead future; verify gain reduction begins before the peak arrives in the main signal

## Technical Details
Add parallel sidechain delay buffers in TransientLimiter:

```cpp
// In header:
std::vector<std::vector<float>> mSidechainDelayBuffers;

// In prepare():
mSidechainDelayBuffers.assign(numChannels, std::vector<float>(mMaxLookaheadSamples + 1, 0.0f));

// In process(), when sidechainData is provided:
// 1. Write sidechain sample into mSidechainDelayBuffers
// 2. Scan mSidechainDelayBuffers for peak (instead of mDelayBuffers or raw sidechain sample)
// 3. Use the sidechain peak for computeRequiredGain()
```

Also clean up the dead code at lines 196-200 (the unused `buf` variable with the `(void)buf` suppression).

## Dependencies
None
