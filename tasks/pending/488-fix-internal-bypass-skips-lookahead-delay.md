# Task 488: Fix Internal Bypass Skips Lookahead Delay

## Description
When the plugin's internal bypass button is engaged (`pBypass` parameter = true),
`LimiterEngine::process()` returns early at line 221–225:

```cpp
if (mBypass.load())
{
    pushBypassMeterData(buffer, inLevelL, inLevelR, numChannels, numSamples);
    return;  // audio exits WITHOUT passing through lookahead delay buffer
}
```

The audio bypasses ALL processing including the lookahead delay (`TransientLimiter`'s
circular delay buffer). However, `setLatencySamples(limiterEngine.getLatencySamples())`
is **not updated** when bypass is toggled — the host continues to apply delay
compensation as if the plugin still has its full lookahead + oversampler latency.

**Effect in a DAW:** With lookahead = 2 ms at 44100 Hz (≈88 samples), the host offsets
other tracks by 88 samples to compensate for M-LIM's delay. When bypass is engaged,
M-LIM outputs audio with 0 actual delay. The bypassed track is now 88 samples ahead
of where the host expects it — audible as a timing shift in the mix.

The standard pro-audio fix is to pass audio through the lookahead delay buffer even in
bypass mode (the "transparent bypass" approach). Audio passes through the delay but
no gain reduction is applied. This keeps the actual output latency constant and matching
the reported value.

An alternative fix — calling `setLatencySamples(0)` when bypass engages — is NOT
recommended: it forces the host to re-compensate and can cause pops/discontinuities.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/dsp/LimiterEngine.cpp` — remove early return in bypass branch; instead,
  run input through TransientLimiter's delay path (with gain reduction skipped),
  downsample, apply ceiling, and output the correctly-delayed but uncompressed audio
Modify: `src/dsp/TransientLimiter.h` — add a `bool processDelayOnly(...)` or modify
  `process()` to accept a bypass flag that runs the delay buffer but always outputs
  gain=1.0 (no reduction applied)
Modify: `src/dsp/TransientLimiter.cpp` — implement the delay-only path
Read: `src/dsp/LimiterEngine.h` — mBypass atomic flag, process() structure
Read: `src/PluginProcessor.cpp` — updateLatency(), pushAllParametersToEngine()

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "bypass|Bypass" --output-on-failure` → Expected: all tests pass
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds
- [ ] Test: with lookahead > 0, engage bypass, feed an impulse at sample 0, verify the
  impulse appears at sample `getLatencySamples()` in the output (not at sample 0)

## Tests
- Unit: `tests/dsp/test_limiter_engine.cpp::test_bypass_delays_audio_by_reported_latency` —
  set lookahead = 2 ms, prepareToPlay at 44100 Hz, engage bypass, feed impulse buffer,
  verify output impulse position == getLatencySamples(), not 0
- Unit: `tests/dsp/test_limiter_engine.cpp::test_bypass_applies_no_gain_reduction` —
  with bypass=true, feed a sine wave at 0 dBFS, verify output is identical in amplitude
  to input (delayed by latency but no GR applied)
- Integration: `tests/integration/test_plugin_processor.cpp::test_internal_bypass_maintains_host_latency` —
  toggle plugin bypass button, verify setLatencySamples value does NOT change

## Technical Details
Two implementation approaches:

**Option A (Recommended) — Delay-only path in LimiterEngine::process()**:
Replace the early return with:
```cpp
if (mBypass.load())
{
    // Pass through lookahead delay to maintain reported latency, but apply no GR.
    stepRunSidechainFilter(buffer, numChannels, numSamples); // not strictly needed
    juce::dsp::AudioBlock<float> upBlock, upSideBlock;
    stepUpsample(buffer, upBlock, upSideBlock);
    // Run TransientLimiter in passthrough mode (advances delay buffer, gain=1.0)
    mTransientLimiter.processBypassDelay(mUpPtrs.data(), upChannels, upSamples);
    mOversampler.downsample(buffer);
    pushBypassMeterData(buffer, inLevelL, inLevelR, numChannels, numSamples);
    return;
}
```

**Option B — Transparent bypass with gain parameter**:
Add `bool bypassGainReduction` flag to `TransientLimiter::process()`. When true,
the delay buffer advances and audio is written back from the delay at unity gain.

Either approach must ensure `getLatencySamples()` returns the same value whether
bypass is on or off.

## Dependencies
None
