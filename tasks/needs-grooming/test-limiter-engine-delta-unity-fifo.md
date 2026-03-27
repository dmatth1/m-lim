# Task: LimiterEngine — Delta Mode, Unity Gain, and Meter FIFO Overflow Tests

## Description
Three untested behaviors in LimiterEngine:

1. **Delta mode output correctness**: When `setDeltaMode(true)`, output should be `(input * inputGain) - (limited output)` — i.e., "what was removed". No test verifies that the delta output actually equals the difference, and that adding delta + limited output reconstructs the original (within floating-point precision).

2. **Unity gain correctness**: `setUnityGain(true)` sets ceiling = -inputGain so that a 0 dBFS input produces a 0 dBFS output. No test verifies this mathematically — specifically that a +6 dB input gain with unity gain enabled produces the correct ceiling (-6 dBFS), not an arbitrary value.

3. **Meter FIFO behavior when consumer is slow**: The `LockFreeFIFO<MeterData>` has a fixed capacity. If the UI thread never pops, eventually the FIFO fills. No test verifies the FIFO handles this gracefully (drops oldest or skips push without corrupting audio processing).

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/dsp/LimiterEngine.h` — `setDeltaMode()`, `setUnityGain()`, FIFO field
Read: `M-LIM/src/dsp/LimiterEngine.cpp` — delta mode output calculation, FIFO push logic
Read: `M-LIM/src/dsp/MeterData.h` — MeterData struct definition
Read: `M-LIM/tests/dsp/test_limiter_engine.cpp` — existing engine tests
Modify: `M-LIM/tests/dsp/test_limiter_engine.cpp` — add new tests

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "LimiterEngine" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_limiter_engine.cpp::test_delta_mode_reconstructs_original` — Enable delta mode. Feed a known sine. Capture delta output. Capture a second run without delta mode (limited output). Sum them and compare to the input after input gain. Require reconstruction error < -120 dBFS (near floating-point precision).
- Unit: `tests/dsp/test_limiter_engine.cpp::test_unity_gain_ceiling_tracks_input_gain` — Set inputGain = +6 dB and enable unity gain. Drive engine with 0 dBFS input. Verify output peak is within [-0.1, +0.1] dBFS (ceiling applied correctly as -6 dBFS).
- Unit: `tests/dsp/test_limiter_engine.cpp::test_unity_gain_ceiling_tracks_negative_input_gain` — Set inputGain = -6 dB and enable unity gain. Verify output peak is near 0 dBFS (ceiling = +6 dBFS, but hard clip still at 0 dBFS — verify no sample exceeds 1.0).
- Unit: `tests/dsp/test_limiter_engine.cpp::test_meter_fifo_full_no_audio_corruption` — Process 10000 blocks without ever reading from the meter FIFO (simulate slow UI). Verify audio output is still bit-accurate and engine does not crash or block.

## Technical Details
- For delta reconstruction test: process the same input buffer twice with identical state (or use a deterministic signal and two separate engine instances). This avoids state contamination between runs.
- FIFO capacity: check `MeterData.h` or `LimiterEngine.cpp` for the FIFO size constant before writing the overflow test (need to process enough blocks to guarantee fill).

## Dependencies
None
