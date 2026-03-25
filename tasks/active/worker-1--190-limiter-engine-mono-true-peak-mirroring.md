# Task 190: Verify True Peak Field Mirroring in LimiterEngine Mono Mode

## Description
`test_mono_metering_mirrors_L_to_R` (LimiterEngine test, line 677) verifies that `inputLevelR`
equals `inputLevelL` and `outputLevelR` equals `outputLevelL` for a mono engine.  It does **not**
check the `truePeakL` / `truePeakR` fields in `MeterData`.

LimiterEngine.cpp:327-335 shows that in mono mode (`numChannels == 1`), `mTruePeakR` is fed the
same input pointer as `mTruePeakL` (line 330), so `md.truePeakR` should equal `md.truePeakL`.
This mirroring logic is untested.

Tests to add in `test_limiter_engine.cpp`:

1. **test_mono_true_peak_mirrors_L_to_R** — Prepare a mono engine (`numChannels = 1`), enable
   true peak detection (`setTruePeakEnabled(true)`), process 10 blocks of a loud sine (amplitude
   0.9), drain the FIFO, and assert `md.truePeakR == Approx(md.truePeakL).margin(1e-6f)`.

2. **test_mono_true_peak_reports_nonzero** — Same setup but assert `md.truePeakL > 0.0f` to
   confirm true peak is actually being computed (not just two zeroes equalling each other).

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/dsp/LimiterEngine.cpp:320-345` — mono true peak mirroring logic
Read: `M-LIM/tests/dsp/test_limiter_engine.cpp:673-698` — existing test_mono_metering_mirrors_L_to_R for fixture pattern
Modify: `M-LIM/tests/dsp/test_limiter_engine.cpp` — add two new TEST_CASE blocks

## Acceptance Criteria
- [ ] Run: `cd M-LIM/build && ctest -R test_limiter_engine --output-on-failure` → Expected: all tests pass including the two new mono true peak tests
- [ ] Run: `grep -c "test_mono_true_peak" M-LIM/tests/dsp/test_limiter_engine.cpp` → Expected: 2

## Tests
- Unit: `tests/dsp/test_limiter_engine.cpp::test_mono_true_peak_mirrors_L_to_R` — truePeakR == truePeakL in mono mode
- Unit: `tests/dsp/test_limiter_engine.cpp::test_mono_true_peak_reports_nonzero` — truePeakL > 0 so test isn't vacuously true

## Technical Details
- Call `engine.setTruePeakEnabled(true)` before processing (uses the engine's setter).
- The MeterData struct has `truePeakL` and `truePeakR` float fields; check MeterData.h for exact
  field names.
- LimiterEngine.cpp:330: `mTruePeakR.processBlock(buffer.getReadPointer(0), numSamples)` — in mono
  mode, R detector processes channel 0 (same as L). Both should yield identical peaks.

## Dependencies
None
