# Task 013: Limiter Engine (DSP Orchestrator)

## Description
Implement the top-level LimiterEngine that orchestrates the full DSP chain: input gain → sidechain filter → oversampling up → transient limiter → leveling limiter → oversampling down → DC filter → dither → output ceiling. Also produces MeterData for the UI.

## Produces
Implements: `LimiterEngineInterface`
Implements: `MeterDataInterface`

## Consumes
TransientLimiterInterface
LevelingLimiterInterface
OversamplerInterface
TruePeakDetectorInterface
SidechainFilterInterface
DCFilterInterface
DitherInterface

## Relevant Files
Create: `M-LIM/src/dsp/MeterData.h` — MeterData struct + LockFreeFIFO (standalone header, see SPEC.md)
Create: `M-LIM/src/dsp/LimiterEngine.h` — class declaration, includes MeterData.h
Create: `M-LIM/src/dsp/LimiterEngine.cpp` — implementation
Create: `M-LIM/tests/dsp/test_limiter_engine.cpp` — unit tests
Read: `M-LIM/src/dsp/TransientLimiter.h` — Stage 1 interface
Read: `M-LIM/src/dsp/LevelingLimiter.h` — Stage 2 interface
Read: `M-LIM/src/dsp/Oversampler.h` — oversampling interface
Read: `M-LIM/src/dsp/TruePeakDetector.h` — true peak interface
Read: `SPEC.md` — LimiterEngineInterface, MeterDataInterface

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIMTests -j$(nproc) && cd build && ctest -R test_limiter_engine --output-on-failure` → Expected: all tests pass

## Tests
- Unit: `tests/dsp/test_limiter_engine.cpp::test_full_chain_no_clip` — loud input should produce output that never exceeds ceiling
- Unit: `tests/dsp/test_limiter_engine.cpp::test_input_gain_applied` — +6dB input gain should increase level by 6dB before limiting
- Unit: `tests/dsp/test_limiter_engine.cpp::test_output_ceiling` — output should not exceed configured ceiling
- Unit: `tests/dsp/test_limiter_engine.cpp::test_algorithm_switch` — changing algorithm mid-stream should not crash or produce artifacts
- Unit: `tests/dsp/test_limiter_engine.cpp::test_latency_reporting` — getLatencySamples matches lookahead + oversampler latency
- Unit: `tests/dsp/test_limiter_engine.cpp::test_meter_data_populated` — after processing, getGainReduction and getTruePeak return valid values

## Technical Details
- DSP chain order: inputGain → sidechainFilter (detection copy) → oversampler.upsample → transientLimiter → levelingLimiter → oversampler.downsample → dcFilter → dither → outputCeiling
- The sidechain filter shapes the DETECTION signal, not the audio path (process a copy for detection, apply GR to original)
- MeterData struct captures snapshots: input/output levels, GR, true peak, waveform buffer
- LockFreeFIFO: single-producer (audio thread) single-consumer (UI thread), fixed-size ring buffer
- All setter methods use atomics or are called from message thread before processing
- Total latency = lookahead samples + oversampler latency samples
- Delta mode: output = input - limited_output (to hear what the limiter removes)
- Unity gain mode: output ceiling automatically tracks input gain (ceiling = -inputGain)
- Bypass: pass audio through unchanged but still meter

## Dependencies
Requires tasks 005, 006, 007, 008, 009, 011, 012, 039
