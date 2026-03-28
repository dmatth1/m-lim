# Task: LimiterEngine — Channel Count Mismatch Handling Tests

## Description
`LimiterEngine::prepare(sampleRate, maxBlockSize, numChannels)` allocates internal buffers
for a specific channel count. If `process(buffer)` is then called with a buffer that has
a *different* channel count than what was prepared, the behavior is undefined — internal
vectors are sized for `numChannels` channels but the buffer may have more or fewer.

This scenario is realistic: a DAW plugin can present varying channel configurations at
different times (e.g., mono → stereo switch in host's I/O settings). The current test
suite has no test for:

1. prepare(2 channels) then process with 1-channel buffer
2. prepare(1 channel) then process with 2-channel buffer
3. prepare(2 channels) then process with 0-channel buffer

These are important because internal `std::vector<float*>` pointers, sidechain arrays,
and the channel loop bounds are all sized at prepare() time and used without bounds
checking in the hot path.

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/dsp/LimiterEngine.cpp` — prepare(), process(), channel loop bounds
Read: `src/dsp/LimiterEngine.h` — public API
Read: `tests/dsp/test_limiter_engine.cpp` — existing tests
Modify: `tests/dsp/test_limiter_engine.cpp` — add channel mismatch tests

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "LimiterEngine" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_limiter_engine.cpp::test_process_fewer_channels_than_prepared_no_crash` — prepare(44100, 512, 2), then process a mono (1-channel) buffer; must not crash, output must be finite
- Unit: `tests/dsp/test_limiter_engine.cpp::test_process_zero_channels_no_crash` — prepare(44100, 512, 2), then process a 0-channel buffer; must not crash (loop should not execute)
- Unit: `tests/dsp/test_limiter_engine.cpp::test_process_more_channels_than_prepared_bounded` — prepare(44100, 512, 1), then process a stereo (2-channel) buffer; document whether it processes only channel 0, both, or silently clips — test must at minimum verify no crash and finite output on channel 0

## Technical Details
The LimiterEngine processes `buffer.getNumChannels()` or its prepared channel count —
read the source to determine which governs the loop. If the engine uses
`min(buffer.getNumChannels(), preparedChannels)`, that is acceptable behavior — just
verify and document it. If it uses only the buffer's channel count without bounds checking,
that could cause an out-of-bounds read on the prepared arrays.

## Dependencies
None
