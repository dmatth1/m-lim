# Task: Add `Dither::reset()` to Separate State-Clear from Coefficient Recomputation

## Description

`LimiterEngine::reset()` at line 122-124 calls `d.prepare(mSampleRate)` for each dither instance to clear the noise-shaping error state (`mError1`, `mError2`). The comment acknowledges this: `"Dither state — re-prepare to clear noise-shaping error buffers"`.

This is a misuse of `prepare()`: it recomputes the noise-shaping coefficients (`mCoeff1`, `mCoeff2`) from scratch on every reset, even though the sample rate hasn't changed. More importantly, it violates the semantic contract of `reset()` across the rest of the codebase — every other DSP component (`TruePeakDetector`, `DCFilter`, `SidechainFilter`, `Oversampler`) exposes a dedicated `reset()` method that clears runtime state without touching configuration. `Dither` is the only exception.

**Fix:** Add a `void Dither::reset() noexcept` method that zeroes `mError1` and `mError2` (and seeds a fresh `mRandom` state if desired). Update `LimiterEngine::reset()` to call `d.reset()` instead of `d.prepare(mSampleRate)`.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/Dither.h` — add `void reset() noexcept;` declaration
Modify: `M-LIM/src/dsp/Dither.cpp` — implement `reset()`: set `mError1 = 0.0f; mError2 = 0.0f;`
Modify: `M-LIM/src/dsp/LimiterEngine.cpp` — line 124: replace `d.prepare(mSampleRate)` with `d.reset()`

## Acceptance Criteria
- [ ] Run: `grep -n "d.prepare" M-LIM/src/dsp/LimiterEngine.cpp` → Expected: no output (prepare call in reset() replaced)
- [ ] Run: `cd /workspace/M-LIM/build && cmake --build . --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -5` → Expected: exit 0
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure -R "dither"` → Expected: all tests pass

## Tests
- Unit: `tests/dsp/test_dither.cpp::test_reset_clears_error_state` — after processing a block of non-silent audio (which accumulates noise-shaping error), call `reset()` and verify that the next block of silence produces output within the TPDF dither band only (no shaped error artefacts carried over). Compare against a freshly constructed `Dither` object on the same block.

## Technical Details
- `Dither::reset()` body: `mError1 = 0.0f; mError2 = 0.0f;` — nothing else is state that needs clearing on stream reset.
- `mRandom` state need not be reset (LFSRs with period >> block size don't produce audible periodicity artefacts at reset boundaries).
- `prepare()` retains its full behaviour (sets coefficients + clears error state) for the initial setup path.
- The `prepare()` doc comment on `Dither.h:24` should note: "For stream reset without reconfiguring, call `reset()` instead."

## Dependencies
None
