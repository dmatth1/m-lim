# Task 146: LimiterEngine — Delta Mode Has No Energy Conservation Test

## Description
`LimiterEngine` delta mode is documented as: "output = (input after gain) - (limited output)".
This means:
```
delta_output + limited_output ≈ gain_adjusted_input
```
No test currently verifies this conservation property. The existing tests only check that
the delta output is "non-zero for loud input" and "near-silence for quiet input" — neither
test bounds the accuracy or direction of the difference.

Without an energy conservation test, the following bugs are undetected:
1. Delta is computed as `limited - input` (reversed sign) — the difference would be negative
   when gain reduction occurs, but the test only checks for `rms > 0`.
2. Delta is scaled by some factor (e.g., applied twice, or after the output ceiling hard-clip
   rather than before) — the sum wouldn't equal the gain-adjusted input.
3. Delta is calculated from the wrong lookahead position — the delta and the limited signal
   are temporally misaligned by the lookahead, so the sum would cancel imperfectly.

Note: exact sample-perfect conservation is not expected because the lookahead delay
introduces a timing offset. The test should verify approximate RMS conservation, not
sample-by-sample identity.

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/dsp/LimiterEngine.h` — setDeltaMode(), mPreLimitBuffer usage
Read: `M-LIM/src/dsp/LimiterEngine.cpp` — how delta = preLimitBuffer - processedBuffer
Modify: `M-LIM/tests/dsp/test_limiter_engine_modes.cpp` — add conservation test

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R LimiterEngineModes --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_limiter_engine_modes.cpp::test_delta_rms_bounded_by_input` — with
  delta mode enabled, process 20 blocks of a +6 dBFS signal. The RMS of the delta output
  must be strictly less than the RMS of the gain-adjusted input (delta ≤ input), and the
  delta RMS must be > 0 (something was removed). This validates the delta represents a
  fraction of the input, not an amplified signal.
- Unit: `tests/dsp/test_limiter_engine_modes.cpp::test_delta_sign_is_positive_for_loud` —
  process 10 warm-up blocks, then capture one block of delta output. With a positive sine
  input and positive gain reduction, the *mean absolute* value of the delta output should be
  positive (not negative), confirming the direction of the difference is correct
  (removed signal, not added signal).

## Technical Details
- Instantiate two LimiterEngine instances with identical settings. Run one in delta mode
  and one in normal mode on the same input. The delta output RMS + normal output RMS
  should sum to approximately the pre-limit (gain-adjusted) input RMS within ±20%.
  The ±20% tolerance accounts for the lookahead-induced temporal misalignment.
- The mPreLimitBuffer pre-allocates the gain-adjusted input snapshot. The delta is
  computed by subtracting the processed output from this snapshot.

## Dependencies
None
