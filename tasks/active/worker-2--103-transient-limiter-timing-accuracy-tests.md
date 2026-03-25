# Task 103: TransientLimiter Gain Reduction Timing and Lookahead Accuracy Tests

## Description
`test_transient_limiter.cpp` verifies that limiting happens but does not verify *when* it happens relative to the peak, nor does it verify that the reported latency matches the actual gain reduction onset. Missing coverage:

- Lookahead latency matches `getLatencyInSamples()`: gain reduction must begin no later than `lookahead_samples` before the peak sample arrives in the output
- Zero lookahead produces zero reported latency
- Reported latency changes when lookahead parameter changes (after `prepare()`)
- With a null sidechain pointer, processing must not crash (defensive use case)
- A block size smaller than the lookahead buffer (e.g. 16 samples with 20 ms lookahead at 44100 = 882 samples) must not crash
- Channel count mismatch (buffer has fewer channels than prepared) must not cause out-of-bounds write

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/dsp/TransientLimiter.h` — getLatencyInSamples(), prepare(), process() signatures
Read: `src/dsp/TransientLimiter.cpp` — lookahead implementation
Modify: `tests/dsp/test_transient_limiter.cpp` — add new test cases

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R TransientLimiter --output-on-failure` → Expected: all tests pass, exit 0
- [ ] Run: `grep -c "TEST_CASE" /workspace/M-LIM/tests/dsp/test_transient_limiter.cpp` → Expected: at least 12 test cases

## Tests
- Unit: `tests/dsp/test_transient_limiter.cpp::test_zero_lookahead_zero_latency` — with lookahead=0, `getLatencyInSamples()` returns 0
- Unit: `tests/dsp/test_transient_limiter.cpp::test_lookahead_latency_matches_reported` — with lookahead=5 ms at 44100 Hz (220 samples), latency is approximately 220 samples (±5 samples)
- Unit: `tests/dsp/test_transient_limiter.cpp::test_latency_changes_with_lookahead_param` — change lookahead from 5 ms to 10 ms via AlgorithmParams, reprepare, latency doubles
- Unit: `tests/dsp/test_transient_limiter.cpp::test_null_sidechain_no_crash` — pass nullptr as sidechain buffer, process 10 blocks → no crash, output finite
- Unit: `tests/dsp/test_transient_limiter.cpp::test_small_block_no_crash` — blockSize=16 with 20 ms lookahead at 44100 Hz → 100 blocks → no crash, output finite

## Technical Details
- Check latency by inspecting `getLatencyInSamples()` before and after parameter changes
- For the lookahead accuracy test: insert a single impulse (one sample at full scale) in silence, then verify gain reduction appears before or at that sample in the output
- AlgorithmParams lookahead is in milliseconds; convert to samples: `int(lookahead_ms * 0.001 * sampleRate)`

## Dependencies
None
