# Task 033: Extract MeterData and LockFreeFIFO to Shared Header

## Description
Task 013 places `MeterData` struct and `LockFreeFIFO<T>` template in `LimiterEngine.h`. These types are shared between the audio thread (producer) and the UI thread (consumer) — they're used by both `PluginProcessor` and `PluginEditor`. Coupling them to LimiterEngine creates an unnecessary dependency: the editor shouldn't need to include LimiterEngine.h just to read meter data.

Extract these into a dedicated `src/MeterData.h` (or `src/dsp/MeterData.h`) so they can be included independently.

## Produces
None

## Consumes
None

## Relevant Files
Create: `M-LIM/src/MeterData.h` — MeterData struct + LockFreeFIFO template
Modify: `M-LIM/src/dsp/LimiterEngine.h` — remove MeterData/LockFreeFIFO definitions, include shared header instead
Read: `SPEC.md` — MeterDataInterface definition

## Acceptance Criteria
- [ ] Run: `grep -l "MeterData" M-LIM/src/MeterData.h` → Expected: file exists with MeterData definition
- [ ] Run: `grep "MeterData" M-LIM/src/dsp/LimiterEngine.h | head -3` → Expected: includes MeterData.h rather than defining MeterData inline
- [ ] Run: `cd M-LIM && cmake --build build --target MLIM_VST3 -j$(nproc) 2>&1 | tail -3` → Expected: builds successfully

## Tests
None (refactoring, no new logic)

## Technical Details
- `MeterData` struct fields per SPEC: inputLevelL/R, outputLevelL/R, gainReduction, truePeakL/R, waveformBuffer (array<float,512>), waveformSize
- `LockFreeFIFO<T>`: single-producer single-consumer lock-free ring buffer, uses atomics for read/write indices
- Both types are plain data + lock-free primitives — no JUCE dependency needed beyond juce_core for atomics
- This should be done during or right after task 013, before task 017 and 027 wire the FIFO between processor and editor

## Dependencies
Requires task 013
