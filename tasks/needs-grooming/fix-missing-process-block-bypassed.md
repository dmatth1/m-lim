# Task: Override processBlockBypassed() to maintain host latency compensation

## Description
`PluginProcessor` does not override `juce::AudioProcessor::processBlockBypassed()`.
JUCE's default implementation is a no-op — it passes audio through unchanged with zero
delay. Because M-LIM reports non-zero latency to the host via `setLatencySamples()`
(lookahead + oversampler delay, up to ~5 ms at 44.1 kHz), the host compensates by
pre-delaying tracks that are NOT routed through M-LIM. When the host invokes its own
DAW-level bypass (calling `processBlockBypassed` rather than `processBlock`), M-LIM's
signal suddenly has zero latency while everything else is still compensated, causing
audible mis-alignment (the bypassed signal arrives early by the plugin's latency).

## Fix
Override `processBlockBypassed()` in `PluginProcessor` to delay the audio by
`getLatencySamples()` samples using a simple FIFO delay line, so the bypass path
remains time-aligned with the host's compensation offset.

Implementation sketch:
- Maintain a `juce::AudioBuffer<float> mBypassDelayBuffer` allocated in `prepareToPlay`
  with size `(getLatencySamples() + maxBlockSize)` per channel.
- In `processBlockBypassed`, write the incoming block into the circular delay buffer and
  read out samples delayed by `getLatencySamples()`. When latency is 0 (no lookahead,
  no oversampling), the delay is a no-op.
- Reallocate and flush the delay buffer whenever `updateLatency()` changes the reported
  latency.

Alternatively (simpler): call `processBlock` directly from `processBlockBypassed`, since
`LimiterEngine` already has an internal bypass mode (`mBypass`) that passes audio through
while still buffering the lookahead delay. Set `mBypass = true` beforehand and the chain
keeps the correct delay automatically.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/PluginProcessor.h` — add `void processBlockBypassed(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;`
Modify: `M-LIM/src/PluginProcessor.cpp` — implement `processBlockBypassed` to maintain latency alignment
Read:   `M-LIM/src/dsp/LimiterEngine.h` — `setBypass(bool)` and `process()` for the simpler approach
Read:   `M-LIM/src/PluginProcessor.cpp` — `updateLatency()` is called when latency changes; delay buffer must be rebuilt there too

## Acceptance Criteria
- [ ] Run: `grep -n "processBlockBypassed" M-LIM/src/PluginProcessor.h` → Expected: line containing override declaration
- [ ] Run: `grep -n "processBlockBypassed" M-LIM/src/PluginProcessor.cpp` → Expected: implementation present
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc)` → Expected: exits 0, no errors

## Tests
None

## Technical Details
- The simpler approach (reuse `processBlock` with bypass flag) avoids duplicating delay
  logic. In that case `processBlockBypassed` calls `limiterEngine.setBypass(true)` then
  `processBlock`, then restores the actual bypass state. Alternatively if `mBypass`
  is already set as a parameter, just ensure the engine processes through with bypass on.
- The bypass delay buffer approach is more explicit but requires careful management of
  the write/read pointers when `getLatencySamples()` changes (oversampling factor change
  or lookahead change → `updateLatency()` → must flush and resize the delay buffer).
- JUCE's `AudioProcessor::processBlockBypassed` signature:
  `virtual void processBlockBypassed(juce::AudioBuffer<float>&, juce::MidiBuffer&)`

## Dependencies
None
