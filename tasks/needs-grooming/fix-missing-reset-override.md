# Task: Add AudioProcessor::reset() override to flush DSP state

## Description
`PluginProcessor` does not override `juce::AudioProcessor::reset()`. JUCE DAWs call
`reset()` when playback is stopped, the transport is repositioned, or the project is
reloaded. Without overriding it, the following internal state retains stale audio from
the previous playback session:

- `TransientLimiter` lookahead ring buffers (`mDelayBuffers`) contain old PCM samples
- `LevelingLimiter` gain/envelope state (`mGainState`, `mEnvState`) is at its last level
- `DCFilter` first-order IIR filter state persists
- `TruePeakDetector` FIR circular buffer state persists (4 instances)

When playback restarts after a stop, stale lookahead content can produce an audible
click or a brief gain-pump artifact at the very first few milliseconds.

## Fix
1. Add `void reset() override;` to `PluginProcessor.h`.
2. In `PluginProcessor.cpp`, implement it by calling `prepareToPlay` with the current
   sample rate and block size so all DSP components are reinitialised to a clean state.
   Guard against the case where `getSampleRate() == 0` (before first prepare).

Alternatively (lower overhead): add a dedicated `LimiterEngine::reset()` public method
that zeros all state buffers without reallocating, and call it from the `reset()` override.
The engine already calls `reset()` on `TruePeakDetector` and `DCFilter` instances in
`stepEnforceTruePeak`; a new method can do so comprehensively plus clear the lookahead
delay buffers and envelope state.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/PluginProcessor.h` — add `void reset() override;`
Modify: `M-LIM/src/PluginProcessor.cpp` — implement `reset()` to re-prepare or call engine reset
Modify: `M-LIM/src/dsp/LimiterEngine.h` — add `void reset();` public method
Modify: `M-LIM/src/dsp/LimiterEngine.cpp` — implement engine reset (clear lookahead, envelopes, filters)
Read:   `M-LIM/src/dsp/TransientLimiter.h` — understand delay buffer layout for zeroing
Read:   `M-LIM/src/dsp/LevelingLimiter.h` — understand gain/envelope state for zeroing

## Acceptance Criteria
- [ ] Run: `grep -n "void reset" M-LIM/src/PluginProcessor.h` → Expected: line containing `void reset() override;`
- [ ] Run: `grep -n "void reset" M-LIM/src/dsp/LimiterEngine.h` → Expected: line containing `void reset();`
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc)` → Expected: exits 0, no errors

## Tests
None

## Technical Details
- `juce::AudioProcessor::reset()` is declared `virtual` in JUCE. The default impl is empty.
- Safe impl: `if (getSampleRate() > 0) prepareToPlay(getSampleRate(), getBlockSize());`
- `TransientLimiter` delay buffers: `mDelayBuffers` (vector of `juce::AudioBuffer<float>`), zero via `clearActiveBufferRegion()` or `clear()`
- `LevelingLimiter` state: `mGainState` and `mEnvState` (vectors of float), zero fill
- `DCFilter::reset()` already exists (line 22 DCFilter.h) — just call it
- `TruePeakDetector::reset()` already exists (line 40 TruePeakDetector.h) — just call it for all 4 instances

## Dependencies
None
