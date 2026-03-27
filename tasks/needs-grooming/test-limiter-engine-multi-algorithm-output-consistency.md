# Task: LimiterEngine — All 8 Algorithms End-to-End Output Consistency Tests

## Description
`test_all_algorithms_produce_distinct_gr` (in test_limiter_algorithm.cpp) tests algorithms
at the TransientLimiter level only. There is **no end-to-end test** that processes the same
signal through the full LimiterEngine with each of the 8 algorithms and verifies:
1. Each algorithm produces valid output (no NaN/Inf, all samples finite)
2. Each algorithm produces non-zero gain reduction on a hot signal
3. No two algorithms produce identical output (they should be distinct)
4. All algorithms respect the output ceiling (no sample exceeds ceiling)

This is an integration-level smoke test that catches regressions in the algorithm→limiter
wiring.

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/dsp/LimiterAlgorithm.h` — algorithm enum (8 values)
Read: `src/dsp/LimiterEngine.h` — setAlgorithm()
Modify: `tests/dsp/test_limiter_engine.cpp` — add per-algorithm integration test

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "LimiterEngine" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_limiter_engine.cpp::test_all_algorithms_valid_output_through_engine` — for each of 8 algorithms: prepare engine, set algorithm, process 20 blocks of +6 dBFS sine; all output samples finite, max output <= ceiling + 0.01, GR < -0.5 dB
- Unit: `tests/dsp/test_limiter_engine.cpp::test_all_algorithms_distinct_output_through_engine` — compare RMS of output across algorithms; at least 6 of 8 should have distinct RMS (within 0.01 dB tolerance)

## Technical Details
Use `LimiterAlgorithm::Transparent` through `LimiterAlgorithm::BussPunch` (or whatever
the 8 enum values are — verify from LimiterAlgorithm.h). Process a deterministic sine
burst at +6 dBFS so all algorithms must apply significant limiting.

## Dependencies
None
