# Task 141: LimiterEngine and PluginProcessor — No Tests for Mono (1-Channel) Processing

## Description
`LimiterEngine::prepare()` accepts any `numChannels` value, and the DSP chain is designed
to support both mono and stereo. However, every existing test uses stereo (2 channels):

- All `test_limiter_engine.cpp` tests call `engine.prepare(kSampleRate, kBlockSize, 2)`.
- All `test_processor_stress.cpp` tests use `juce::AudioBuffer<float>(kNumChannels=2, ...)`.
- `test_plugin_processor.cpp` uses 2 channels throughout.

Bugs this would catch:
1. The sidechain buffer allocation (`mSidechainBuffer`) might not resize correctly for
   `numChannels=1`, causing an out-of-bounds write during `process()`.
2. `mUpPtrs` / `mSidePtrs` working arrays might be sized for 2 channels even when only 1 is
   provided, leaving a dangling second pointer that gets dereferenced.
3. The TruePeakDetector for the right channel (`mTruePeakR`) is called unconditionally
   even for mono — it should be skipped or fed the same signal.
4. The `TransientLimiter::process()` channel-linking logic may divide by 0 or access
   `mGainState[1]` when only 1 channel is prepared.

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/dsp/LimiterEngine.h` — prepare() signature, numChannels handling
Read: `M-LIM/src/dsp/LimiterEngine.cpp` — how numChannels propagates through DSP chain
Read: `M-LIM/src/dsp/TransientLimiter.h` — per-channel state vectors
Modify: `M-LIM/tests/dsp/test_limiter_engine.cpp` — add mono-specific test cases

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R LimiterEngine --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_limiter_engine.cpp::test_mono_no_clip` — prepare with
  `numChannels=1`, feed a loud mono sine (amplitude 5.0) for 20 blocks, verify output is finite
  and peak ≤ ceiling + margin on all blocks after block 5.
- Unit: `tests/dsp/test_limiter_engine.cpp::test_mono_silence_passes` — prepare with
  `numChannels=1`, feed 10 blocks of silence, verify output is all-zero (no spurious GR or
  DC offset introduced).
- Unit: `tests/dsp/test_limiter_engine.cpp::test_mono_input_gain_applied` — set +6 dB input
  gain on a mono engine, feed a signal at -12 dBFS for 10 blocks; verify GR is active
  (getGainReduction() < -0.5 dB) due to the gain boost pushing the signal over the ceiling.

## Technical Details
- Use `juce::AudioBuffer<float>(1, blockSize)` for mono test buffers.
- The DSP components (TransientLimiter, LevelingLimiter, DCFilter, Dither) are all
  per-channel in their state, so a 1-channel prepare must allocate 1 state entry each.
- TruePeakDetector: the engine has separate mTruePeakL and mTruePeakR — for mono, only
  mTruePeakL should be used. The test verifies `getTruePeakL() >= 0` and no crash.

## Dependencies
None
