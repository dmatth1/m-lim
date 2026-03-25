# Task 144: SidechainFilter — Missing Test for Combined HP + LP Pass-Band

## Description
`SidechainFilter` supports high-pass, low-pass, and tilt EQ simultaneously. Existing tests
cover each filter in isolation:

- `test_highpass_attenuates_bass` — HP alone at 100 Hz attenuates 50 Hz.
- `test_lowpass_attenuates_treble` — LP alone at 10 kHz attenuates 15 kHz.
- `test_tilt_attenuates_high_frequencies` — tilt EQ alone.
- `test_passthrough_when_disabled` — all filters bypassed.

No test verifies that when HP and LP are active simultaneously with a reasonable band-pass
window (e.g., HP at 80 Hz, LP at 8 kHz), the filter:
1. Passes mid-band content (e.g., 1 kHz) without significant attenuation (< 1 dB).
2. Attenuates content below the HP cutoff (50 Hz → > 6 dB down).
3. Attenuates content above the LP cutoff (15 kHz → > 6 dB down).

Without this test, a bug where enabling the LP filter accidentally re-enables or resets
the HP filter state (or vice versa) would go undetected.

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/dsp/SidechainFilter.h` — setHighPassFreq(), setLowPassFreq(), enable flags
Read: `M-LIM/src/dsp/SidechainFilter.cpp` — how HP and LP are composed
Modify: `M-LIM/tests/dsp/test_sidechain_filter.cpp` — add combined HP+LP test

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R SidechainFilter --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_sidechain_filter.cpp::test_combined_hp_lp_passband` — configure
  HP at 80 Hz and LP at 8000 Hz. Feed three separate test signals:
  (a) 50 Hz sine → verify attenuation > 6 dB vs. unity,
  (b) 1000 Hz sine → verify attenuation < 1 dB (passes with < 1 dB loss),
  (c) 15000 Hz sine → verify attenuation > 6 dB vs. unity.
  Each measurement should use the settle + measure pattern from existing tests
  (`kSettleSamples=8192` + `kMeasureSamples=8192`).

## Technical Details
- Use `filter.setHighPassFreq(80.0f)` and `filter.setLowPassFreq(8000.0f)` and ensure both
  are enabled (check whether SidechainFilter requires separate enable flags or whether
  setting a frequency automatically enables the stage).
- For the attenuation measurement, use the same RMS + dB formula as in existing tests:
  `attenuationDb = 20 * log10(rmsOut / rmsIn)`.
- The 1 kHz mid-band signal should use a stereo (2-channel) AudioBuffer to match how
  SidechainFilter is called from LimiterEngine.
- Reset the filter between the three sub-tests (construct a fresh SidechainFilter instance
  or call prepare() again) to avoid filter state bleeding between frequency measurements.

## Dependencies
None
