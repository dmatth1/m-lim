# Task 033: True Peak Detector — Exact ITU-R BS.1770-4 FIR Coefficients

## Description
Task 007 specifies a "48-tap FIR interpolation filter" for true peak detection but does not provide the actual ITU-R BS.1770-4 coefficient table. Workers implementing this without the exact coefficients will likely use a generic low-pass FIR, which will not be standards-compliant and will fail the ±0.1 dB true peak accuracy requirement.

This task ensures the TruePeakDetector uses the exact 4-phase polyphase FIR coefficients from ITU-R BS.1770-4, Table 1 (or the equivalent EBU R128 reference implementation).

## Produces
None

## Consumes
TruePeakDetectorInterface

## Relevant Files
Modify: `tasks/pending/007-true-peak-detector.md` — Add exact FIR coefficient table to Technical Details
Modify: `M-LIM/src/dsp/TruePeakDetector.cpp` — Verify coefficients match ITU-R BS.1770-4 (if already implemented)

## Acceptance Criteria
- [ ] Run: `grep -c "0.0017" M-LIM/src/dsp/TruePeakDetector.cpp` → Expected: at least 1 (characteristic coefficient value from the ITU table)
- [ ] Run: `cd M-LIM && cmake --build build --target MLIMTests -j$(nproc) && cd build && ctest -R test_true_peak --output-on-failure` → Expected: all tests pass

## Tests
- Unit: `tests/dsp/test_true_peak.cpp::test_itu_compliance_1khz` — 1kHz sine at 0dBFS: true peak should read between +3.0 and +3.1 dBTP (known ITU-R reference value for 1kHz at 44.1kHz sample rate)
- Unit: `tests/dsp/test_true_peak.cpp::test_itu_compliance_intersample` — Two adjacent samples at [0.0, 0.5, 1.0, 0.5, 0.0]: true peak should detect the interpolated peak > 1.0

## Technical Details
The ITU-R BS.1770-4 specifies a 4x oversampled FIR filter decomposed into 4 polyphase phases, each with 12 taps (48 taps total). The coefficients for each phase are:

**Phase 0** (original samples, should be identity-like):
```
0.0017089843750, 0.0109863281250, -0.0196533203125, 0.0332031250000,
-0.0594482421875, 0.1373291015625, 0.9721679687500, -0.1022949218750,
0.0476074218750, -0.0266113281250, 0.0148925781250, -0.0083007812500
```

**Phase 1** (1/4 sample offset):
```
-0.0291748046875, 0.0292968750000, -0.0517578125000, 0.0891113281250,
-0.1665039062500, 0.4650878906250, 0.7797851562500, -0.2003173828125,
0.1015625000000, -0.0582275390625, 0.0330810546875, -0.0189208984375
```

**Phase 2** (2/4 sample offset):
```
-0.0189208984375, 0.0330810546875, -0.0582275390625, 0.1015625000000,
-0.2003173828125, 0.7797851562500, 0.4650878906250, -0.1665039062500,
0.0891113281250, -0.0517578125000, 0.0292968750000, -0.0291748046875
```

**Phase 3** (3/4 sample offset):
```
-0.0083007812500, 0.0148925781250, -0.0266113281250, 0.0476074218750,
-0.1022949218750, 0.9721679687500, 0.1373291015625, -0.0594482421875,
0.0332031250000, -0.0196533203125, 0.0109863281250, 0.0017089843750
```

Note: Phase 0 and Phase 3 are mirrors, as are Phase 1 and Phase 2. The worker must store these as a `static constexpr float[4][12]` array.

The detector needs a circular buffer of 12 samples (the FIR history) per channel. For each input sample, compute 4 interpolated values (one per phase), take the max absolute value across all 4 phases, and update the running peak.

## Dependencies
Requires task 007 (TruePeakDetector must exist to verify)
