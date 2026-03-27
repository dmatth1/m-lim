# Task 430: Add AudioProcessor::reset() and processBlockBypassed() Overrides

## Description
`PluginProcessor` is missing two important `juce::AudioProcessor` overrides:

**1. `reset()`** — JUCE DAWs call this when playback stops or the transport is repositioned.
Without it, stale state persists across playback sessions:
- `TransientLimiter` lookahead ring buffers contain old PCM samples
- `LevelingLimiter` gain/envelope state is at its last level
- `DCFilter` IIR filter state persists
- `TruePeakDetector` FIR circular buffer state persists

**2. `processBlockBypassed()`** — JUCE's default is a no-op (zero delay). Because M-LIM
reports non-zero latency via `setLatencySamples()` (lookahead + oversampler delay), the
host compensates other tracks. When the host calls `processBlockBypassed`, M-LIM's signal
has zero latency while compensated tracks are pre-delayed, causing audible mis-alignment.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/PluginProcessor.h` — add `void reset() override;` and `void processBlockBypassed(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;`
Modify: `src/PluginProcessor.cpp` — implement both overrides
Modify: `src/dsp/LimiterEngine.h` — add `void reset();` public method
Modify: `src/dsp/LimiterEngine.cpp` — implement engine reset (clear lookahead, envelopes, filters)
Read: `src/dsp/TransientLimiter.h` — delay buffer layout for zeroing
Read: `src/dsp/LevelingLimiter.h` — gain/envelope state fields for zeroing

## Acceptance Criteria
- [ ] Run: `grep -n "void reset" src/PluginProcessor.h` → Expected: line containing `void reset() override;`
- [ ] Run: `grep -n "processBlockBypassed" src/PluginProcessor.h` → Expected: override declaration present
- [ ] Run: `grep -n "processBlockBypassed" src/PluginProcessor.cpp` → Expected: implementation present
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds, exit 0

## Tests
None

## Technical Details
**reset() implementation** (safe approach — reuse prepare):
```cpp
void PluginProcessor::reset()
{
    if (getSampleRate() > 0)
        prepareToPlay (getSampleRate(), getBlockSize());
}
```
Alternatively add a lightweight `LimiterEngine::reset()` that zeros state without
reallocating (faster for real-time hosts that call reset frequently):
- `DCFilter::reset()` already exists — just call it
- `TruePeakDetector::reset()` already exists — call for all 4 instances
- `TransientLimiter::mDelayBuffers`: clear via `buffer.clear()`
- `LevelingLimiter::mGainState`, `mEnvState`: zero-fill vectors

**processBlockBypassed() implementation** (simplest correct approach):
```cpp
void PluginProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& midi)
{
    // Delegate to processBlock with engine bypass on so lookahead delay is maintained
    const bool wasBypass = limiterEngine.isBypass();
    limiterEngine.setBypass (true);
    processBlock (buffer, midi);
    limiterEngine.setBypass (wasBypass);
}
```
If `LimiterEngine` has no `isBypass()` accessor, maintain a separate `mHostBypassed`
flag in `PluginProcessor` and check it in `processBlock`.

Files use `src/` paths relative to the `M-LIM/` directory. Build from workspace root:
`cmake --build build --target MLIM_Standalone_Standalone -j$(nproc)`.

## Dependencies
None
