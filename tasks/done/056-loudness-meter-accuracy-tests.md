# Task 056: Loudness Meter ITU-R BS.1770-4 Accuracy Tests

## Description
Task 010 specifies a 1kHz sine test with 1 LU tolerance — this is far too loose for ITU compliance (spec requires 0.1 LU). The loudness meter claims ITU-R BS.1770-4 compliance but lacks tests against the standard's reference signals. Add tests using known reference signals with tight tolerances to verify K-weighting filter accuracy and gated measurement correctness.

## Produces
None

## Consumes
LoudnessMeterInterface

## Relevant Files
Create: `M-LIM/tests/dsp/test_loudness_meter_accuracy.cpp` — ITU accuracy tests
Read: `M-LIM/src/dsp/LoudnessMeter.h` — LoudnessMeter interface

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIMTests -j$(nproc) && cd build && ctest -R test_loudness_meter_accuracy --output-on-failure` → Expected: all tests pass

## Tests
- Unit: `tests/dsp/test_loudness_meter_accuracy.cpp::test_1khz_reference_signal` — 1kHz stereo sine at -23dBFS should measure -23.0 LUFS ±0.1 LU (ITU reference)
- Unit: `tests/dsp/test_loudness_meter_accuracy.cpp::test_kweighting_frequency_response` — verify K-weighting: 1kHz at 0dB gain, 100Hz attenuated ~0.0dB, 50Hz attenuated, 10kHz boosted ~2.4dB (per ITU filter spec)
- Unit: `tests/dsp/test_loudness_meter_accuracy.cpp::test_momentary_window_400ms` — feed 400ms of signal then silence, verify momentary measurement reflects only the 400ms window
- Unit: `tests/dsp/test_loudness_meter_accuracy.cpp::test_shortterm_window_3s` — feed 3s of signal, verify short-term measurement stabilizes
- Unit: `tests/dsp/test_loudness_meter_accuracy.cpp::test_integrated_gating` — feed alternating loud/silent segments, verify integrated LUFS uses absolute gating (-70 LUFS) and relative gating (-10 LU below ungated)
- Unit: `tests/dsp/test_loudness_meter_accuracy.cpp::test_stereo_vs_dual_mono` — correlated stereo should measure same as dual mono at same level (within 0.1 LU)
- Unit: `tests/dsp/test_loudness_meter_accuracy.cpp::test_loudness_range` — feed signal with known dynamic range, verify LRA measurement within ±1 LU

## Technical Details
- Generate reference signals programmatically (sine waves at exact frequencies and levels)
- ITU-R BS.1770-4 K-weighting is a two-stage filter: pre-filter (high shelf +4dB at ~1.5kHz) + RLB weighting (high-pass at ~38Hz)
- For 400ms momentary test: at 48kHz, 400ms = 19200 samples. Feed exactly this many, verify measurement
- For integrated gating test: -70 LUFS absolute gate, then relative gate at -10 LU below ungated mean
- All tolerances should be ±0.1 LU for frequency response and ±0.5 LU for gating (gating has wider variance)

## Dependencies
Requires task 010
