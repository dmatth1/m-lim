# Task 034: Integrate LoudnessMeter into Processor Chain

## Description
The LoudnessMeter (task 010) is implemented but never wired into the signal chain. Task 013 (LimiterEngine) doesn't list LoudnessMeterInterface in its Consumes, and task 017 (PluginProcessor) doesn't mention it either. The LoudnessPanel UI (task 023) expects loudness data but has no source.

The loudness meter should be called from the PluginProcessor (or LimiterEngine) after the limiter processes audio, measuring the output signal. The measured values need to reach the LoudnessPanel in the UI.

## Produces
None

## Consumes
LoudnessMeterInterface
LimiterEngineInterface

## Relevant Files
Modify: `M-LIM/src/PluginProcessor.cpp` — call loudnessMeter.processBlock() on output buffer in processBlock
Modify: `M-LIM/src/PluginProcessor.h` — add LoudnessMeter member
Modify: `M-LIM/src/MeterData.h` — add LUFS fields to MeterData struct (momentary, shortTerm, integrated, range)
Modify: `M-LIM/src/PluginEditor.cpp` — read LUFS data from MeterData and update LoudnessPanel
Read: `M-LIM/src/dsp/LoudnessMeter.h` — metering interface
Read: `M-LIM/src/ui/LoudnessPanel.h` — UI consumer

## Acceptance Criteria
- [ ] Run: `grep "loudnessMeter" M-LIM/src/PluginProcessor.cpp` → Expected: loudnessMeter.processBlock called in processBlock
- [ ] Run: `grep "momentaryLUFS\|shortTermLUFS\|integratedLUFS" M-LIM/src/MeterData.h` → Expected: LUFS fields present in MeterData
- [ ] Run: `cd M-LIM && cmake --build build --target MLIM_VST3 -j$(nproc) 2>&1 | tail -3` → Expected: builds successfully

## Tests
- Integration: `tests/integration/test_plugin_processor.cpp::test_loudness_metering_active` — process sine wave blocks, verify getLoudnessMeter values are non-zero

## Technical Details
- LoudnessMeter should measure the OUTPUT of the limiter chain (post-limiting signal)
- Add to MeterData: float momentaryLUFS, shortTermLUFS, integratedLUFS, loudnessRange
- In processBlock: after limiterEngine.process(), call loudnessMeter.processBlock(buffer)
- In MeterData push: populate LUFS fields from loudnessMeter getters
- In editor timerCallback: read LUFS from MeterData, call loudnessPanel.setMomentary/setShortTerm/setIntegrated/setLoudnessRange
- LoudnessMeter must be prepared in prepareToPlay alongside the limiter engine

## Dependencies
Requires tasks 010, 017
