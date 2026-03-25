# Task 136: Custom Threshold Enforcement Tests for TransientLimiter and LevelingLimiter

## Description
Both `TransientLimiter::setThreshold()` and `LevelingLimiter::setThreshold()` accept a linear
threshold (default 1.0 = 0 dBFS). In production, `LimiterEngine` calls these with the output
ceiling value (e.g., 0.891 for -1 dBFS). However, all existing unit tests for these components
use the **default threshold of 1.0** — there is **no test** that a custom threshold is respected.

If a bug caused `setThreshold()` to be ignored (e.g., the threshold stored in a local variable
that's never used in `computeRequiredGain()`), all current tests would still pass.

Add tests to `tests/dsp/test_transient_limiter.cpp` and `tests/dsp/test_leveling_limiter.cpp`
that verify:

1. **Output does not exceed a custom -1 dBFS ceiling**: Call `setThreshold(0.891f)` and feed
   a loud signal. All output samples must be ≤ 0.891f (+ tiny margin for filter rounding).

2. **Threshold at -6 dBFS is enforced**: Call `setThreshold(0.501f)` (-6 dBFS). Feed a signal
   at +6 dBFS (2.0f). Output must be ≤ 0.501f (not just ≤ 1.0f).

3. **Threshold change mid-session takes effect**: Feed a loud signal with threshold=1.0, then
   change threshold to 0.5f, then feed the same loud signal again. Output in the second phase
   must not exceed 0.5f + margin.

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/dsp/TransientLimiter.h` — setThreshold, computeRequiredGain
Read: `src/dsp/LevelingLimiter.h` — setThreshold, computeRequiredGain
Modify: `tests/dsp/test_transient_limiter.cpp` — add threshold tests
Modify: `tests/dsp/test_leveling_limiter.cpp` — add threshold tests

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R "TransientLimiter|LevelingLimiter" --output-on-failure` → Expected: all tests pass
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure 2>&1 | grep -E "passed|failed"` → Expected: no regressions

## Tests
- Unit: `tests/dsp/test_transient_limiter.cpp::test_custom_threshold_minus_1dBFS` — threshold=0.891 → output peak ≤ 0.891 + 1e-4
- Unit: `tests/dsp/test_transient_limiter.cpp::test_custom_threshold_minus_6dBFS` — threshold=0.501 → output peak ≤ 0.501 + 1e-4
- Unit: `tests/dsp/test_transient_limiter.cpp::test_threshold_change_mid_session` — change threshold during processing → new threshold enforced on subsequent blocks
- Unit: `tests/dsp/test_leveling_limiter.cpp::test_custom_threshold_minus_1dBFS` — same as above for LevelingLimiter
- Unit: `tests/dsp/test_leveling_limiter.cpp::test_custom_threshold_minus_6dBFS` — same as above for LevelingLimiter
- Unit: `tests/dsp/test_leveling_limiter.cpp::test_threshold_change_mid_session` — change threshold mid-session → new threshold enforced

## Technical Details
```cpp
TEST_CASE("test_custom_threshold_minus_6dBFS", "[TransientLimiter]")
{
    TransientLimiter limiter;
    limiter.prepare(44100.0, 512, 2);
    limiter.setAlgorithmParams(getAlgorithmParams(LimiterAlgorithm::Transparent));
    limiter.setLookahead(1.0f);
    limiter.setThreshold(0.501f);  // -6 dBFS

    // Feed loud input for several blocks to engage GR
    for (int block = 0; block < 10; ++block)
    {
        std::vector<std::vector<float>> buf(2, std::vector<float>(512, 2.0f));
        auto ptrs = makePtrs(buf);
        limiter.process(ptrs.data(), 2, 512);
        if (block > 2)  // skip warm-up
            REQUIRE(blockPeak(buf[0]) <= 0.501f + 1e-4f);
    }
}
```

Note: `LevelingLimiter` uses a slow envelope follower, so testing threshold enforcement
requires more warm-up blocks (30+) to fully engage gain reduction.

## Dependencies
None
