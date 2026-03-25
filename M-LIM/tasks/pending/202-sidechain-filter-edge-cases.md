# Task 202: SidechainFilter — weak assertions and edge-case tests

## Description
`test_sidechain_filter.cpp` has two weaknesses identified by QA review:

1. **Weak attenuation assertion** — the existing HP filter test checks
   `REQUIRE(attenuationDb < -6.0)` but does not verify _how much_ attenuation, nor does it check
   that the attenuation is proportional to frequency (i.e., actually acting as a high-pass). Fix by
   additionally checking that attenuation at cutoff/10 is at least 3× (in dB) the attenuation at
   cutoff/2 — demonstrating roll-off, not just a fixed loss.

2. **Missing: frequencies near Nyquist** — no tests exercise the filter at cutoff frequencies
   near Nyquist (e.g., 18000 Hz at 44100 Hz sample rate). A bilinear-transform biquad can become
   numerically unstable or sonically wrong when the pole/zero approaches the unit circle.

Add tests for:

1. **HP filter attenuation gradient** — at 44100 Hz with HP cutoff at 1000 Hz: measure output RMS
   at 100 Hz and at 500 Hz; verify `rms_at_100hz < rms_at_500hz` (more attenuation lower in
   frequency, confirming roll-off).
2. **LP filter attenuation gradient** — symmetric test for LP cutoff at 5000 Hz; measure at 10 kHz
   and 20 kHz; verify `rms_at_20khz < rms_at_10khz`.
3. **HP cutoff near Nyquist (18000 Hz at 44100 Hz)** — process a 440 Hz sine for 512 samples;
   output must be finite and the attenuation at 440 Hz must be > 20 dB.
4. **LP cutoff near 20 Hz** — process a 440 Hz sine with LP cutoff at 20 Hz for 512 samples;
   output must be finite and significantly attenuated (> 20 dB).
5. **Repeated `prepare()` doesn't accumulate state** — call `prepare(44100, 512)` three times;
   process a 440 Hz sine; output must be finite and within ±3 dB of the first-prepare output.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `tests/dsp/test_sidechain_filter.cpp` — fix existing assertion, add new TEST_CASEs
Read: `src/dsp/SidechainFilter.h` — prepare(), process(), setHighPassFreq(), setLowPassFreq(),
    any tilt filter API

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R "SidechainFilter" --output-on-failure` → Expected: all sidechain filter tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_sidechain_filter.cpp::test_hp_attenuation_gradient` — more attenuation at lower frequency, confirming roll-off shape
- Unit: `tests/dsp/test_sidechain_filter.cpp::test_lp_attenuation_gradient` — more attenuation at higher frequency above cutoff
- Unit: `tests/dsp/test_sidechain_filter.cpp::test_hp_cutoff_near_nyquist` — 18kHz HP cutoff, 440 Hz input attenuated > 20 dB, output finite
- Unit: `tests/dsp/test_sidechain_filter.cpp::test_lp_cutoff_near_20hz` — 20 Hz LP cutoff, 440 Hz input attenuated > 20 dB, output finite
- Unit: `tests/dsp/test_sidechain_filter.cpp::test_reprepare_no_state_accumulation` — three prepare() calls, output within ±3 dB of single-prepare result

## Technical Details
- Compute RMS of the output buffer: `sqrt(sumSq / N)` where sumSq sums `sample*sample`.
- Attenuation in dB: `20 * log10(rms_out / rms_in)`. For a 0.5f amplitude sine, `rms_in = 0.5/sqrt(2)`.
- For the gradient tests, run two separate `SidechainFilter` instances (or re-prepare) for the
  two test frequencies; compare their output RMS values.
- Feed at least 512 samples (one full block) to allow the biquad to reach steady state.
- Check `SidechainFilter.h` for the actual API — look for `setHighPassEnabled()`, `setHighPassFreq()`,
  `setLowPassEnabled()`, `setLowPassFreq()`, or combined `setFilter()` method.

## Dependencies
None
