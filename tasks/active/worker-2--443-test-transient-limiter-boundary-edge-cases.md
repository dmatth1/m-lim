# Task 443: TransientLimiter — Lookahead Boundary and Multi-Prepare Edge Cases

## Description
Missing TransientLimiter tests for boundary conditions:
1. Lookahead buffer wrap-around at exact capacity (N-1, N, N+1 samples)
2. Multiple prepare() calls — second prepare clears state, no artifact peak
3. Zero-length block is a no-op
4. Mono sidechain with stereo main must not crash

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/dsp/TransientLimiter.h` — lookahead buffer, prepare() signature
Read: `src/dsp/TransientLimiter.cpp` — ring buffer implementation
Read: `tests/dsp/test_transient_limiter.cpp` — existing tests
Modify: `tests/dsp/test_transient_limiter.cpp` — add new tests

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "TransientLimiter" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_transient_limiter.cpp::test_lookahead_wrap_around_exact_capacity` — feed maxLookahead then maxLookahead+1 samples with a peak; peak appears at correct delay offset
- Unit: `tests/dsp/test_transient_limiter.cpp::test_reprepare_clears_state` — prepare() twice, feed impulse after second; only one output peak
- Unit: `tests/dsp/test_transient_limiter.cpp::test_zero_length_block_noop` — process(buffer, 0) then normal signal; output identical to no-zero-block case
- Unit: `tests/dsp/test_transient_limiter.cpp::test_sidechain_mono_main_stereo_no_crash` — mono sidechain + stereo main; must not crash

## Dependencies
None
