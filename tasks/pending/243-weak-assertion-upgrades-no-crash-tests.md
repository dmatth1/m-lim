# Task 243: Upgrade Weak "No Crash" Assertions to Value-Checking Assertions

## Description
Several tests in the suite check only that code "doesn't crash" without verifying output correctness. These tests will not catch bugs that produce incorrect (but non-crashing) output. Identified weak patterns:

1. **`tests/integration/test_plugin_processor.cpp::test_process_block_no_crash`** — processes 1000 blocks but ends with `REQUIRE(true)`. Should add: output is finite, output is not all-zeros when input is non-zero, and peak level is within expected range.

2. **`tests/integration/test_dsp_components.cpp::test_all_dsp_headers_compile`** — ends with a comment "If we reach here, all DSP headers compiled without conflicts." The assertion is implicit (compilation). Fine as-is, but the `test_all_dsp_prepare` test (line 52) should additionally verify each component's state after prepare (e.g., latency > 0 for TransientLimiter with lookahead).

3. **`tests/dsp/test_dsp_edge_cases.cpp::test_very_high_sample_rate`** — checks `isfinite` but not that output is actually meaningful (e.g., DCFilter at 192kHz should still attenuate DC).

4. **`tests/dsp/test_limiter_algorithm.cpp::test_all_algorithms_have_params`** — checks that `getAlgorithmParams()` returns without crash, but uses `REQUIRE(true)` after the call.

For each location, upgrade the assertion from "no crash" to "output is correct":
- `test_process_block_no_crash`: add `CHECK(allFinite(buffer))` and `CHECK(peakLevel(buffer) > 0.0f)` after each processed block
- `test_all_algorithms_have_params`: assert that `params.kneeWidth >= 0.0f && params.kneeWidth <= 12.0f` and all fields are finite
- `test_very_high_sample_rate::DCFilter`: add a DC removal correctness check (similar to `test_removes_dc_offset`)

## Produces
None

## Consumes
None

## Relevant Files
Modify: `tests/integration/test_plugin_processor.cpp` — upgrade `test_process_block_no_crash`
Modify: `tests/dsp/test_limiter_algorithm.cpp` — upgrade `test_all_algorithms_have_params`
Modify: `tests/dsp/test_dsp_edge_cases.cpp` — upgrade `test_very_high_sample_rate` DCFilter section
Read: `tests/integration/test_plugin_processor.cpp` — existing `peakLevel()` and `allFinite()` helpers to reuse

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ./tests/MLIMTests "[PluginProcessor]" "[LimiterAlgorithm]" "[DSPEdgeCases]" --reporter compact` → Expected: all tests pass with no failures

## Tests
No new test cases — this task upgrades existing ones. The upgraded tests must use meaningful `CHECK` or `REQUIRE` calls that would fail if the component produced wrong output.

## Technical Details
- In `test_process_block_no_crash`: after the loop, check the last buffer state. A helper `allFinite(buf)` may already exist in the file — reuse it. Also check `peakLevel(buffer) > 0.001f` (the limiter should not silence a 0.5 amplitude sine).
- In `test_all_algorithms_have_params`: replace `REQUIRE(true)` with field-level checks: `REQUIRE(std::isfinite(params.kneeWidth))`, `REQUIRE(params.kneeWidth >= 0.0f)`, `REQUIRE(params.saturationAmount >= 0.0f && params.saturationAmount <= 1.0f)`.
- In `test_very_high_sample_rate::DCFilter section`: after processing 64 samples of 0.5f DC at 192kHz, measure the last few samples and verify they are near zero (DC removed). Tolerance should be loose (< 0.1f) since only 64 samples may not fully settle at this rate.

## Dependencies
None
