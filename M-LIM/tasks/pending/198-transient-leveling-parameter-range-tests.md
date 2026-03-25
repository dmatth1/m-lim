# Task 198: TransientLimiter and LevelingLimiter — parameter boundary and out-of-range tests

## Description
`test_transient_limiter.cpp` and `test_leveling_limiter.cpp` only exercise typical in-range
parameter values. Both classes document valid ranges in their headers but have no tests verifying
clamp/no-crash behavior at the boundaries or beyond.

Add tests for:

### TransientLimiter
1. **Extreme channel link values** — call `setChannelLink(0.0f)` and `setChannelLink(1.0f)`,
   process a loud stereo signal, and verify: (a) both extremes produce finite output, (b) at
   link=1.0 both channels have identical gain reduction applied.
2. **Zero-length lookahead** — if `setLookahead(0.0f)` is supported, verify it does not crash
   and that the limiter still holds peaks below the ceiling.
3. **Rapid threshold changes mid-block** — alternate `setThreshold(0.5f)` and `setThreshold(1.0f)`
   on every block for 20 blocks; output must always be finite.

### LevelingLimiter
1. **Attack time = 0 ms** — `setAttack(0.0f)` then process a step-function signal (0→2.0f in one
   sample); verify the first sample of output does not exceed 1.0f (ceiling) by more than 0.01f.
2. **Release time at extremes** — `setRelease(10.0f)` (min) and `setRelease(1000.0f)` (max);
   process a loud burst then silence; verify the release envelope converges (output approaches 1.0f)
   within 2× the configured release time.
3. **Threshold at 1.0f and at very low value** — `setThreshold(1.0f)` passes signal through
   unattenuated for sub-unity signals; `setThreshold(0.01f)` severely limits a 0.5f constant signal.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `tests/dsp/test_transient_limiter.cpp` — add TransientLimiter boundary TEST_CASEs
Modify: `tests/dsp/test_leveling_limiter.cpp` — add LevelingLimiter boundary TEST_CASEs
Read: `src/dsp/TransientLimiter.h` — setLookahead(), setChannelLink(), setThreshold(), process()
Read: `src/dsp/LevelingLimiter.h` — setAttack(), setRelease(), setThreshold(), setChannelLink()

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R "TransientLimiter" --output-on-failure` → Expected: all transient limiter tests pass, exit 0
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R "LevelingLimiter" --output-on-failure` → Expected: all leveling limiter tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_transient_limiter.cpp::test_channel_link_extremes` — link=0 and link=1 produce finite output; link=1 yields equal GR on both channels
- Unit: `tests/dsp/test_transient_limiter.cpp::test_zero_lookahead_no_crash` — setLookahead(0) does not crash; output peak ≤ ceiling
- Unit: `tests/dsp/test_transient_limiter.cpp::test_rapid_threshold_change` — 20-block rapid threshold alternation produces all-finite output
- Unit: `tests/dsp/test_leveling_limiter.cpp::test_instant_attack` — 0ms attack limits step-function first-sample to ≤ 1.01f
- Unit: `tests/dsp/test_leveling_limiter.cpp::test_release_boundary_values` — 10ms and 1000ms release both converge; gain measured at 2× release time is > 0.9× unity
- Unit: `tests/dsp/test_leveling_limiter.cpp::test_threshold_extremes` — setThreshold(1.0f) passes 0.5f signal unchanged; setThreshold(0.01f) reduces 0.5f constant to ≤ 0.011f

## Technical Details
- For the channel-link=1.0 test: use a stereo buffer where channel 0 has amplitude 1.5f and channel
  1 has amplitude 0.3f. After processing, the ratio of gain reductions applied to each channel
  should be equal (|gainCh0 - gainCh1| < 0.01f). Compare input-to-output ratio on each channel.
- For the release convergence test: feed 100 blocks of loud signal, then 100 blocks of silence,
  and check that `blockPeak()` of the output in the silence phase grows toward 1.0f.
- Re-use the existing `makePtrs()` and `blockPeak()` helpers already in each test file.
- Use `setAlgorithmParams(getAlgorithmParams(LimiterAlgorithm::Transparent))` for a clean baseline.

## Dependencies
None
