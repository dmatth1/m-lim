# Task 032: Wire LoudnessMeter into Processing Chain

## Description
The LoudnessMeter (Task 010) is currently disconnected from the processing pipeline. Neither LimiterEngine (Task 013) nor PluginProcessor (Task 017) consume or call it. The loudness meter must be called with post-limiter audio data so the LoudnessPanel UI can display LUFS values. Wire LoudnessMeter into LimiterEngine or PluginProcessor and expose readings via MeterData.

## Produces
None

## Consumes
LimiterEngineInterface
LoudnessMeterInterface
MeterDataInterface

## Relevant Files
Modify: `M-LIM/src/dsp/LimiterEngine.h` — add LoudnessMeter member, add LUFS getters
Modify: `M-LIM/src/dsp/LimiterEngine.cpp` — call loudnessMeter.processBlock() after limiting, populate MeterData with LUFS values
Modify: `M-LIM/src/dsp/MeterData.h` — add momentaryLUFS, shortTermLUFS, integratedLUFS, loudnessRange fields to MeterData struct
Read: `M-LIM/src/dsp/LoudnessMeter.h` — LoudnessMeter interface

## Acceptance Criteria
- [ ] Run: `cd M-LIM && grep "loudnessMeter" src/dsp/LimiterEngine.cpp | head -3` → Expected: LoudnessMeter is called in the process chain
- [ ] Run: `cd M-LIM && grep "LUFS\|lufs" src/dsp/MeterData.h` → Expected: LUFS fields present in MeterData struct

## Tests
- Unit: `tests/dsp/test_limiter_engine.cpp::test_lufs_meter_populated` — after processing audio, getMomentaryLUFS returns a finite value (not -inf for non-silent input)

## Technical Details
- LoudnessMeter should process the OUTPUT audio (post-limiting, post-dither)
- Add to MeterData: `float momentaryLUFS`, `float shortTermLUFS`, `float integratedLUFS`, `float loudnessRange`
- LimiterEngine.prepare() must also call loudnessMeter.prepare()
- This is a wiring task — the LoudnessMeter implementation itself is in Task 010

## Dependencies
Requires tasks 010, 013
