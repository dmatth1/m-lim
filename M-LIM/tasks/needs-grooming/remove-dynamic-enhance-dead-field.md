# Task: Remove Dead `dynamicEnhance` Field from AlgorithmParams

## Description
`AlgorithmParams.dynamicEnhance` (declared in `LimiterAlgorithm.h` line 26) is explicitly marked
`// TODO: not yet implemented` and is never read by any DSP processing code. It is:

- Written in every `getAlgorithmParams()` case (8 branches)
- Copied into `mParams` on every `setAlgorithmParams()` call in `TransientLimiter` and `LevelingLimiter`
- Validated in tests (`test_limiter_algorithm.cpp` lines 38, 57, 58, 397)

But it is **never read** anywhere in `TransientLimiter.cpp`, `LevelingLimiter.cpp`, or `LimiterEngine.cpp`.
Keeping it in the struct misleads future developers into thinking this feature exists, adds a dead field
to a hot-path struct (copied on every algo change), and creates false test coverage.

Remove the field entirely. If dynamic transient enhancement is ever implemented, it should be added
back with a real implementation — not left as a dead placeholder.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/LimiterAlgorithm.h` — remove `dynamicEnhance` field from `AlgorithmParams`
  and remove all `dynamicEnhance` values from the 8 `getAlgorithmParams()` initialiser lists
Modify: `M-LIM/tests/dsp/test_limiter_algorithm.cpp` — remove the 3 test lines that validate
  `dynamicEnhance` (lines 38, 57–58, 397) since the field no longer exists

## Acceptance Criteria
- [ ] Run: `grep -rn "dynamicEnhance" M-LIM/src/ M-LIM/tests/` → Expected: no output
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass

## Tests
None (the tests for `dynamicEnhance` should be removed along with the field)

## Technical Details
- `AlgorithmParams` struct is in `LimiterAlgorithm.h`, a header-only translation unit
- All 8 branches of `getAlgorithmParams()` pass a 6th initialiser for `dynamicEnhance` — after
  removal, all initialisers drop to 5 values (transientAttackCoeff, releaseShape, saturationAmount,
  kneeWidth, adaptiveRelease)
- No source file outside of `LimiterAlgorithm.h` and the test file references `dynamicEnhance`

## Dependencies
None
