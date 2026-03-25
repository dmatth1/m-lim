# Task 111: getTailLengthSeconds() Is Inconsistent With Reported Latency

## Description
`MLIMAudioProcessor::setLatencySamples()` reports:

```
latency = lookaheadSamples + oversamplerFilterLatencySamples
```

(via `LimiterEngine::getLatencySamples()` which sums both components).

But `getTailLengthSeconds()` only returns the lookahead component:

```cpp
double MLIMAudioProcessor::getTailLengthSeconds() const
{
    if (pLookahead != nullptr)
        return static_cast<double>(pLookahead->load()) * 0.001;
    return 0.0;
}
```

**What `getTailLengthSeconds()` means**: the duration the plugin continues to produce
meaningful output after input silence begins. For a lookahead limiter with an oversampler:

1. The lookahead delay buffer still contains audio after input stops → tail = lookaheadMs.
2. The oversampler's polyphase IIR filter has a group delay that is already compensated
   by the reported latency, but the filter's impulse response extends beyond that latency,
   so a few additional samples "ring out" — these are already included in
   `getLatencySamples()` via `mOversampler.getLatencySamples()`.

Therefore the tail should equal the total reported latency:
```cpp
double MLIMAudioProcessor::getTailLengthSeconds() const
{
    const double sr = getSampleRate();
    if (sr > 0.0)
        return static_cast<double>(getLatencySamples()) / sr;
    // Fallback before prepareToPlay (sr == 0): use lookahead only
    if (pLookahead != nullptr)
        return static_cast<double>(pLookahead->load()) * 0.001;
    return 0.0;
}
```

This ensures offline render tails and DAW export tails are correct for any oversampling
setting. Currently with 4× oversampling the reported latency is ~200+ samples but the
tail is only the lookahead time, leaving the oversampler ringout unaccounted for.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/PluginProcessor.cpp` — `getTailLengthSeconds()` (~lines 38–43)
Read:   `M-LIM/src/dsp/LimiterEngine.h`  — `getLatencySamples()` declaration
Read:   `M-LIM/src/PluginProcessor.h`    — `updateLatency()` helper; `getLatencySamples()` relationship

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM && cmake --build build -j$(nproc) 2>&1 | tail -5` → Expected: zero errors.
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure 2>&1 | tail -10` → Expected: all tests pass.
- [ ] Run: `grep -A 10 "getTailLengthSeconds" /workspace/M-LIM/src/PluginProcessor.cpp` → Expected: implementation references `getLatencySamples()` or `getSampleRate()`, not only `pLookahead`.

## Tests
- Integration: `tests/integration/test_plugin_processor.cpp` — after `prepareToPlay(44100, 512)`:
  - Set oversampling to 2× (factor=1).
  - `getTailLengthSeconds()` must be `>= getLatencySamples() / 44100.0 - 0.001` (within 1 ms tolerance).
  - `getTailLengthSeconds()` must equal `getLatencySamples() / getSampleRate()` exactly.

## Technical Details
- `getSampleRate()` is inherited from `AudioProcessor` and returns 0.0 before `prepareToPlay`.
  The fallback path (pLookahead-based) is safe for the pre-prepare case.
- `getLatencySamples()` is safe to call from the message thread (it reads `mLookaheadMs`
  via atomic and `mSampleRate` which is stable after prepare).
- The fix is a 3-line change; no DSP logic is altered.

## Dependencies
None
