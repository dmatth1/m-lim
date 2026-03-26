# Task 242: LimiterEngine Silence Passthrough With Oversampling Active

## Description
`test_limiter_engine.cpp` has `test_silence_produces_no_gain_reduction` which only tests with the default oversampling factor (0 = 1x, no oversampling). There is no test verifying that silence in → silence out holds when oversampling is enabled (2x, 4x, 8x).

This matters because:
- The oversampling chain (upsample → process → downsample) could introduce DC bias or ringing artifacts that produce non-zero output on silence
- The JUCE `dsp::Oversampling` filter initialization could leave residual state after `prepare()` that bleeds into the first few blocks

Add tests that set the oversampling factor to 2x/4x/8x and verify that silence in produces silence (or near-silence) out after the settling transient.

## Produces
None

## Consumes
None

## Relevant Files
Read: `tests/dsp/test_limiter_engine.cpp` — existing test at line ~300 to understand the pattern
Read: `src/dsp/LimiterEngine.h` — `setOversamplingFactor()` and `prepare()` API
Modify: `tests/dsp/test_limiter_engine.cpp` — add the new test cases

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ./tests/MLIMTests "[LimiterEngine]" --reporter compact` → Expected: all tests pass including new silence+oversampling tests

## Tests
- Unit: `tests/dsp/test_limiter_engine.cpp::test_silence_with_2x_oversampling` — prepare engine with factor=1 (2x), process 50 blocks of silence, verify output peak < 1e-5 after warmup
- Unit: `tests/dsp/test_limiter_engine.cpp::test_silence_with_4x_oversampling` — same with factor=2 (4x)
- Unit: `tests/dsp/test_limiter_engine.cpp::test_silence_with_8x_oversampling` — same with factor=3 (8x)

## Technical Details
- Use `kSampleRate = 44100.0`, `kBlockSize = 512`, `kNumChannels = 2`
- Call `engine.setOversamplingFactor(factor)` then `engine.prepare(kSampleRate, kBlockSize, kNumChannels)` to avoid deferred flag
- Process 10 warm-up blocks (to settle filter state), then 40 measured blocks — all must have peak output < 1e-5
- Use `engine.process(buffer)` where buffer is `juce::AudioBuffer<float>` cleared to zero
- After processing, check both channels for absolute peak value
- The GR meter data from FIFO is not required — just check audio output

## Dependencies
None
