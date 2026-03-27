# Task 444: LimiterEngine — Delta Mode, Unity Gain, and Meter FIFO Overflow Tests

## Description
Three untested LimiterEngine behaviors:
1. Delta mode: output = input - limited; delta + limited reconstructs original (< -120 dBFS error)
2. Unity gain: ceiling tracks input gain correctly
3. Meter FIFO full when UI is slow — audio output not corrupted

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/dsp/LimiterEngine.h` — setDeltaMode(), setUnityGain(), FIFO field
Read: `src/dsp/LimiterEngine.cpp` — delta output calculation, FIFO push logic
Read: `src/dsp/MeterData.h` — MeterData struct, FIFO capacity
Read: `tests/dsp/test_limiter_engine.cpp` — existing engine tests
Modify: `tests/dsp/test_limiter_engine.cpp` — add new tests

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "LimiterEngine" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_limiter_engine.cpp::test_delta_mode_reconstructs_original` — delta + limited output reconstructs input within -120 dBFS
- Unit: `tests/dsp/test_limiter_engine.cpp::test_unity_gain_ceiling_tracks_input_gain` — inputGain=+6dB + unity gain; output peak within [-0.1, +0.1] dBFS
- Unit: `tests/dsp/test_limiter_engine.cpp::test_unity_gain_ceiling_tracks_negative_input_gain` — inputGain=-6dB + unity gain; no sample exceeds 1.0
- Unit: `tests/dsp/test_limiter_engine.cpp::test_meter_fifo_full_no_audio_corruption` — 10000 blocks, never read FIFO; audio output bit-accurate, no crash

## Dependencies
None
