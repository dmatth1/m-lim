# Task 038: Real-Time Safe Oversampling Factor Change

## Description
Changing the oversampling factor requires destroying and recreating the `juce::dsp::Oversampling` object, which allocates memory. If `processBlock` reads the `oversamplingFactor` parameter and immediately calls `setOversamplingFactor()` on LimiterEngine, this allocation happens on the audio thread — violating real-time safety. Implement a deferred parameter change mechanism: detect the change on the audio thread, defer the actual reallocation to prepareToPlay or a message-thread callback, and update latency reporting afterward.

## Produces
None

## Consumes
OversamplerInterface
LimiterEngineInterface
PluginProcessorCore

## Relevant Files
Modify: `M-LIM/src/dsp/Oversampler.h` — add `bool needsRebuild()` / `void commitRebuild()` or similar deferred API
Modify: `M-LIM/src/dsp/Oversampler.cpp` — implement deferred rebuild
Modify: `M-LIM/src/dsp/LimiterEngine.h` — add deferred oversampling change support
Modify: `M-LIM/src/dsp/LimiterEngine.cpp` — detect pending oversampling change, skip oversampling until rebuilt
Modify: `M-LIM/src/PluginProcessor.cpp` — use AsyncUpdater or similar to trigger rebuild from message thread
Read: `M-LIM/src/dsp/Oversampler.h` — current interface

## Acceptance Criteria
- [ ] Run: `cd M-LIM && grep -n "AsyncUpdater\|triggerAsyncUpdate\|handleAsyncUpdate\|needsRebuild\|pendingFactor\|deferredOversamplingChange" src/PluginProcessor.cpp src/dsp/Oversampler.h src/dsp/LimiterEngine.h | head -5` → Expected: evidence of deferred change mechanism
- [ ] Run: `cd M-LIM && cmake --build build --target MLIMTests -j$(nproc) && cd build && ctest -R test_oversampler --output-on-failure` → Expected: all tests pass

## Tests
- Unit: `tests/dsp/test_oversampler.cpp::test_deferred_factor_change` — calling setFactor during processing doesn't crash; rebuild happens on next prepare
- Integration: `tests/integration/test_plugin_processor.cpp::test_oversampling_change_no_audio_glitch` — changing oversamplingFactor parameter while processing doesn't cause assertions or crashes

## Technical Details
- Pattern: store desired factor in an atomic. In processBlock, if desired != current, either bypass oversampling (use 1x) or use the old factor until rebuild.
- Trigger `juce::AsyncUpdater::triggerAsyncUpdate()` from processBlock when factor changes. In `handleAsyncUpdate()` (message thread): suspend processing, rebuild oversampler, call `setLatencySamples()`, resume.
- Alternative simpler pattern: only change oversampling factor via `prepareToPlay` — the parameter change triggers a prepare cycle. Some hosts support this via `AudioProcessor::setLatencySamples()` triggering a re-prepare.
- Must also update latency reporting after the rebuild completes.

## Dependencies
Requires tasks 008, 013, 017
