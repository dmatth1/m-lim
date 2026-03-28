# Task: Remove `dynamicEnhance` Dead Field from `AlgorithmParams`

## Description

`AlgorithmParams::dynamicEnhance` (declared in `M-LIM/src/dsp/LimiterAlgorithm.h:26`) is set to non-zero values in six of the eight algorithm presets but is **never read or applied anywhere** in the codebase. The field carries a `// TODO: not yet implemented` comment.

This creates a misleading API surface: callers of `setAlgorithm()` believe they are selecting a preset that includes transient enhancement, but the field has zero effect on audio. All the non-zero preset values (`Punchy=0.2`, `Dynamic=0.6`, `Aggressive=0.3`, `Allround=0.3`, `Bus=0.1`, `Modern=0.2`) are silently ignored.

The fix is to remove the field entirely until the feature is genuinely implemented. If transient enhancement is planned, it should be added as a complete feature (parameter, DSP code, and tests) in its own dedicated task — not as a placeholder field with no effect.

**Exact location:** `M-LIM/src/dsp/LimiterAlgorithm.h:26` — `float dynamicEnhance;`

All 8 case branches and the default in `getAlgorithmParams()` pass a positional initializer list (e.g., `return { 0.3f, 0.5f, 0.0f, 0.0f, 6.0f, true };`); removing `dynamicEnhance` from the struct requires updating all 9 initializers to drop the 4th positional value.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/LimiterAlgorithm.h` — remove `dynamicEnhance` field from `AlgorithmParams` struct; update all 9 initializer lists in `getAlgorithmParams()` to drop the 4th positional argument
Read: `M-LIM/src/dsp/TransientLimiter.cpp` — verify `dynamicEnhance` is not referenced (search for usages before deleting)
Read: `M-LIM/src/dsp/LevelingLimiter.cpp` — same verification
Read: `M-LIM/src/dsp/LimiterEngine.cpp` — same verification

## Acceptance Criteria
- [ ] Run: `grep -r "dynamicEnhance" M-LIM/src/` → Expected: no output (field fully removed)
- [ ] Run: `cd /workspace/M-LIM/build && cmake --build . --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -5` → Expected: exit 0, no errors
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure -R "algorithm"` → Expected: all tests pass

## Tests
- Unit: `tests/dsp/test_limiter_algorithm.cpp` — verify existing algorithm parameter tests still pass after struct field removal (no new tests needed; field removal shouldn't change behavior since it was never used)

## Technical Details
The struct uses a positional initializer list (no designated initializers), so removing the 4th field shifts `kneeWidth` and `adaptiveRelease` to positions 4 and 5. All 9 initializer lists in `getAlgorithmParams()` must drop their 4th value.

If transient enhancement is later implemented, it should be added as a new task that includes: the `AlgorithmParams` field, actual DSP application in `TransientLimiter::process()`, a parameter in `Parameters.h`, and full test coverage.

## Dependencies
None
