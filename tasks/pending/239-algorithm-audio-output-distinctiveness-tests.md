# Task 239: Algorithm Audio Output Distinctiveness Tests

## Description
`tests/dsp/test_limiter_algorithm.cpp` has a test named `test_algorithm_enum_count` whose comment says "Verify all 8 algorithms are distinct" but it only checks integer enum ordinals — not actual audio output. There is no test that applies each of the 8 algorithms to a `TransientLimiter` + `LevelingLimiter` pair and verifies that each algorithm produces measurably different gain reduction behavior on the same input signal.

This gap means a developer could accidentally use the same `AlgorithmParams` struct for two different algorithms (e.g., by duplicating a `switch` case) and no test would catch it.

Add tests that:
1. Process identical loud audio through a `TransientLimiter` configured with each algorithm's params.
2. Record the peak gain reduction or output peak for each algorithm.
3. Assert that no two algorithms produce exactly the same output peak (within a small tolerance), confirming they are actually distinct in DSP behavior.
4. Verify that `Aggressive` produces more GR than `Safe` (behavioral ordering test).
5. Verify that `Transparent` (zero saturation) and `Aggressive` (heavy saturation) produce different waveform shapes (RMS differs from peak differently).

## Produces
None

## Consumes
None

## Relevant Files
Read: `tests/dsp/test_limiter_algorithm.cpp` — existing tests to extend
Read: `src/dsp/LimiterAlgorithm.h` — the 8 algorithm parameter sets
Read: `src/dsp/TransientLimiter.h` — to apply algorithm params and measure GR
Read: `src/dsp/LevelingLimiter.h` — second stage for full behavioral difference
Modify: `tests/dsp/test_limiter_algorithm.cpp` — add the new test cases

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ./tests/MLIMTests "[LimiterAlgorithm]" --reporter compact` → Expected: all tests pass including the new distinctiveness tests, no failures

## Tests
- Unit: `tests/dsp/test_limiter_algorithm.cpp::test_all_algorithms_produce_distinct_gr` — process loud audio with each of the 8 algorithms, record GR; verify all 8 GR values are distinct (no duplicates within 0.01 dB)
- Unit: `tests/dsp/test_limiter_algorithm.cpp::test_aggressive_more_gr_than_safe` — Aggressive must produce more gain reduction than Safe on a +6 dBFS input
- Unit: `tests/dsp/test_limiter_algorithm.cpp::test_saturation_affects_waveform_shape` — Transparent (sat=0) vs Aggressive (sat=0.8): for same loud input, peak-to-RMS ratio should differ by measurable amount (saturation compresses peaks)

## Technical Details
- Use `kSampleRate = 44100.0`, `kBlockSize = 1024` to match existing test constants
- Input: constant amplitude of 2.0f (+6 dBFS) for enough blocks to reach steady state (at least 3 blocks)
- Apply `limiter.setAlgorithmParams(getAlgorithmParams(algo))` before each measurement
- For GR measurement, call `limiter.process(ptrs.data(), kNumChannels, kBlockSize)` then compare input vs output peak
- Distinct tolerance: two algorithms are considered distinct if their output peaks differ by > 0.001 linear (approximately 0.008 dB)

## Dependencies
None
