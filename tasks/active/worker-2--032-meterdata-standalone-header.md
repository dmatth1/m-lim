# Task 032: Extract MeterData to Standalone Header

## Description
SPEC.md originally placed MeterData in PluginProcessor.h, while Task 013 places it in LimiterEngine.h. Both create a coupling problem: UI components (WaveformDisplay, LevelMeter, etc.) would need to include a DSP or processor header just for a data struct. Extract MeterData and LockFreeFIFO into a standalone `src/dsp/MeterData.h` header that can be included by both DSP and UI code without pulling in heavy dependencies.

## Produces
None

## Consumes
MeterDataInterface

## Relevant Files
Create: `M-LIM/src/dsp/MeterData.h` — MeterData struct + LockFreeFIFO template
Modify: `M-LIM/src/dsp/LimiterEngine.h` — include MeterData.h instead of defining MeterData inline
Modify: `M-LIM/src/PluginProcessor.h` — include MeterData.h, use LockFreeFIFO<MeterData> from it
Read: `SPEC.md` — MeterDataInterface (now references MeterData.h)

## Acceptance Criteria
- [ ] Run: `ls M-LIM/src/dsp/MeterData.h` → Expected: file exists
- [ ] Run: `cd M-LIM && grep "struct MeterData" src/dsp/MeterData.h` → Expected: MeterData defined in standalone header
- [ ] Run: `cd M-LIM && cmake --build build --target MLIM_VST3 -j$(nproc) 2>&1 | tail -3` → Expected: builds successfully

## Tests
None (header reorganization — verified by successful compilation)

## Technical Details
- MeterData.h should be a lightweight header: only `<array>`, `<atomic>`, `<cstdint>` includes
- LockFreeFIFO<T> is a single-producer single-consumer ring buffer — can use `std::atomic<int>` for read/write indices
- No JUCE includes needed in MeterData.h — keeps it decoupled
- This resolves the implicit dependency where WaveformDisplay (Task 022) needs MeterData but only depends on Task 003

## Dependencies
Requires task 013
