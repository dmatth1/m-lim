# Task 017: Plugin Processor Integration

## Description
Wire up the PluginProcessor to use LimiterEngine for audio processing, read parameters from APVTS, populate MeterData FIFO, handle state save/load, and report correct latency to the host.

## Produces
Implements: `PluginProcessorCore`

## Consumes
LimiterEngineInterface
ParameterLayout
ABStateInterface
PresetManagerInterface
UndoManagerInterface

## Relevant Files
Modify: `M-LIM/src/PluginProcessor.h` — add LimiterEngine, ABState, PresetManager, UndoManager members
Modify: `M-LIM/src/PluginProcessor.cpp` — implement processBlock, prepareToPlay, state save/load
Read: `M-LIM/src/dsp/LimiterEngine.h` — DSP engine interface
Read: `M-LIM/src/Parameters.h` — parameter IDs
Read: `M-LIM/src/state/ABState.h` — A/B state
Read: `M-LIM/src/state/PresetManager.h` — preset system
Create: `M-LIM/tests/integration/test_plugin_processor.cpp` — integration tests

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIMTests -j$(nproc) && cd build && ctest -R test_plugin_processor --output-on-failure` → Expected: all tests pass
- [ ] Run: `cd M-LIM && cmake --build build --target MLIM_VST3 -j$(nproc) 2>&1 | tail -3` → Expected: builds successfully

## Tests
- Integration: `tests/integration/test_plugin_processor.cpp::test_process_block_no_crash` — process 1000 blocks without crash
- Integration: `tests/integration/test_plugin_processor.cpp::test_output_within_ceiling` — output never exceeds ceiling parameter
- Integration: `tests/integration/test_plugin_processor.cpp::test_state_save_load` — save state, create new processor, load state, verify params match
- Integration: `tests/integration/test_plugin_processor.cpp::test_latency_reported` — getLatencyInSamples() > 0 when lookahead > 0

## Technical Details
- In prepareToPlay: call limiterEngine.prepare(), set initial parameter values from APVTS
- In processBlock: read all parameter values from APVTS atomics, update limiterEngine setters, call limiterEngine.process(), push MeterData to FIFO
- Parameter reading: use `apvts.getRawParameterValue("paramId")->load()` for real-time safe access
- State save/load: serialize APVTS state tree to/from MemoryBlock using XML
- Latency: call setLatencySamples(limiterEngine.getLatencySamples()) in prepareToPlay and whenever lookahead OR oversampling changes (see tasks 033, 034)
- IMPORTANT: Do NOT call limiterEngine.setOversamplingFactor() directly from processBlock — oversampling factor changes require memory allocation. Use a deferred mechanism (see task 033)
- NOTE: LoudnessMeter wiring is handled separately in task 040
- Bypass: when bypass param is true, pass audio through but still meter

## Dependencies
Requires tasks 013, 014, 015, 016
