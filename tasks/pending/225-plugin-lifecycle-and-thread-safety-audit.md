# Task 225: Plugin Lifecycle and Thread Safety Audit

## Description
Audit the plugin for correctness issues that would cause crashes or misbehavior in a real DAW environment. Focus on:

1. **processBlock thread safety**: Verify no heap allocations, no mutex locks, no std::vector push_back, no juce::String construction, no Logger calls inside `processBlock`. Use `grep` to audit.
2. **prepareToPlay re-entrancy**: Verify `prepareToPlay` can be called multiple times with different sample rates and block sizes without memory leaks or state corruption. All DSP components must be re-initialized cleanly.
3. **releaseResources cleanup**: Verify `releaseResources` tears down everything that `prepareToPlay` set up.
4. **Plugin state round-trip**: `getStateInformation` → `setStateInformation` must restore identical parameter values. Write a simple standalone test or use existing tests to verify.
5. **Null buffer handling**: `processBlock` must not crash if called with 0 samples (some DAWs do this).
6. **Single-channel (mono) processing**: `isBusesLayoutSupported` must return true for mono layouts. Verify `processBlock` handles `buffer.getNumChannels() == 1` without out-of-bounds access.
7. **CLAP ID uniqueness**: The CLAP ID `com.mlimaudio.mlim` in CMakeLists.txt should be a proper reverse-DNS format — verify it matches the company domain pattern.

Fix any issues found directly. If an issue is too large for this task, create a follow-up task.

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/PluginProcessor.cpp` — full audit
Read: `src/PluginProcessor.h` — interface
Read: `src/dsp/LimiterEngine.cpp` — processBlock path
Read: `src/dsp/LimiterEngine.h` — check prepare/reset API
Read: `CMakeLists.txt` — CLAP_ID verification
Read: `tests/integration/` — existing lifecycle tests

## Acceptance Criteria
- [ ] Run: `grep -n "new \|malloc\|std::vector.*push_back\|juce::String\b" /workspace/M-LIM/src/PluginProcessor.cpp` → Expected: no matches inside processBlock body (outside of prepareToPlay)
- [ ] Run: `grep -n "new \|malloc\|std::vector.*push_back" /workspace/M-LIM/src/dsp/LimiterEngine.cpp` → Expected: no matches inside process() method
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure 2>&1 | tail -10` → Expected: all tests pass
- [ ] Run: `grep "isBusesLayoutSupported" /workspace/M-LIM/src/PluginProcessor.cpp` → Expected: implementation present that handles mono

## Tests
None

## Technical Details
- Real-time safe = no heap alloc, no locks, no blocking I/O in processBlock
- JUCE's AudioBuffer::getNumChannels() returns actual channel count, always check before index access
- juce::AudioProcessorValueTreeState parameters are thread-safe for reads (atomic)

## Dependencies
Requires task 224
