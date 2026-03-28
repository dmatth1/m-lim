# Task 533: DspUtil.h Unit Tests

## Description
`src/dsp/DspUtil.h` contains 5 inline utility functions (`gainToDecibels`, `decibelsToGain`, `clampThreshold`, `floatBitsEqual`, `applyChannelLinking`) used across all DSP modules. None have dedicated unit tests — they're only exercised indirectly through LimiterEngine and limiter tests. Direct tests are needed to verify edge cases (0 dB roundtrip, -120 dB floor, NaN/Inf inputs, single-channel linking, link=0 vs link=1 boundaries) that indirect tests don't cover.

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/dsp/DspUtil.h` — all 5 functions under test
Create: `tests/dsp/test_dsp_util.cpp` — new unit test file
Modify: `tests/CMakeLists.txt` — add test_dsp_util.cpp to MLIM_TEST_CASES

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "DspUtil" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_dsp_util.cpp::test_gainToDecibels_zero_dB_roundtrip` — gainToDecibels(1.0f) == 0.0f
- Unit: `tests/dsp/test_dsp_util.cpp::test_gainToDecibels_minus120_floor` — gainToDecibels(0.0f) returns -120 dB (clamped to kDspUtilMinGain)
- Unit: `tests/dsp/test_dsp_util.cpp::test_gainToDecibels_negative_input` — negative linear gain handled same as positive (log10 of abs? or clamped?)
- Unit: `tests/dsp/test_dsp_util.cpp::test_decibelsToGain_roundtrip` — decibelsToGain(gainToDecibels(x)) ≈ x for several values
- Unit: `tests/dsp/test_dsp_util.cpp::test_decibelsToGain_minus_infinity` — very low dB value produces near-zero gain
- Unit: `tests/dsp/test_dsp_util.cpp::test_clampThreshold_below_min` — values below 1e-6f clamp to kDspUtilMinGain
- Unit: `tests/dsp/test_dsp_util.cpp::test_clampThreshold_above_max` — values > 1.0f clamp to 1.0f
- Unit: `tests/dsp/test_dsp_util.cpp::test_clampThreshold_in_range` — values in [1e-6, 1.0] pass through unchanged
- Unit: `tests/dsp/test_dsp_util.cpp::test_floatBitsEqual_same_value` — returns true for identical bit patterns
- Unit: `tests/dsp/test_dsp_util.cpp::test_floatBitsEqual_positive_negative_zero` — +0.0f and -0.0f have different bits, should return false
- Unit: `tests/dsp/test_dsp_util.cpp::test_floatBitsEqual_nan` — NaN != NaN in float, but memcmp should match same NaN bit pattern
- Unit: `tests/dsp/test_dsp_util.cpp::test_applyChannelLinking_link_zero_no_change` — link=0 leaves per-channel gains unchanged
- Unit: `tests/dsp/test_dsp_util.cpp::test_applyChannelLinking_link_one_all_min` — link=1 sets all channels to minimum
- Unit: `tests/dsp/test_dsp_util.cpp::test_applyChannelLinking_single_channel_no_change` — chCount=1 is a no-op regardless of link value
- Unit: `tests/dsp/test_dsp_util.cpp::test_applyChannelLinking_partial_blend` — link=0.5 blends 50% toward min

## Technical Details
Include `DspUtil.h` directly. No JUCE dependency needed for this test file — these are pure math functions. Use Catch2 `Approx()` for float comparisons.

## Dependencies
None
