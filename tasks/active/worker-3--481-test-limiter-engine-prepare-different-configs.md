# Task 481: LimiterEngine — Re-prepare With Different Configurations Test

## Description
`LimiterEngine::prepare()` can be called multiple times with different sample rates, block
sizes, and channel counts (as happens when a DAW changes its audio settings). There is no
test that verifies the engine works correctly after being re-prepared with a significantly
different configuration (e.g. 44100→96000 Hz, 512→128 block size, stereo→mono).

A bug here would manifest as buffer overflows, incorrect timing constants, or stale state
from the previous configuration.

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/dsp/LimiterEngine.cpp` — prepare() implementation
Modify: `tests/dsp/test_limiter_engine.cpp` — add re-prepare tests

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "LimiterEngine" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_limiter_engine.cpp::test_reprepare_44100_to_96000_valid_output` — prepare at 44100/512/2, process, re-prepare at 96000/256/2, process 10 blocks; output finite, no crash
- Unit: `tests/dsp/test_limiter_engine.cpp::test_reprepare_stereo_to_mono_valid_output` — prepare at 44100/512/2, process, re-prepare at 44100/512/1, process 10 blocks of mono; no crash, output finite
- Unit: `tests/dsp/test_limiter_engine.cpp::test_reprepare_large_to_small_block_no_overflow` — prepare at 44100/2048/2, process, re-prepare at 44100/64/2, process 10 blocks; no crash

## Technical Details
After re-prepare, verify: output is finite, no samples exceed ceiling, GR is reasonable
(not stuck at a previous state). The stereo→mono transition is particularly important
because it changes buffer/pointer sizing throughout the chain.

## Dependencies
None
