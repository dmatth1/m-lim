# Task 120: Sidechain Data Path Tests for TransientLimiter and LevelingLimiter

## Description
Both `TransientLimiter::process()` and `LevelingLimiter::process()` accept an optional
`sidechainData` parameter that reroutes peak detection to a separate signal while gain
reduction is applied to the main audio. This code path has **zero test coverage** — all
existing tests pass `nullptr` for sidechainData.

Two concrete scenarios must be tested:
1. **Sidechain detection vs. main audio detection**: Feed a loud signal on the sidechain
   only (main audio is quiet). The limiter should apply GR based on the sidechain peak,
   attenuating the quiet main audio — which it would NOT do if using the main audio for
   detection.
2. **Quiet sidechain, loud main audio**: Feed silence on the sidechain but a loud signal
   on the main audio. The limiter should NOT apply significant GR because it is detecting
   on the (silent) sidechain, allowing the loud main audio to pass through substantially
   unattenuated.

These tests must be added to the existing test files for each component.

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/dsp/TransientLimiter.h` — sidechain parameter of process()
Read: `src/dsp/LevelingLimiter.h` — sidechain parameter of process()
Modify: `tests/dsp/test_transient_limiter.cpp` — add sidechain tests
Modify: `tests/dsp/test_leveling_limiter.cpp` — add sidechain tests

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R "TransientLimiter|LevelingLimiter" --output-on-failure` → Expected: all tests pass, including new sidechain tests
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure 2>&1 | grep -E "passed|failed"` → Expected: no regressions, zero failures

## Tests
- Unit: `tests/dsp/test_transient_limiter.cpp::test_sidechain_drives_gr_not_main` — loud sidechain + quiet main → GR applied to quiet main
- Unit: `tests/dsp/test_transient_limiter.cpp::test_sidechain_silent_no_gr` — silent sidechain + loud main → minimal GR on main
- Unit: `tests/dsp/test_leveling_limiter.cpp::test_sidechain_drives_gr_not_main` — same as above but for LevelingLimiter
- Unit: `tests/dsp/test_leveling_limiter.cpp::test_sidechain_silent_no_gr` — same as above but for LevelingLimiter

## Technical Details
For TransientLimiter: `process(float** channelData, int numChannels, int numSamples, const float* const* sidechainData)`.
When `sidechainData != nullptr`, the peak-detection deque is fed from `sidechainData` while the
delay buffer still holds `channelData`. The gain is computed from the sidechain peak and applied
to the main audio.

For LevelingLimiter: same pattern — envelope follower runs on sidechainData, gain applied to channelData.

Setup pattern:
```cpp
// loud sidechain, quiet main
float sc[kBlockSize]; std::fill(sc, sc + kBlockSize, 2.0f);   // 6 dB over threshold
float main[kBlockSize]; std::fill(main, main + kBlockSize, 0.5f);
float* channelPtrs[1]  = { main };
const float* scPtrs[1] = { sc };
limiter.process(channelPtrs, 1, kBlockSize, scPtrs);
// main should be attenuated even though it was quiet
REQUIRE(blockPeak(main) < 0.5f);
```

## Dependencies
None
