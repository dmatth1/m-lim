# Task: Address Dither Noise Potentially Exceeding True Peak Ceiling

## Description
In `LimiterEngine::process()`, the DSP chain order is:
1. stepApplyCeiling (hard clip)
2. stepDCFilter
3. stepEnforceTruePeak (ITU-R BS.1770-4 inter-sample peak enforcement)
4. stepDither (TPDF dithering + noise shaping)

Dither runs AFTER true peak enforcement, meaning dither noise can push inter-sample peaks above the enforced ceiling. For 24-bit dither (step = 2^(-23) ≈ 1.2e-7), the peak dither amplitude is ~2 * step ≈ 2.4e-7, which is -132 dBFS — completely inaudible and unlikely to cause a measurable true peak exceedance.

However, for 16-bit dither (step = 2^(-15) ≈ 3.05e-5), peak TPDF amplitude is ~6.1e-5 (-84 dBFS). With noise shaping mode 2, the shaped noise can be amplified by up to ~12 dB at Nyquist, giving peak noise of ~2.4e-4 (-72 dBFS). For a ceiling of -0.1 dBFS, this is negligible. But for professional broadcast compliance (EBU R128 TP ceiling of -1 dBTP), any exceedance matters.

**Fix**: Move dither BEFORE true peak enforcement in the process chain, so enforcement accounts for dither noise. The DC filter should also precede enforcement.

Proposed order:
1. stepApplyCeiling
2. stepDCFilter
3. stepDither
4. stepEnforceTruePeak

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/LimiterEngine.cpp` — reorder steps in process() around line 232-235

## Acceptance Criteria
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass, exit 0
- [ ] Verify that with 16-bit dither and noise shaping enabled, true peak never exceeds ceiling

## Tests
- Unit: `tests/dsp/test_limiter_engine.cpp::test_dither_before_true_peak_enforcement` — process a near-ceiling signal with 16-bit dither + noise shaping mode 2, verify true peak stays within ceiling + 0.01 dB tolerance

## Technical Details
- This is a 2-line reorder in process() — swap the order of stepDither and stepEnforceTruePeak calls
- DC filter should also move before dither (already is in current order)
- No algorithm changes needed, just call reordering

## Dependencies
None
